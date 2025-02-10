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
#include "pooling_layer.h"
#include "bif.h"

namespace CNN_LAYER {

DMA_COMMANDS::DMA_DESCRIPTOR AveragePooling2D::kernelLoad(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER buffer) {

    uint32_t lm_offset = (int(buffer) * (VPRO_CFG::LM_SIZE / 2));

    DMA_COMMANDS::DMA_DESCRIPTOR dma;
    dma.dir = e2l2D;
    dma.cluster = cluster;
    dma.unit = unit;
    dma.lm_addr = lm_offset + VPRO_CFG::LM_SIZE / 4 - (seg.out.w * seg.out.h * (lane + 1));
    dma.word_count = seg.out.w * seg.out.h;
    dma.x_size = seg.out.w;
    dma.y_size = seg.out.h;    
    dma.y_leap = out_dim.x - seg.out.w + 1; // FIXME what is y_leap?
    // dma.isMM_Kernel_offset = true; // FIXME what is kernel_offset
    dma.mm_addr = (uint64_t) getKernelMMAddr(segment.x_seg * seg.out.w, segment.y_seg * seg.out.h);
    // #ifdef COMPARABLE_CMDS
    //     dma.mm_addr -= getWeightsMMAddr(); // relative address
    // #endif
    // assert(uint64_t(uint32_t(dma.mm_addr)) == dma.mm_addr && "Kernel offset exceeds 32 bit!");
    return dma;
}

DMA_COMMANDS::DMA_DESCRIPTOR AveragePooling2D::dataLoad(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER buffer) {
    uint32_t lm_offset = (int(buffer) * (VPRO_CFG::LM_SIZE / 2));
    
    DMA_COMMANDS::DMA_DESCRIPTOR dma;
    dma.dir = e2l2D;
    dma.cluster = cluster;
    dma.unit = unit;
    dma.lm_addr = lm_offset + (seg.in.w * seg.in.h * lane);
    dma.word_count = seg.in.w * seg.in.h;
    dma.x_size = seg.in.w; // dma output size including padding
    dma.y_size = seg.in.h;
    paddedSegmentToDma(segment, dma, 0);

    // printf("dma seg.in.w %d, y_stride %d -> y_leap %d\n", seg.in.w, segment.in_MM_y_stride[source], dma.y_leap);
    return dma;
}

void AveragePooling2D::load(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer) {

    std::vector<DMA_COMMANDS::DMA_DESCRIPTOR> dmas_1d, dmas_2d;
    dmas_2d.reserve(2 * VPRO_CFG::parallel_Lanes); // kernel and data load for each lane

    unsigned int cl=0, un=0, ln=0; // cluster, unit and lane of segment
    for (unsigned int i = 0; i < VPRO_CFG::parallel_Lanes; i++) {
        SEGMENT &segment = *segments[i+seg_cnt];
        if (!segment.dummy) {
            // Load coefficients for input and output of this segment to respective hardware element
            dmas_2d.push_back(kernelLoad(segment, cl, un, ln, buffer));
            dmas_2d.push_back(dataLoad(segment, cl, un, ln, buffer));
        }
        next_hardware_element(cl, un, ln);
    }

    auto dma_commands = DMA_COMMANDS::DMA_DESCRIPTOR::startBroadcastLoad(dmas_1d, dmas_2d);
    cmd_cnt.dma += (int) dma_commands.size();
    commands.insert(std::end(commands), std::begin(dma_commands), std::end(dma_commands));
}

void AveragePooling2D::compute(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer, BUFFER &store_buffer) {
    // segments[seg_cnt]: 1st segment within set (i.e. cluster 0, unit 0, lane 0)

    SEGMENT &segment = *segments[seg_cnt];
    if (!segment.dummy) { // assumption: if the batch contains non-dummy segments, the 1st segment is no dummy
        // following commands are broadcasted
        cmd_cnt.vpro++;
        commands.push_back(load_pool_store(segment, buffer, store_buffer));
    }
}

BIF::COMMAND_SEGMENT AveragePooling2D::load_pool_store(const CNN_LAYER::SEGMENT &segment, BUFFER &buffer, BUFFER &store_buffer) {

    BIF::COMMAND_SEGMENT cmd;
    cmd.type.type = VPRO_CMD;
    cmd.vpro.command = VPRO_TYPE::avgpool2d;
    cmd.vpro.buffer = (int(buffer) * VPRO_CFG::LM_SIZE / 2);   // LOAD / Pool
    // calculate results to a new buffer (switch store buffer)
    store_buffer = (store_buffer == A) ? B : A;
    cmd.vpro.offset = (int(store_buffer) * VPRO_CFG::LM_SIZE / 2) + VPRO_CFG::LM_SIZE / 4; // STORE
    cmd.vpro.xend = seg.out.w; // FIXME missing -1?
    cmd.vpro.yend = seg.out.h;
    cmd.vpro.lane = 0;

    // kernel load
    cmd.vpro.kernel_load_buffer_l0 = (int(buffer) * VPRO_CFG::LM_SIZE / 2) + VPRO_CFG::LM_SIZE / 4 - (seg.out.w * seg.out.h * (0 + 1));
    cmd.vpro.kernel_load_buffer_l1 = (int(buffer) * VPRO_CFG::LM_SIZE / 2) + VPRO_CFG::LM_SIZE / 4 - (seg.out.w * seg.out.h * (1 + 1));

    return cmd;
}

} // namespace CNN_LAYER
