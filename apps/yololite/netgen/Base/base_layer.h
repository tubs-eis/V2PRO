/*
 *  * Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
 *                    Technische Universitaet Braunschweig, Germany
 *                    www.tu-braunschweig.de/en/eis
 * 
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 * 
 */
#ifndef LAYER_BASE_H
#define LAYER_BASE_H

#include <cassert>
#include <cstdint>
#include <fstream>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <list>
#include <math.h>
#include <string>
#include <vector>

#include "misc.h"
#include "bif.h"
#include "DMACommandExtensions/interleaver.h"
#include "DMACommandExtensions/dma_block_extension.h"
#include "command_helpers.h"
#include "segment.h"
#include "base_enum.h"
#include "dim.h"

// == Coarse program flow overview (last updated 2023-09-29)
// Net::generateNet()
//     Net::instantiateLayers()
//         // generate CNN graph
//         Layer::processParams() for layers
//             Layer::computeOutputDim() // -> out_dim.(x|y)
//             Layer::computeInputPadding() // -> padding.algo
// 
//     Net::designMmLayoutVpro()
//         Layer::setOutputMMAddr() for layers
//             Layer::setSegmentDimensions();
//                 // divide image into separately processable chunks -> seg.(num|in|out).(w|h|x_stride|y_stride)
//             Layer::computeDmaPadding() // padding.algo -> padding.dma
//             Layer::setOutputMemDimensions() // -> out_dim.mm.(x|y)
//             Layer::calcOutputMemLayout() // derive all other out_dim.mm.* fields from out_dim.mm.(x|y)
// 
//     Net::generateLayerExecList()
//     Net::generateVproBlob() // weights
//     Net::generateEisvBlob() // program
//         Layer::generateCommandSegments() for layers
//             Layer::generateSegments() // -> segments[]
//                 // map segments to lanes
//                 Layer::firstInputChannel()
//                 Layer::getSegment()
//                 Layer::compatibleSegmentsBlock()
//                 Layer::insertRepeatedSegmentSetForAllInChannels()
//                     Layer::getSegment()
//                     Layer::nextInputChannel()
//             Layer::generateCommands() // -> commands[]
//                 // double buffering
//                 loop {
//                     Conv::load() // DMA MM->LM for each individual unit
//                     Conv::compute() // VPRO instructions broadcast to all lanes; results in LM
//                     Layer::store() // DMA LM->MM
//                         dataStore() for each lane
//                 }
//             Layer::compressCommands()
//                 Layer::DmaMerger()
//                 Layer::DmaBlockExtension()
//                 Layer::DmaLoopExtension()
//                 Layer::DmaClusterMixer()
//     Net::exportEisvBlob()
//     Net::exportVproBlob()
//     Net::exportLayersText()
//     Net::exportSegmentsText()
//     Net::exportCommandsText()
//     Net::exportSimInputConfig()
//     Net::exportSimOutputConfig()

// Distinct phases:
// 1. setup algorithm-view CNN structure in CNN_NET::Net.instantiateLayers()
//   - allocate layers
//   - connect layers: addSrcLayers()
//   - assign layer parameters
//   - load weights, quantization parameters etc.
//   --> CNN structure, weights etc. stored in CNN_LAYER::Layer and CNN_NET::Net objects
//
// 2. implementation
//   - allocate space for CNN results in VPRO memory: setOutputMMAddr()
//     - layer computes everything required to know it's output size
//       - segmentation
//   - generate output files (BLOBs, debug files etc.)

// == design principles
// - separate cnn structure description (may be hand-crafted) from coefficients (always auto-generated)
// - only places requiring adaptations are the cnn structure description and auto-generated coefficients
// - easily extendable to new layer types (no copy-paste orgies!)
// - syntax as simple as possible

namespace CNN_LAYER {


  // abstract base class
  class Layer {
  // user-supplied parameters
  // Manually call processParams() after all parameters have been set!
  public:
    std::string name{"<not set>"}; // arbitrary human-readable name; does not need to be unique
    int number{-1}; // the unique handle of each layer: arbitrary user-supplied integer; printed in parentheses: (17)
    Dim out_dim{};    // exactly one output dimension per layer
    // grouped conv etc.
    int groups{GROUPS_UNSET}; // default filled in by processParams() individually per layer class unless assigned to explicitly; do not assign to during layer instantiation when in doubt
    // base Layer knows segmentation and command generation for grouped computation, mainly used by Conv layers
    // groups == 1: each output channel depends on all input channels
    // groups == out_dim.ch == in_dim.ch: each output channel depends on one input channel only
    bool out_is_result{}; // layer output is CNN result
    LAYERCFG layercfg;

  // methods
  public:
    virtual std::string getLayerTypeName() { return "Layer"; }
    virtual LAYERTYPE getLayerType() { return LAYERTYPE::UNKNOWN_LAYERTYPE; }

    // call after all parameters have been set and before using the layer
    virtual void processParams();

    virtual std::string getDefaultWeightsFilename();

    // CNN structure

    // specify layer inputs
    void addSrcLayers(std::initializer_list<Layer *> new_layers);

    // called internally by addSrcLayers only
    void addDest(Layer *layer) { dest_layers.push_back(layer); }

    // takes into account "transparent layers", which do no execution on VPRO (not produces_binary_data)
    bool is_transient_input_layer() {
      if (is_input_layer)
        return true;

      if (produces_binary_data) // this layer computes
        return false;

      for (auto sl: src_layers) {
        if (sl->is_transient_input_layer())
          return true;
      }
      return false;
    }

    // input dimensions are determined by the output of the source
    virtual Dim& in_dim(int input_idx) {  return src_layers[input_idx]->out_dim; }

    // conv input is the padded feature map
    // mm.channel_base is the top left corner of unpadded feature map -> shift to top left of padded feature map
    virtual int padded_in_MM_base(int input_idx, int ch) {
      return in_dim(input_idx).mm.channel_base[ch] - 2 * (in_dim(0).mm.x * padding.dma.top + padding.dma.left);
    }

    // algorithm-view geometry: derive out_dim.(x|y) from inputs and parameters
    // implementation-view out_dim.mm.* will be set by setOutputMMAddr()
    virtual void computeOutputDim();

    // algorithm-view
    virtual void computeInputPadding();

    virtual void computeDmaPadding();

    virtual int expectedWeightCount() { return 0; } // default: layer expects no weights

    // quantized weights
    void loadWeights(const std::string &path = "");

    virtual void loadQuantData() {
      loadWeights();
      // FIXME load quantization parameters (shifts etc.)
    }

    // called by memory management
    virtual void setOutputMMAddr(mm_addr_type base_addr);

    // tell memory management how much output space is required (may be larger than actual payload data)
    virtual mm_size_type getOutputMMSize();

    // set quantized weights
    virtual void setWeights(std::vector<weight_t> &weights);

    void setWeightsMMAddr(mm_addr_type addr) { mm_weights = addr; }
    mm_addr_type getWeightsMMAddr() { return mm_weights; }

      // in bytes
    virtual mm_size_type getWeightsMMSize();

    // return value: does this layer have a BIF::LAYER representation? Input layers do not.
    virtual void generateBifLayer(BIF::LAYER &bl);

    std::vector<BIF::COMMAND_SEGMENT> &generateCommandSegments();

    // groups
    virtual int firstInputChannel(int x, int y, int out_ch, int src_idx = 0);
    virtual int lastInputChannel(int x, int y, int out_ch, int src_idx = 0);
    virtual int nextInputChannel(int x, int y, int in_ch, int out_ch, int src_idx = 0);
    virtual int numUsedInputChannels(int x, int y, int out_ch, int src_idx = 0);
    virtual bool usesInputCh(int x, int y, int in_ch, int out_ch, int src_idx = 0);

    virtual SEGMENT *getSegment(int x, int y, int in_ch, int out_ch);
    virtual void setSegmentDimensions();

    virtual bool compatibleSegmentsBlock(const SEGMENT *a, const SEGMENT *b, int lane, int lane_out_ch) const;
    virtual void insertRepeatedSegmentSetForAllInChannels(std::vector<SEGMENT *> &set, int &appended_segs, int &appended_dummies);

    virtual void generateSegments();
    virtual void generateCommands();
    virtual void compressCommands();

    virtual void load(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer) {}
    virtual void compute(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer, BUFFER &store_buffer) {}
    virtual void store(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer);

    // implementation-view memory layout
    virtual void setOutputMemDimensions();  // set out_dim.mm.(x|y) // formerly virtual void setOutputMMAccessor();
    virtual void calcOutputMemLayout(); // derive other out_dim.mm.* fields from out_dim.mm.(x|y)

    bool sanityCheckWeightsCount(int weights_count);

    // debug
    virtual std::string getFullName();

    // characters 'I' and/or 'O' if this layer is a CNN input and/or output
    virtual std::string getIoStr(bool pre_space = false, bool post_space = false);
    std::string segDimStr();
    std::string cmdCntStr();

    virtual std::string getLayerInfoText();
    virtual std::string getSegmentsString();
    virtual std::string getSegmentsShortString();
    virtual std::string getLaneUsageString();
    virtual std::string getCommandsString();

    void paddedSegmentToDma(const SEGMENT &segment, DMA_COMMANDS::DMA_DESCRIPTOR &dma, int source);
    DMA_COMMANDS::DMA_DESCRIPTOR dataLoad(const SEGMENT &segment, int cluster, int unit, BUFFER &buffer, int source=0);
    DMA_COMMANDS::DMA_DESCRIPTOR dataLoad1D(const SEGMENT &segment, int cluster, int unit, BUFFER &buffer, int source=0);
    virtual BIF::COMMAND_SEGMENT dataStore(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER &buffer_load) {
      return dataStore2D(segment, cluster, unit, lane, buffer_load); // default: 2D
    };
    virtual BIF::COMMAND_SEGMENT dataStore2D(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER &buffer_load);
    virtual BIF::COMMAND_SEGMENT dataStore1D(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER &buffer_load);

    bool weights_loaded = false;
    std::vector<weight_t> weights_packed; // format depends on derived class

    bool qparams_loaded = false;

    std::vector<SEGMENT *> segments; // flattened representation of [VPRO_CFG::CLUSTERS][VPRO_CFG::UNITS][VPRO_CFG::LANES]
    std::vector<BIF::COMMAND_SEGMENT> commands; // commands of this layer

  // internal TODO: public?
  public:
    std::vector<Layer *> src_layers; // links to all source layers; use addSrcLayers()
    std::vector<Layer *> dest_layers; // links to all layers consuming this layer's output; set automatically

    bool produces_binary_data{true};
    bool is_input_layer{false};
    bool use_dynamic_shape{false};

    // in the order of Net.layer_execlist
    // FIXME storing first../last.. here does not work as expected for layers that are used multiple times in Net.layer_execlist
    bool last_layer_using_input{false}; // CNN input
    bool first_layer_producing_output{false}; // CNN output

    int parallel_outchannels_per_lane{1}, parallel_inchannels_per_lane{1};

    // Memory layout
    mm_addr_type mm_weights; // main memory address of weights

    int lm_lane_stride{STRIDE_UNSET}; // result stride in LM

    // input padding
    struct {
        BIF::PAD_REDUCED algo; // algorithm-view: additional pixels around in_dim.(x|y)
        BIF::PAD_REDUCED dma; // padding pixels inside segmented image
        bool enabled{true}; // disable input padding for non-standard use of segmentation
    } padding;

    // Segmentation: divide image into separately computable regions
    segDim seg;

    CMD_COUNT cmd_cnt;

  protected:
    std::string weights_fname;
    std::string qparams_fname;

  }; // class Layer

} // namespace CNN_LAYER

#endif
