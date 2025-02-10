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

#include "elwise_layer.h"
#include "bif.h"

namespace CNN_LAYER {

DMA_COMMANDS::DMA_DESCRIPTOR Elementwise::dataLoad(const SEGMENT &segment, int cluster, int unit, BUFFER &buffer, int source/*=0*/) {
    uint32_t lm_offset = (int(buffer) * (VPRO_CFG::LM_SIZE / 2));
    DMA_COMMANDS::DMA_DESCRIPTOR dma;
    dma.dir = e2l2D;
    dma.cluster = cluster;
    dma.unit = unit;
    dma.x_size = bc_x(source) ? 1 : seg.in.w; // dma output size including padding
    dma.y_size = bc_y(source) ? 1 : seg.in.h;
    dma.lm_addr = lm_offset + source * (seg.in.w * seg.in.h);

    paddedSegmentToDma(segment, dma, source);

    return dma;
}

  
void Elementwise::load(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer) {
    std::vector<DMA_COMMANDS::DMA_DESCRIPTOR> dmas_1d, dmas_2d;

    unsigned int cl=0, un=0, ln=0; // cluster, unit and lane of segment
    for (unsigned int i = 0; i < VPRO_CFG::parallel_Lanes; i++) {
        SEGMENT &segment = *segments[i+seg_cnt];
        if (!segment.dummy) {
            // Load data for both inputs
            dmas_2d.push_back(dataLoad(segment, cl, un, buffer, 0));
            dmas_2d.push_back(dataLoad(segment, cl, un, buffer, 1));
        }
        next_hardware_element(cl, un, ln);
    }
    auto dma_commands = DMA_COMMANDS::DMA_DESCRIPTOR::startBroadcastLoad(dmas_1d, dmas_2d);
    cmd_cnt.dma += (int) dma_commands.size();
    commands.insert(std::end(commands), std::begin(dma_commands), std::end(dma_commands));
}


void Elementwise::compute(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer, BUFFER &store_buffer) {
    SEGMENT &segment = *segments[seg_cnt];
    assert (!segment.dummy);

    BIF::COMMAND_VPRO mem_layout;
    mem_layout.type = VPRO_CMD;
    mem_layout.lane_mask = 1; // apply pooling, activation and shift_store to L0 only
    mem_layout.xend = seg.out.w - 1;
    mem_layout.yend = seg.out.h - 1;
    mem_layout.rf_base = 0;
    mem_layout.lm_base = (int(buffer) * VPRO_CFG::LM_SIZE/2);

    mem_layout.shift_right = store_shift_right;
    mem_layout.rf_frac_bits = rf_frac_bits;
    
    BIF::COMMAND_SEGMENT cmd;
    cmd.vpro = mem_layout;
    cmd.vpro.command = get_vpro_type();
    cmd.vpro.broadcast_map = (bc_ch(1)<<5) | (bc_y(1)<<4) | (bc_x(1)<<3) | (bc_ch(0)<<2) | (bc_y(0)<<1) | bc_x(0);
    commands.push_back(cmd);
    cmd_cnt.vpro++;

    poolActivationVPRO(mem_layout);

    // transfer result from RF to LM
    mem_layout.shift_right = store_shift_right;
    commands.push_back(shiftStoreVPRO(mem_layout, store_buffer));
    cmd_cnt.vpro++;
}

void Elementwise::generateCommands() {
    
    RF_RELU_6_BASE = RF_DISCARD_ADDR - 1;
    FusedFunc::generateCommands();
}

}
