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
#ifndef DCONV_DEFORM_H
#define DCONV_DEFORM_H

#include "Base/base_layer.h"

namespace CNN_LAYER {
  class DConvDeform: public Layer {
    // user-supplied parameters
  public:
    // kernel size (kernel_width * kernel_height) of final DeformableConv Convolution, only 3x3 = 9 is supported
    int kernel_size{9};

    // Maximum allowed offset for deform, values sampled from larger offsets are set to 0
    int max_offset_x{4};
    int max_offset_y{4};

    qparam_t result_shift_right{8};

  public:
    virtual std::string getLayerTypeName() { return "DConvDeform"; }
    virtual LAYERTYPE getLayerType() { return LAYERTYPE::DCONV_DEFORM; }

    virtual void processParams() {
      layercfg.scheduling_order = ITERATE_ALL_SORTED_OUTC;
      assert(kernel_size == 9);
      Layer::processParams();

      groups = out_dim.ch; // segment generation: one input channel per output channel
    };

    // Deform requires exactly two input layers, the inputs and the offsets + masks
    void setSrcLayers(Layer* input, Layer* offset);

    virtual void computeOutputDim();
    virtual int expectedWeightCount();

    // binary export
    virtual void generateBifLayer(BIF::LAYER &bl);
    virtual void setSegmentDimensions();
    virtual SEGMENT *getSegment(int x, int y, int in_ch, int out_ch);

    virtual bool compatibleSegmentsBlock(const SEGMENT *a, const SEGMENT *b, int lane, int lane_out_ch) const;
    
    // virtual void generateCommands(bool use_interleaving=false, bool use_dma_extension=false);

    virtual void load(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer);
    virtual void compute(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer);
    virtual void store(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer);

    DMA_COMMANDS::DMA_DESCRIPTOR staticOffsetLoad(int cluster, int unit);
    DMA_COMMANDS::DMA_DESCRIPTOR inputLoad(const SEGMENT &segment, BIF::PAD_REDUCED pad, int cluster, int unit, BUFFER buffer);
    DMA_COMMANDS::DMA_DESCRIPTOR offsetLoad(const SEGMENT &segment, int row, int cluster, int unit, BUFFER buffer);
    BIF::COMMAND_SEGMENT dataStore(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER &buffer_load);

    BIF::COMMAND_SEGMENT setDMAPadding(BIF::PAD_REDUCED pad);

    BIF::COMMAND_SEGMENT dconvDeformVPRO(const CNN_LAYER::SEGMENT &segment, BUFFER &buffer);

    virtual std::string getLayerInfoText();

  protected:
    // Interface for input and offset access
    static constexpr int INPUT_SRC_INDEX = 0;
    static constexpr int OFFSET_SRC_INDEX = 1;

    Dim getInputDim() { return in_dim(INPUT_SRC_INDEX); }
    Dim getOffsetDim() { return in_dim(OFFSET_SRC_INDEX); }

    // Memory layout helpers
    uint16_t lm_input_segment_width() const;
    uint16_t lm_input_segment_height() const;
    uint16_t lm_input_size() const;
    uint16_t lm_offset_size() const;
    uint16_t lm_output_size() const;
    uint16_t lm_static_offset_size() const;
    uint16_t lm_input_addr(BUFFER buffer) const;
    uint16_t lm_offset_addr(BUFFER buffer) const;
    uint16_t lm_output_addr(BUFFER buffer) const;
    uint16_t lm_static_offset_addr() const;

  }; // class DConvDeform

} // namespace CNN_LAYER

#endif
