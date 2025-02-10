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
#include "CommandDMA.h"

#include "depth_to_space_layer.h"
#include "bif.h"

namespace CNN_LAYER {


void DepthToSpace::load(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer){
    for(uint32_t ic = 0; ic < (uint32_t)block_size * block_size; ic++){
            for(uint32_t cl = 0; cl < VPRO_CFG::CLUSTERS; cl++){
                
            SEGMENT &segment = *segments[ic * VPRO_CFG::CLUSTERS + cl + seg_cnt];
        
                if (!segment.dummy){

                    BIF::COMMAND_SEGMENT cmd;
                    cmd.type.type = DMA_CMD;
                    cmd.dma.direction = e2l2D;
                    cmd.dma.cluster = 1 << cl;
                    cmd.dma.unit_mask = 1;
                    cmd.dma.mm_addr = segment.in_MM_base[0];
                    cmd.dma.lm_addr = (segment.in_channel % (block_size * block_size)) * seg.in.w * seg.in.h;
                    cmd.dma.x_size = seg.in.w;
                    cmd.dma.y_size = seg.in.h;
                    cmd.dma.y_leap = segment.in_MM_y_stride[0] - cmd.dma.x_size + 1;

                    commands.push_back(cmd);
                }
            } 
    }
}

void DepthToSpace::compute(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer, BUFFER &store_buffer) {
    //SEGMENT &segment = *segments[seg_cnt];
    cmd_cnt.vpro++;
    BIF::COMMAND_SEGMENT cmd;
    cmd.type.type = VPRO_CMD;
    cmd.vpro.command = depth_to_space;
    cmd.vpro.buffer = 0;
    cmd.vpro.xend = 1; // 3
    cmd.vpro.yend = 1; // 3
    commands.push_back(cmd);
}

void DepthToSpace::store(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer){
    for(uint32_t ic = 0; ic < (uint32_t)block_size * block_size; ic++){ 
            for(uint32_t cl = 0; cl < VPRO_CFG::CLUSTERS; cl++){

            SEGMENT &segment = *segments[ic * VPRO_CFG::CLUSTERS + cl + seg_cnt];
                if (!segment.dummy){

                    BIF::COMMAND_SEGMENT cmd;
                    cmd.type.type = DMA_CMD;
                    cmd.dma.direction = l2e2D;
                    cmd.dma.cluster = 1 << cl;
                    cmd.dma.unit_mask = 1;
                    cmd.dma.mm_addr = segment.out_MM_base;
                    cmd.dma.lm_addr = (segment.in_channel % (block_size * block_size)) * seg.in.w * seg.in.h;
                    cmd.dma.x_size = seg.in.w * seg.in.h; // FIXME why not seg.out?
                    cmd.dma.y_size = 1;
                    cmd.dma.y_leap = segment.out_MM_y_stride - cmd.dma.x_size + 1;
                    commands.push_back(cmd);
                }
            } 
    }
}



void DepthToSpace::generateCommands() {
    cmd_cnt = CMD_COUNT{};
    BUFFER buffer_load = BUFFER::A;

    for(uint segment = 0; segment < segments.size(); segment += (VPRO_CFG::CLUSTERS * block_size * block_size)){
        load(segments, segment, buffer_load);
        commands.push_back(DMA_COMMANDS::DMA_DESCRIPTOR::wait());
        cmd_cnt.sync++;

        compute(segments, segment, buffer_load, buffer_load);
        commands.push_back(VPRO_COMMANDS::sync());

        store(segments, segment, buffer_load);
        commands.push_back(DMA_COMMANDS::DMA_DESCRIPTOR::wait());
        cmd_cnt.sync++;
    }
}

void DepthToSpace::compressCommands() {
    // no compression
}


} // namespace CNN_LAYER

