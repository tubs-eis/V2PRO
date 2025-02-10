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
#ifndef DCONV_CONV_H
#define DCONV_CONV_H

#include "Base/base_layer.h"
#include "Layer_Conv/conv_layer.h"

namespace CNN_LAYER {

  // Flat "convolution", applying using a 1xN kernel and only using each input once.
  class DConvConv: public Conv2D {

    // methods
  public:
    virtual std::string getLayerTypeName() { return "DConvConv"; }
    virtual LAYERTYPE getLayerType() { return LAYERTYPE::DCONV_CONV; }

    virtual void processParams() {
      // Stride not supported
      assert(stride == 1);
      // No padding needed, so padding mode does not make sense.
      assert(padding_mode == PADDING_MODE::SAME);
      Conv2D::processParams();
      // DConvConv is never padded
      this->padding.algo = {};

      // remove double buffering optimization
      layercfg.use_dma_store_splitter = false;
      lm_lane_stride = VPRO_CFG::RF_SIZE * 2;
    };

    virtual void computeOutputDim() {
      assert(in_dim(0).x % kernel_length == 0);
      out_dim.x = in_dim(0).x / kernel_length;
      out_dim.y = in_dim(0).y;
      conv_out_dim = out_dim;
    }

    virtual int expectedWeightCount() {
      int kernel_size = out_dim.ch * in_dim(0).ch * kernel_length;
      int bias_size = (use_bias) ? out_dim.ch : 0;
      return kernel_size + bias_size;
    }

    virtual mm_addr_type getBiasMMAddr(int out_channel=0) {
      return getWeightsMMAddr() + sizeof(weight_t) * (out_dim.ch * in_dim(0).ch * kernel_length + out_channel);
    }

    virtual mm_addr_type getKernelMMAddr(int in_channel=0, int out_channel=0, int x=0, int y=0) {
      // Expected format: Kernel in order (ch_in, ch_out, x, y)
      return getWeightsMMAddr() + sizeof(weight_t) * (x + kernel_length * (out_channel + out_dim.ch * in_channel));
    }

    virtual void setSegmentDimensions();

    virtual void generateCommands();

    virtual BIF::COMMAND_SEGMENT convVPRO(const CNN_LAYER::SEGMENT &segment, BUFFER &buffer, uint32_t lane_mask, BIF::COMMAND_VPRO &mem_layout);

    virtual std::string getLayerInfoText() {
      std::stringstream ss;
      ss << Layer::getLayerInfoText();
      ss << "  kernel " << 1 << "x" << kernel_length << "\n";
      return ss.str();
    }

    virtual DMA_COMMANDS::DMA_DESCRIPTOR kernelLoad(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER buffer);

    virtual DMA_COMMANDS::DMA_DESCRIPTOR biasLoad(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER buffer);

    virtual BIF::COMMAND_SEGMENT shiftStoreVPRO(BIF::COMMAND_VPRO &mem_layout, BUFFER &store_buffer);

    BIF::COMMAND_SEGMENT dataStore(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER &buffer_load);
  }; // class DConvConv

} // namespace CNN_LAYER

#endif
