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
#ifndef CONV1D_H
#define CONV1D_H

#include "Layer_Conv/conv_layer.h"

namespace CNN_LAYER {

class Conv1D: public Conv {
  // user-supplied parameters
public:
  bool load_weights_at_once = true; // whether to load all weights required to compute one output channel at once
  // methods
public:
  virtual std::string getLayerTypeName() { return "Conv1D"; }
  virtual LAYERTYPE getLayerType() { return LAYERTYPE::CONV1; }

  virtual void processParams() {
    assert(in_dim(0).y == 1 && "Current implementation of Conv1D demands input shape (x, y=1, ch)!");
    assert(kernel_length == 1 && "Current implementation of Conv1D requires kernel length 1!");
    assert(stride == 1 && "Current implementation of Conv1D requires stride 1!");
    assert(padding_mode == PADDING_MODE::VALID && "Padding SAME not implemented for Conv1D");
    assert(load_weights_at_once && "Loading weights separately is currently not handled correctly in segment_scheduling.cpp!");
    if (!dilation_rate.size())
      dilation_rate = {1};
    assert(dilation_rate.size() == 1);
    assert(dilation_rate[0] == 1 && "Dilation not implemented for Conv1D");
    Conv::processParams();
  };

  virtual int expectedWeightCount() {
    int kernel_size = out_dim.ch * in_dim(0).ch / groups * kernel_length;
    int bias_size = (use_bias) ? out_dim.ch : 0;
    return kernel_size + bias_size;
  }

  mm_addr_type getBiasMMAddr(int out_channel=0) {
    return getWeightsMMAddr() + sizeof(weight_t) * (out_dim.ch * in_dim(0).ch / groups * kernel_length + out_channel);
  }

  mm_addr_type getKernelMMAddr(int in_channel=0, int out_channel=0, int x=0) {
    // fprintf(stderr, "Conv2D::getKernelMMAddr(in_channel = %d, out_channel = %d, x = %d, y = %d), in_dim(0).ch = %d, out_dim.ch = %d, groups = %d\n", in_channel, out_channel, x, y, in_dim(0).ch, out_dim.ch, groups);
    // Expected format: Kernel in order (ch_in, ch_out, x); x: coeff offset within kernel
    // grouped conv: each output channel only depends on a subset of input channels
    // memory layout: kernel[in_group_len][out_dim.ch][kernel_length]
    // out_dim.ch and in_dim.ch are divisable by groups
    // normal (groups = 1) and depthwise (groups = out_dim.ch = in_dim.ch) are corner cases of grouped conv
    int in_group_len = in_dim(0).ch / groups; // input channels per group
    int out_group_len = out_dim.ch / groups; // output channels per group
    int group = out_channel / out_group_len;
    assert(in_channel / in_group_len == group && "grouped conv: output channel does not depend on requested input channel");

    int in_offs = in_channel % in_group_len; // channel offset within input group
    return getWeightsMMAddr() + sizeof(weight_t) * (x + kernel_length * (out_channel + out_dim.ch * in_offs));
  }

  virtual std::string getLayerInfoText() {
    std::stringstream ss;
    ss << Conv::getLayerInfoText();
    ss << "  kernel " << kernel_length << ", stride " << stride << ", groups " << groups << "\n";
    return ss.str();
  }
  
  // FIXME no padding?
  virtual void computeInputPadding() {}
  virtual void computeDmaPadding() {}

  /*============================================ Segmentation ===========================================================*/
  virtual void setSegmentDimensions() {
    // number of weights required per segment for this layer
    int n_in_channels = (load_weights_at_once) ? in_dim(0).ch : 1;
    int n_weights = n_in_channels * kernel_length + int(use_bias);
    // local memory is partitioned in two parts for double buffering
    int lm_free_entries = VPRO_CFG::LM_SIZE / 2 - VPRO_CFG::LANES * n_weights;
    
    // number of parameters required for activation function, these are directly loaded into the RF via immediates
    int n_act_params = (activation==LEAKY || activation==RELU6) ? 1 : 0;
    int rf_free_entries = RF_DISCARD_ADDR - n_weights - n_act_params;

    // maximum segment length due to number of free entries in LM/RF
    int seg_len = std::min({lm_free_entries, rf_free_entries, in_dim(0).x});

    // Determine minimal number of required segments
    seg.num.x = ceil_div(in_dim(0).x, seg_len);
    seg.num.y = 1;

    seg.in.w = seg_len;
    seg.in.h = 1;
    seg.out.w = seg_len;
    seg.out.h = 1;

    seg.in.x_stride = seg.out.w * stride;
  }

  /*============================================ Command generation =====================================================*/

  virtual BIF::COMMAND_SEGMENT convVPRO(const CNN_LAYER::SEGMENT &segment, BUFFER &buffer, uint32_t lane_mask, BIF::COMMAND_VPRO &mem_layout) {
    assert(result_shift_right >= 0 && "Kernel to be shifted negative! -- not implemented");
    
    BIF::COMMAND_SEGMENT cmd;
    int lm_partition_size = VPRO_CFG::LM_SIZE / 2; // size of double buffering partitions
    int lm_partition_end = (int(buffer) + 1) * lm_partition_size;
    int n_ch = (load_weights_at_once) ? in_dim(0).ch : 1; // number of channels to load at once
    int ch_off = (load_weights_at_once) ? segment.in_channel : 0; // offset for the current channel
    
    cmd.type.type = VPRO_CMD;
    cmd.vpro.command = (segment.isFirst) ? VPRO_TYPE::conv1d_start : VPRO_TYPE::conv1d_add;
    cmd.vpro.rf_base = 0;
    cmd.vpro.lm_base = (int(buffer) * lm_partition_size);
    cmd.vpro.in_ch_offset = ch_off;
    cmd.vpro.zend = seg.out.w - 1;
    cmd.vpro.lane = lane_mask;
    cmd.vpro.kernel_load_buffer_l0 = lm_partition_end -     (n_ch - ch_off) * kernel_x;
    cmd.vpro.kernel_load_buffer_l1 = lm_partition_end - 2 * (n_ch - ch_off) * kernel_x;
    if (segment.isFirst) { // bias load
        cmd.vpro.bias_load_buffer_l0 = cmd.vpro.kernel_load_buffer_l1 - 1;
        cmd.vpro.bias_load_buffer_l1 = cmd.vpro.kernel_load_buffer_l1 - 2;
    }

    // memory layout produced by conv
    mem_layout.type = VPRO_CMD;
    mem_layout.lane_mask = lane_mask;
    mem_layout.xend = 0;
    mem_layout.yend = 0;
    mem_layout.zend = cmd.vpro.zend;
    mem_layout.rf_ch_stride = 1;
    mem_layout.rf_base = 0;
    mem_layout.lm_ch_stride = 1;
    mem_layout.lm_base = cmd.vpro.buffer;

    mem_layout.shift_right = store_shift_right;
    mem_layout.rf_frac_bits = rf_frac_bits; // required for activation functions

    return cmd;
  }

  DMA_COMMANDS::DMA_DESCRIPTOR biasLoad(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER buffer) {
    // determine which half of the local memory is currently used for DMA transfers during double buffering
    DMA_COMMANDS::DMA_DESCRIPTOR dma;
    uint32_t lm_partition_end = (int(buffer) + 1) * (VPRO_CFG::LM_SIZE / 2);
    int n_in_channels = (load_weights_at_once) ? in_dim(0).ch : 1;

    dma.dir = e2l1D;
    dma.cluster = cluster;
    dma.unit = unit;
    dma.lm_addr = lm_partition_end - 2 * n_in_channels * kernel_length - 1 - lane;
    dma.isMM_Bias_offset = true;
    dma.mm_addr = (uint64_t) getBiasMMAddr(segment.out_channel);
    dma.word_count = 1;
    dma.y_size = 1;
    dma.y_leap = 0;
    assert(uint64_t(uint32_t(dma.mm_addr)) == dma.mm_addr && "Kernel offset exceeds 32 bit!");
    return dma;
  }

  DMA_COMMANDS::DMA_DESCRIPTOR kernelLoad(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER buffer) {
    // Use 2D DMA transfer to load all kernel weights for all input channels at once.
    // The kernel has format [in_ch, out_ch, x] in row-major order.
    uint32_t lm_partition_end = (int(buffer) + 1) * (VPRO_CFG::LM_SIZE / 2);
    int n_in_channels = (load_weights_at_once) ? in_dim(0).ch : 1;
    DMA_COMMANDS::DMA_DESCRIPTOR dma;

    dma.dir = e2l2D;
    dma.cluster = cluster;
    dma.unit = unit;
    dma.lm_addr = lm_partition_end - n_in_channels * (lane + 1) * kernel_length;
    dma.x_size = kernel_length;
    dma.y_size = n_in_channels;
    dma.y_leap = out_dim.ch * kernel_length;
    dma.isMM_Kernel_offset = true;
    dma.mm_addr = (uint64_t) getKernelMMAddr(segment.in_channel, segment.out_channel);
    assert(uint64_t(uint32_t(dma.mm_addr)) == dma.mm_addr && "Kernel offset exceeds 32 bit!");
    return dma;
  }

  virtual void load(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer) {
    std::vector<DMA_COMMANDS::DMA_DESCRIPTOR> dmas_1d, dmas_2d;
    dmas_1d.reserve(2 * VPRO_CFG::parallel_Lanes); // kernel and bias load for each lane

    unsigned int cl=0, un=0, ln=0; // cluster, unit and lane of segment
    for (unsigned int i = 0; i < VPRO_CFG::parallel_Lanes; i++) {
        SEGMENT &segment = *segments[i+seg_cnt];
        if (!segment.dummy) {
            // Load coefficients for input and output of this segment to respective hardware element
            if (segment.isFirst || !load_weights_at_once) {
                dmas_2d.push_back(kernelLoad(segment, cl, un, ln, buffer));
            }
            if (segment.isFirst) {
                dmas_1d.push_back(biasLoad(segment, cl, un, ln, buffer));
            }
            // Load input data
            if (ln == 0) {
                dmas_1d.push_back(dataLoad1D(segment, cl, un, buffer));
            }
        }
        next_hardware_element(cl, un, ln);
    }

    auto dma_commands = DMA_COMMANDS::DMA_DESCRIPTOR::startBroadcastLoad(dmas_1d, dmas_2d);
    cmd_cnt.dma += (int) dma_commands.size();
    commands.insert(std::end(commands), std::begin(dma_commands), std::end(dma_commands));
  }

  virtual BIF::COMMAND_SEGMENT dataStore(const CNN_LAYER::SEGMENT &segment, int cluster, int unit, int lane, BUFFER &buffer_load) {
      return dataStore1D(segment, cluster, unit, lane, buffer_load);
  }

}; // class Conv1D


} // namespace CNN_LAYER

#endif // CONV1D_H
