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

// HW parameters taken from ISS
#include "vpro_globals.h"
#include "vpro_cmd_defs.h"
#include "CommandDMA.h"
#include "DMACommandExtensions/DmaLoopExtension.h"
#include "DMACommandExtensions/DmaMerger.h"
#include "DMACommandExtensions/DMAStoreSplitter.h"
#include "Base/base_layer.h"
#include "DMACommandExtensions/DmaClusterMixer.h"

namespace CNN_LAYER {

    void Layer::paddedSegmentToDma(const SEGMENT &segment, DMA_COMMANDS::DMA_DESCRIPTOR &dma, int source) {
        // padding concept
        // segment geometry and start address unaffected by padding; think: HW returns zeros instead of data from MM in padding regions
        // HW DMA unit wants a different format: MM-address counter does not increment in padding regions
        // -> translation required

        dma.pad[CommandDMA::PAD::TOP] = segment.pad_top; // switch padding on/off per segment
        dma.pad[CommandDMA::PAD::RIGHT] = segment.pad_right;
        dma.pad[CommandDMA::PAD::BOTTOM] = segment.pad_bottom;
        dma.pad[CommandDMA::PAD::LEFT] = segment.pad_left;

        // padding sanity checks
        int pad_x = 0;
        if (segment.pad_left)
          pad_x += padding.dma.left;
        if (segment.pad_right)
          pad_x += padding.dma.right;
        assert(pad_x <= (int)dma.x_size && "Requested more horizontal padding than transfer size x");
        
        int pad_y = 0;
        if (segment.pad_top)
          pad_y += padding.dma.top;
        if (segment.pad_bottom)
          pad_y += padding.dma.bottom;
        assert(pad_y <= (int)dma.y_size && "Requested more vertical padding than transfer size y");
        
        // convert address of top-left segment element to top-left non-padding element
        dma.mm_addr = segment.in_MM_base[source];
        if (segment.pad_top)
            dma.mm_addr += 2 * segment.in_MM_y_stride[source] * padding.dma.top;
        if (segment.pad_left)
            dma.mm_addr += 2 * padding.dma.left;

        // segment.in_MM_y_stride: MM distance of two pixels in vertical direction (in elements)
        // dma.leap: distance from last non-padding element of line n to the first non-padding element of line n+1
        // assumption: DMA unit has been configured with dma_set_pad_widths(padding.dma)
        int leap = segment.in_MM_y_stride[source] - ((int)dma.x_size - pad_x) + 1; // signed required
        dma.y_leap = leap;

        if (dma.y_size > 1) // only relevant if more than 1 row is trasferred
          assert(leap >= 0 && "DMA unit uses unsigned leap, backwards overlapping transfers unsupported. Unpadded row N+1 can not start before the end of row N");

    }

    DMA_COMMANDS::DMA_DESCRIPTOR Layer::dataLoad(const SEGMENT &segment, int cluster, int unit, BUFFER &buffer, int source/*=0*/) {
        uint32_t lm_offset = (int(buffer) * (VPRO_CFG::LM_SIZE / 2));
        DMA_COMMANDS::DMA_DESCRIPTOR dma;
        dma.dir = e2l2D;
        dma.cluster = cluster;
        dma.unit = unit;
        dma.x_size = seg.in.w; // dma output size including padding
        dma.y_size = seg.in.h;
        dma.lm_addr = lm_offset + source * (seg.in.w * seg.in.h);

        paddedSegmentToDma(segment, dma, source);

        // printf("dma seg.in.w %d, y_stride %d -> y_leap %d\n", seg.in.w, segment.in_MM_y_stride[source], dma.y_leap);
        return dma;
    }

    DMA_COMMANDS::DMA_DESCRIPTOR Layer::dataLoad1D(const SEGMENT &segment, int cluster, int unit, BUFFER &buffer, int source/*=0*/) {

        assert(source==0);
        uint32_t lm_offset = (int(buffer) * (VPRO_CFG::LM_SIZE / 2));
        DMA_COMMANDS::DMA_DESCRIPTOR dma;
        dma.dir = e2l1D;
        dma.cluster = cluster;
        dma.unit = unit;
        dma.word_count = seg.in.w;
        dma.y_size = 1;
        dma.y_leap = 0;
        dma.mm_addr = segment.in_MM_base[0];
        dma.lm_addr = lm_offset;
        return dma;
    }

    BIF::COMMAND_SEGMENT Layer::dataStore2D(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER &buffer_load) {

        BIF::COMMAND_SEGMENT cmd;
        cmd.type.type = DMA_CMD;
        cmd.dma.direction = l2e2D;
        cmd.dma.cluster = cluster;
        cmd.dma.unit_mask = 1 << unit;
        cmd.dma.mm_addr = segment.out_MM_base;
        cmd.dma.lm_addr = (int(buffer_load) * VPRO_CFG::LM_SIZE / 2) + lane * lm_lane_stride + VPRO_CFG::LM_SIZE / 4;
        cmd.dma.x_size = seg.out.w;
        cmd.dma.y_size = seg.out.h;
        cmd.dma.y_leap = segment.out_MM_y_stride - cmd.dma.x_size + 1;

        // cmd.word_count, pad not used!
        return cmd;
    }

    BIF::COMMAND_SEGMENT Layer::dataStore1D(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER &buffer_load) {

        BIF::COMMAND_SEGMENT cmd;
        cmd.type.type = DMA_CMD;
        cmd.dma.direction = l2e2D;
        cmd.dma.cluster = cluster;
        cmd.dma.unit_mask = 1 << unit;
        cmd.dma.mm_addr = segment.out_MM_base;
        cmd.dma.lm_addr = (int(buffer_load) * VPRO_CFG::LM_SIZE / 2) + lane * lm_lane_stride + VPRO_CFG::LM_SIZE / 4;
        cmd.dma.y_leap = 0;
        cmd.dma.x_size = seg.out.w;
        cmd.dma.y_size = 1;

        // The last segment in x direction may contain invalid elements,
        // thus reduce dma.x_size by number of invalid elements
        if (segment.x_seg == seg.num.x - 1) {
            cmd.dma.x_size -= seg.out.w * seg.num.x - out_dim.x;
        }
        return cmd;
    }

    void Layer::store(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer) {
        unsigned int cl=0, un=0, ln=0; // cluster, unit and lane of segment
        for (unsigned i = 0; i < VPRO_CFG::parallel_Lanes; i++) {
            SEGMENT &segment = *segments[i + seg_cnt];
            if (!segment.dummy && segment.isLast) {
                cmd_cnt.dma++;
                commands.push_back(dataStore(segment, cl, un, ln, buffer));
            }
            next_hardware_element(cl, un, ln);
        }
    }

    void Layer::generateCommands() {
        // This function defines the default behavior for generating commands with double buffering.
        // Everything specific to the layer type should be implemented in the load(), compute() and store() functions.
        cmd_cnt = CMD_COUNT{};

        BUFFER buffer_load = BUFFER::A, buffer_calc = BUFFER::A, buffer_store = BUFFER::A;

        /*** first load ***/
        unsigned int seg_size = segments.size();
        unsigned int curr_segments = 0;
        unsigned int next_segments;
        load(segments, curr_segments, buffer_load);
        buffer_load = (buffer_load == A) ? B : A;
        commands.push_back(DMA_COMMANDS::DMA_DESCRIPTOR::wait());
        cmd_cnt.sync++;

        /*** loop over all segments ***/
        for (; curr_segments < seg_size - VPRO_CFG::parallel_Lanes; curr_segments += VPRO_CFG::parallel_Lanes) {
            next_segments = curr_segments + VPRO_CFG::parallel_Lanes;
            load(segments, next_segments, buffer_load);       // load next segments
            compute(segments, curr_segments, buffer_calc, buffer_store);    // compute current segments
            commands.push_back(VPRO_COMMANDS::sync());
            cmd_cnt.sync++;

            store(segments, curr_segments, buffer_store);
            buffer_load = (buffer_load == A) ? B : A;
            buffer_calc = (buffer_calc == A) ? B : A;
        }

        /*** Remaining Segments ***/
        // last segment is loaded but not yet executed or stored;
        compute(segments, curr_segments, buffer_calc, buffer_store);
        commands.push_back(VPRO_COMMANDS::sync());
        cmd_cnt.sync++;

        store(segments, curr_segments, buffer_store);
        commands.push_back(VPRO_COMMANDS::sync());
        cmd_cnt.sync++;
    }

    void Layer::compressCommands() {
        // apply interleaving / dma extension to command list if necessary
        assert(!(layercfg.use_dma_extension && layercfg.use_dma_interleaver) && "DMA extension and interleaving are not compatible with each other!");
        assert((layercfg.use_dma_extension || layercfg.use_dma_interleaver) && "DMA extension or interleaving must be active to generate correct dma cluster masks!");

        printf("  [Segment Mapping | Unit Broadcasting] %lu Commands (double buffering: DMA/VPRO/Sync)\n", commands.size());

        if (layercfg.use_dma_merger) {
            auto merger = DmaMerger(commands);
            commands = merger.generate();
        }
        if (layercfg.use_dma_interleaver) {
            auto il = SegmentInterleaver(commands); // deprecated
            commands = il.interleave();
        }
        if (layercfg.use_dma_extension) {
            auto ext = DmaBlockExtension(commands); // better than interleaver; one of them required to translate ids into masks
            commands = ext.generate();

            if (layercfg.use_dma_merger) {
                auto merger = DmaMerger(commands);
                commands = merger.generate();
            }

            if (layercfg.use_dma_store_splitter) {
                auto splitter = DmaStoreSpliiter(commands);
                commands = splitter.generate();
            }

            if (layercfg.use_dma_loop_extension) {
              auto ext2 = DmaLoopExtension(commands);
              commands = ext2.generate();
            }
        }

        if (layercfg.use_dma_l2e_mix_extension) {
            auto ext = DmaClusterMixer(commands);
            commands = ext.generate();
        }

    }

} // namespace CNN_LAYER
