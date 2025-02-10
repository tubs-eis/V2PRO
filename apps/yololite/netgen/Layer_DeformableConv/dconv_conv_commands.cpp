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
#include "vpro_globals.h"
#include "vpro_cmd_defs.h"

#include "dconv_conv_layer.h"
#include "bif.h"

namespace CNN_LAYER {

BIF::COMMAND_SEGMENT DConvConv::convVPRO(const CNN_LAYER::SEGMENT &segment, BUFFER &buffer, uint32_t lane_mask, BIF::COMMAND_VPRO &mem_layout) {
    BIF::COMMAND_SEGMENT cmd = Conv2D::convVPRO(segment, buffer, lane_mask, mem_layout);
    cmd.vpro.command = (segment.isFirst) ? VPRO_TYPE::dconv_conv_start : VPRO_TYPE::dconv_conv_add;

    // remove double buffering optimization
    cmd.vpro.kernel_load_buffer_l0 += VPRO_CFG::LM_SIZE / 4;
    cmd.vpro.kernel_load_buffer_l1 += VPRO_CFG::LM_SIZE / 4;
    cmd.vpro.bias_load_buffer_l0 += VPRO_CFG::LM_SIZE / 4;
    cmd.vpro.bias_load_buffer_l1 += VPRO_CFG::LM_SIZE / 4;

    return cmd;
}

void DConvConv::generateCommands() {

    // former globals from vpro_functions.h, now member variables
    RF_KERNEL_BASE = RF_DISCARD_ADDR - kernel_length;
    RF_BIAS_BASE = RF_KERNEL_BASE - 1;
    RF_RELU_6_BASE = RF_BIAS_BASE - 1;
    kernel_x = kernel_length;
    kernel_y = 1;

    Layer::generateCommands();
}

DMA_COMMANDS::DMA_DESCRIPTOR DConvConv::biasLoad(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER buffer) {
    // remove double buffering optimization
    auto cmd = Conv2D::biasLoad(segment, cluster, unit, lane, buffer);
    cmd.lm_addr += VPRO_CFG::LM_SIZE / 4;
    return cmd;
}
DMA_COMMANDS::DMA_DESCRIPTOR DConvConv::kernelLoad(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER buffer) {
    // remove double buffering optimization
    auto cmd = Conv2D::kernelLoad(segment, cluster, unit, lane, buffer);
    cmd.lm_addr += VPRO_CFG::LM_SIZE / 4;
    return cmd;
}

BIF::COMMAND_SEGMENT DConvConv::dataStore(const CNN_LAYER::SEGMENT &segment, int cluster, int unit, int lane, BUFFER &buffer_load) {
    // remove double buffering optimization
    auto cmd = Conv2D::dataStore(segment, cluster, unit, lane, buffer_load);
    cmd.dma.lm_addr -= VPRO_CFG::LM_SIZE / 4;
    return cmd;
}

BIF::COMMAND_SEGMENT DConvConv::shiftStoreVPRO(BIF::COMMAND_VPRO &mem_layout, BUFFER &store_buffer){
    // remove double buffering optimization
    auto cmd = Conv2D::shiftStoreVPRO(mem_layout, store_buffer);
    cmd.vpro.lm_base -= VPRO_CFG::LM_SIZE / 4;
    return cmd;
}


}; // namespace CNN_LAYER
