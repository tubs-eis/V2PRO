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
#ifndef CONV_H
#define CONV_H

#include "Base/fusedfunc_layer.h"

namespace CNN_LAYER {

  // generic base class for all convolutions
  class Conv: public FusedFunc {
  // user-supplied parameters
  public:
    int kernel_length{1};
    int stride{1};
    std::vector<int> dilation_rate{};
    bool use_bias{false};
    // pre_zp and padding_mode are independent; BOTH allowed at the same time
    BIF::PAD_REDUCED pre_zp{}; // ZeroPadding2D layer fused into this layer
    PADDING_MODE padding_mode{PADDING_MODE::SAME}; // Conv2D padding mode
    
    qparam_t result_shift_right{0};
    qparam_t bias_shift_right{0};

    int outchannel_block_size{-1};
    int outchannel_parallelism{-1};

    // methods
  public:

    virtual std::string getLayerInfoText() {
      std::stringstream ss;
      ss << FusedFunc::getLayerInfoText();
      ss << "  conv_seg wh " << conv_seg_w << "x" << conv_seg_h << "\n";
      return ss.str();
    }

    virtual void processParams() {
      assert(kernel_length > 0);
      assert(stride > 0);
      for (int d: dilation_rate) {
        assert((stride == 1 || d == 1) && "Either dilation_rate or stride must be 1");
        (void)d;
      }
      FusedFunc::processParams();
    };

    // binary export
    virtual void generateBifLayer(BIF::LAYER &bl) {
      FusedFunc::generateBifLayer(bl);

      bl.seg_out_w = conv_seg_w;
      bl.seg_out_h = conv_seg_h;

      bl.stride = stride;
      bl.kernel_length = kernel_length;
      bl.conv_groups = groups;
      bl.dilation_rate_w = dilation_rate[0];
      bl.dilation_rate_h = dilation_rate[1];

      bl.conv_result_shift_right = result_shift_right;
      bl.bias_shift_right = bias_shift_right;
    }

    virtual BIF::COMMAND_SEGMENT convVPRO(const CNN_LAYER::SEGMENT &segment, BUFFER &buffer, uint32_t lane_mask, BIF::COMMAND_VPRO &mem_layout);
    virtual void compute(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer, BUFFER &store_buffer);
    virtual void generateCommands();

  protected:
    Dim conv_out_dim;
    int conv_in_dim_w{0};
    int conv_in_dim_h{0};

    // former globals from vpro_functions.h
    uint32_t RF_KERNEL_BASE;
    uint32_t RF_BIAS_BASE;
    uint32_t kernel_x;
    uint32_t kernel_y;

    int conv_seg_w; // segmentation: conv output dimension (before fused pooling/upsampling)
    int conv_seg_h;

  }; // class Conv

  class Conv2D: public Conv {

  // methods
  public:
    virtual std::string getLayerTypeName() { return "Conv2D"; }

    virtual LAYERTYPE getLayerType() { return LAYERTYPE::CONV2; }

    virtual void processParams() {
      if (!dilation_rate.size())
        dilation_rate = {1, 1};
      assert(dilation_rate.size() == 2);
      dilated_kernel_w = (kernel_length - 1) * dilation_rate[0] + 1;
      dilated_kernel_h = (kernel_length - 1) * dilation_rate[1] + 1;
      conv_in_dim_w = in_dim(0).x + pre_zp.left + pre_zp.right;
      conv_in_dim_h = in_dim(0).y + pre_zp.top + pre_zp.bottom;
      Conv::processParams();
    };
    
    virtual void generateSegments();
    virtual void generateCommands();

    virtual void computeOutputDim() {
      // chain: zeropadding - conv - maxpool2x2

      // tensorflow, keras etc. use different formulas for padding and hence output size. This is valid for
      // - legacy cnn_converter
      // - tensorflow
      conv_out_dim.x = conv_in_dim_w;
      conv_out_dim.y = conv_in_dim_h;
      conv_out_dim.ch = out_dim.ch;

      // https://www.tensorflow.org/api_docs/python/tf/nn#notes_on_padding_2
      if (padding_mode == PADDING_MODE::VALID) {       
        conv_out_dim.x -= (int)dilated_kernel_w - 1;
        conv_out_dim.y -= (int)dilated_kernel_h - 1;
      }
      conv_out_dim.x = ceil_div(conv_out_dim.x, stride);
      conv_out_dim.y = ceil_div(conv_out_dim.y, stride);

      out_dim.x = conv_out_dim.x;
      out_dim.y = conv_out_dim.y;
    }

    virtual void computeInputPadding() {
      // compute algorithm-view input padding from PADDING_MODE; FIXME this could be moved to nn_quantization
      // algorithm-view:
      // - padding.algo specifies additional pixels around in_dim.(x|y)
      // - convolution input size is in_dim.(x|y) + padding.algo
      // tensorflow, keras etc. use different formulas for padding and hence output size.
      // This is valid for tensorflow:
      if (padding_mode == PADDING_MODE::SAME && kernel_length > 1) { // FIXME really does not apply to kernel_size == 1?
        // https://www.pico.net/kb/what-is-the-difference-between-same-and-valid-padding-in-tf-nn-max-pool-of-tensorflow/
        int pad_x = (conv_out_dim.x - 1) * stride + dilated_kernel_w - conv_in_dim_w;
        int pad_y = (conv_out_dim.y - 1) * stride + dilated_kernel_h - conv_in_dim_h;
        padding.algo.left    = pad_x / 2;
        padding.algo.right   = pad_x - padding.algo.left;
        padding.algo.top     = pad_y / 2;
        padding.algo.bottom  = pad_y - padding.algo.top;
      }
      padding.algo.left += pre_zp.left;
      padding.algo.right += pre_zp.right;
      padding.algo.top += pre_zp.top;
      padding.algo.bottom += pre_zp.bottom;
      // padding.dma will be set later after segmentation is known
    }

    virtual int expectedWeightCount() {
      int kernel_size = out_dim.ch * in_dim(0).ch / groups * kernel_length * kernel_length;
      int bias_size = (use_bias) ? out_dim.ch : 0;
      // std::cout << "Layer " << name << ": kernel_size " << kernel_size << ", bias_size " << bias_size << "\n";
      return kernel_size + bias_size;
    }

    virtual mm_addr_type getBiasMMAddr(int out_channel=0) {
      return getWeightsMMAddr() + sizeof(weight_t) * (out_dim.ch * in_dim(0).ch / groups * kernel_length * kernel_length + out_channel);
    }

    virtual mm_addr_type getKernelMMAddr(int in_channel=0, int out_channel=0, int x=0, int y=0) {
      // fprintf(stderr, "Conv2D::getKernelMMAddr(in_channel = %d, out_channel = %d, x = %d, y = %d), in_dim(0).ch = %d, out_dim.ch = %d, groups = %d\n", in_channel, out_channel, x, y, in_dim(0).ch, out_dim.ch, groups);
      // Expected format: Kernel in order (ch_in, ch_out, y, x); x, y: coeff offset within kernel
      // grouped conv: each output channel only depends on a subset of input channels
      // C++ memory layout: kernel[in_group_len][out_dim.ch][kernel_length_y][kernel_length_x] (nn_quant: IOHW)
      // out_dim.ch and in_dim.ch are divisable by groups
      // normal (groups = 1) and depthwise (groups = out_dim.ch = in_dim.ch) are corner cases of grouped conv
      assert((in_channel >= 0 && in_channel < in_dim(0).ch) && "in_channel out of range");
      assert((out_channel >= 0 && out_channel < out_dim.ch) && "out_channel out of range");
      int in_group_len = in_dim(0).ch / groups; // input channels per group
      int out_group_len = out_dim.ch / groups; // output channels per group
      int group = out_channel / out_group_len;
      assert(in_channel / in_group_len == group && "grouped conv: output channel does not depend on requested input channel");

      int in_offs = in_channel % in_group_len; // channel offset within input group
      return getWeightsMMAddr() + sizeof(weight_t) * (x + kernel_length/*_x*/ * (y + kernel_length/*_y*/ * (out_channel + out_dim.ch * in_offs)));
    }

    virtual void setSegmentDimensions();
//    virtual void calcOutputMemLayout();

    virtual void setOutputMemDimensions();

    virtual void load(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer);
    virtual void store(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer);

    virtual BIF::COMMAND_SEGMENT dataStore(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER &buffer_load);

    virtual DMA_COMMANDS::DMA_DESCRIPTOR biasLoad(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER buffer);
    virtual DMA_COMMANDS::DMA_DESCRIPTOR kernelLoad(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER buffer);

    virtual std::string getLayerInfoText() {
      std::stringstream ss;
      ss << Conv::getLayerInfoText();
      ss << "  fused pre-zeropadding trbl " << pre_zp.top << ", " << pre_zp.right << ", " << pre_zp.bottom << ", " << pre_zp.left << "\n";
      ss << "  padding_mode " << to_string(padding_mode) << ", kernel " << kernel_length << "x" << kernel_length << ", stride " << stride << ", dilation rate " << dilation_rate[0] << "x" << dilation_rate[1]
         << " -> dilated kernel " << dilated_kernel_w << "x" << dilated_kernel_h << ", conv_out_dim " << conv_out_dim.x << "x" << conv_out_dim.y << "\n";

      return ss.str();
    }

  private:
    uint32_t dilated_kernel_w;
    uint32_t dilated_kernel_h;
    int overcalc_elements_1d{1};  // by to large segment size (n*size > input)
  }; // class Conv2D

} // namespace CNN_LAYER

#endif
