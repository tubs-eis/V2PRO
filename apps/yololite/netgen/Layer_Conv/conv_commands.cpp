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

#include "conv_layer.h"
#include "bif.h"

// legacy reference uses relative mm_addr; enable diff between legacy and netgen generated commands.txt
//#define COMPARABLE_CMDS

namespace CNN_LAYER {

    BIF::COMMAND_SEGMENT Conv::convVPRO(const CNN_LAYER::SEGMENT &segment, BUFFER &buffer, uint32_t lane_mask, BIF::COMMAND_VPRO &mem_layout) {

        assert(result_shift_right >= 0 && "Kernel to be shifted negative! -- not implemented");

        int lm_dbsz = VPRO_CFG::LM_SIZE/2;
        
        BIF::COMMAND_SEGMENT cmd;
        cmd.type.type = VPRO_CMD;
        cmd.vpro.command = (segment.isFirst) ? VPRO_TYPE::conv_start : VPRO_TYPE::conv_add;
        cmd.vpro.buffer = int(buffer) * lm_dbsz;
        cmd.vpro.lane = lane_mask;
        assert(L0 == 1 && L1 == 2 && "runtime assumes LANE to be a bit mask");

        if(kernel_length == 1 && pool_size[0] == 1 && stride == 1 && groups == 1 && parallel_outchannels_per_lane > 1 && upsampling_scale == 1) {
            // kernel load
            cmd.vpro.kernel_load_buffer_l0 = cmd.vpro.buffer + lm_dbsz / 2 - (parallel_outchannels_per_lane * 2);  // lower part of LM -> L0
            cmd.vpro.kernel_load_buffer_l1 = cmd.vpro.buffer + lm_dbsz / 2 - (parallel_outchannels_per_lane * 1);

            if (segment.isFirst) {
                // bias load
                cmd.vpro.bias_load_buffer_l0 = cmd.vpro.buffer + lm_dbsz / 2 - (parallel_outchannels_per_lane * 4);
                cmd.vpro.bias_load_buffer_l1 = cmd.vpro.buffer + lm_dbsz / 2 - (parallel_outchannels_per_lane * 3);
            }
        } else {
            // kernel load
            cmd.vpro.kernel_load_buffer_l0 = cmd.vpro.buffer + lm_dbsz / 2 - (kernel_x * kernel_y * parallel_outchannels_per_lane * (0 + 1));
            cmd.vpro.kernel_load_buffer_l1 = cmd.vpro.buffer + lm_dbsz / 2 - (kernel_x * kernel_y * parallel_outchannels_per_lane * (1 + 1));

            if (segment.isFirst) {
                // bias load
                cmd.vpro.bias_load_buffer_l0 = cmd.vpro.buffer + lm_dbsz / 2 - (kernel_x * kernel_y * parallel_outchannels_per_lane * 2) - 1 * parallel_outchannels_per_lane - 0;
                cmd.vpro.bias_load_buffer_l1 = cmd.vpro.buffer + lm_dbsz / 2 - (kernel_x * kernel_y * parallel_outchannels_per_lane * 2) - 1 * parallel_outchannels_per_lane - 1 * parallel_outchannels_per_lane;
            }
        }


        // memory layout produced by conv
        mem_layout.type = VPRO_CMD;
        mem_layout.lane_mask = lane_mask;
        mem_layout.xend = conv_seg_w - 1;
        mem_layout.yend = conv_seg_h - 1;
        mem_layout.zend = parallel_outchannels_per_lane - 1;
        mem_layout.rf_ch_stride = parallel_outchannels_per_lane > 1 ? conv_seg_w * conv_seg_h : 0;
        mem_layout.rf_base = 0;
        mem_layout.lm_ch_stride = mem_layout.rf_ch_stride;

        mem_layout.lm_base = cmd.vpro.buffer; // This is the Input Buffer -> still needed in swish (LM temp region) -> Store is defined by store_buffer in VPRO Store command generation

        mem_layout.shift_right = store_shift_right;
        mem_layout.rf_frac_bits = rf_frac_bits;

        return cmd;
    }

    BIF::COMMAND_SEGMENT Conv2D::dataStore(const CNN_LAYER::SEGMENT &segment, int cluster, int unit, int lane, BUFFER &buffer_load) {
        auto cmd = FusedFunc::dataStore2D(segment, cluster, unit, lane, buffer_load);

        if (kernel_length == 1 && pool_size[0] == 1 && stride == 1 && groups == 1 && parallel_outchannels_per_lane > 1 && upsampling_scale == 1) {
            cmd.dma.y_leap = 1;  // 1D transfers or fake 2D without leap (DMA Merger will translate to 1D transfers)

            if( segment.x_seg == seg.num.x - 1){
                cmd.dma.skipped_elements_at_end = overcalc_elements_1d; // eval in dma merge (2d needs to be merged to 1d before subtracting this from x_size)
                if (overcalc_elements_1d != 0)
                  assert(layercfg.use_dma_merger == true);  // dma merger is needed for overcalc correction!
            }
        }

        return cmd;
    }

    DMA_COMMANDS::DMA_DESCRIPTOR Conv2D::biasLoad(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER buffer) {

        // determine which half of the local memory is currently used for DMA transfers during double buffering
        uint32_t lm_offset = (int(buffer) * (VPRO_CFG::LM_SIZE / 2));

        DMA_COMMANDS::DMA_DESCRIPTOR dma;
        dma.dir = e2l1D;
        dma.cluster = cluster;
        dma.unit = unit;
        dma.lm_addr = lm_offset + VPRO_CFG::LM_SIZE / 4 - 2 * kernel_x * kernel_y - 1 - lane;
        dma.isMM_Bias_offset = true;
        dma.mm_addr = (uint64_t) getBiasMMAddr(segment.out_channel);
#ifdef COMPARABLE_CMDS
        dma.mm_addr -= getWeightsMMAddr(); // relative address
#endif
        dma.word_count = 1;
        dma.y_size = 1;   // UNUSED! (std as on previous calc method, without dcache)
        dma.y_leap = 0; // UNUSED! - 1  = 1 (std as on previous calc method, without dcache) // TODO: CHECK: 1D dont have a stride

        assert(uint64_t(uint32_t(dma.mm_addr)) == dma.mm_addr && "Bias offset exceeds 32 bit!");
        return dma;
    }

    DMA_COMMANDS::DMA_DESCRIPTOR Conv2D::kernelLoad(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER buffer) {

        uint32_t lm_offset = (int(buffer) * (VPRO_CFG::LM_SIZE / 2));

        DMA_COMMANDS::DMA_DESCRIPTOR dma;
        dma.dir = e2l1D;
        dma.cluster = cluster;
        dma.unit = unit;
        dma.lm_addr = lm_offset + VPRO_CFG::LM_SIZE / 4 - (kernel_x * kernel_y * (lane + 1));
        dma.word_count = kernel_x * kernel_y;
        dma.y_size = 1;     // UNUSED! (std as on previous calc method, without dcache)
        dma.y_leap = 0; // 2  // UNUSED! - 1  = 1 (std as on previous calc method, without dcache) // TODO: CHECK: 1D dont have a stride
        dma.isMM_Kernel_offset = true;
        dma.mm_addr = (uint64_t) getKernelMMAddr(segment.in_channel, segment.out_channel);
#ifdef COMPARABLE_CMDS
        dma.mm_addr -= getWeightsMMAddr(); // relative address
#endif
        assert(uint64_t(uint32_t(dma.mm_addr)) == dma.mm_addr && "Kernel offset exceeds 32 bit!");

        return dma;
    }

    void Conv2D::generateCommands() {
        if (kernel_length == 1 && pool_size[0] == 1 && stride == 1 && groups == 1 && parallel_outchannels_per_lane > 1 && upsampling_scale == 1) {
            // former globals from vpro_functions.h, now member variables
            RF_KERNEL_BASE = RF_DISCARD_ADDR - (kernel_length * parallel_outchannels_per_lane);
            RF_BIAS_BASE = RF_KERNEL_BASE - parallel_outchannels_per_lane;
            RF_RELU_6_BASE = RF_BIAS_BASE - 1;
            kernel_x = kernel_length;
            kernel_y = kernel_length;
            // This function defines the default behavior for generating commands with double buffering.
            // Everything specific to the layer type should be implemented in the load(), compute() and store() functions.
            cmd_cnt = CMD_COUNT{};

            BUFFER buffer_load = BUFFER::A, buffer_calc = BUFFER::A, buffer_store = BUFFER::A;
            unsigned int seg_size = segments.size();
            /*** first load ***/
            unsigned int curr_segments = 0;
            unsigned int next_segments;
            load(segments, curr_segments, buffer_load);
            buffer_load = (buffer_load == A) ? B : A;
            commands.push_back(DMA_COMMANDS::DMA_DESCRIPTOR::wait());
            cmd_cnt.sync++;

            /*** loop over all segments ***/
            for (; curr_segments < seg_size - VPRO_CFG::parallel_Lanes * parallel_outchannels_per_lane; curr_segments += VPRO_CFG::parallel_Lanes * parallel_outchannels_per_lane) {
                next_segments = curr_segments + VPRO_CFG::parallel_Lanes * parallel_outchannels_per_lane;
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

        } else {
            Conv::generateCommands();
        }
    }

    void Conv2D::load(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer) {
        if (kernel_length == 1 && pool_size[0] == 1 && stride == 1 && groups == 1 && parallel_outchannels_per_lane > 1 && upsampling_scale == 1) {
            assert(parallel_outchannels_per_lane >= 1);
            assert(parallel_inchannels_per_lane == 1); // TODO
            std::vector<DMA_COMMANDS::DMA_DESCRIPTOR> dmas_1d, dmas_2d;
            dmas_1d.reserve(2 * parallel_outchannels_per_lane * VPRO_CFG::parallel_Lanes); // kernel and bias load for each lane

            unsigned int cl = 0, un = 0, ln = 0; // cluster, unit and lane of segment
            for (unsigned int lane = 0; lane < VPRO_CFG::parallel_Lanes; lane += 1) {
                for (int n_iteration = 0; n_iteration < parallel_outchannels_per_lane; ++n_iteration) {
                    SEGMENT &segment = *segments[lane * parallel_outchannels_per_lane + seg_cnt + n_iteration];
                    if (!segment.dummy) {
                        // Load coefficients for input and output of this segment to respective hardware element
                        /**
                         * [LM]
                         *  n * conv_seg_w (output blocks)
                         *
                         *  end - 2*n - 2*n -> Bias (n * L0)(n * L1)
                         *  end - 2*n -> Kernels (n * L0)(n * L1)
                         *  end = 4096 / 8192
                         */
//                        printf("Kernel to Lane %3i [n %i]: in %i, out %i, mm %lu\n", lane, n_iteration, segment.in_channel, segment.out_channel, dma.mm_addr);
                        auto dma = kernelLoad(segment, cl, un, ln, buffer);
                        dma.lm_addr = (int(buffer) * (VPRO_CFG::LM_SIZE / 2)) + // buffer offset
                                      VPRO_CFG::LM_SIZE / 4 - 2 * parallel_outchannels_per_lane + n_iteration + parallel_outchannels_per_lane*ln;   // end of lm
                        dmas_1d.push_back(dma);

                        if (segment.isFirst) {
                            dma = biasLoad(segment, cl, un, ln, buffer);
                            dma.lm_addr =  (int(buffer) * (VPRO_CFG::LM_SIZE / 2)) + // buffer offset
                                          VPRO_CFG::LM_SIZE / 4 - 2 * parallel_outchannels_per_lane - 2 * parallel_outchannels_per_lane + n_iteration + parallel_outchannels_per_lane*ln;
                            dmas_1d.push_back(dma);
                        }

                        // Load input data // TODO: m > 1
                        if (ln == 0) {
                            DMA_COMMANDS::DMA_DESCRIPTOR dma;
                            dma.dir = e2l2D;
                            dma.cluster = cl;
                            dma.unit = un;
                            dma.x_size = seg.in.w; // dma output size including padding
                            dma.y_size = seg.in.h;
                            dma.mm_addr = segment.in_MM_base[0];
                            dma.lm_addr = (int(buffer) * (VPRO_CFG::LM_SIZE / 2));
                            dma.y_leap = 1; // fake 1d -> DMA Merger will translate to 1D transfers
                            dmas_2d.push_back(dma);
//                            dmas_1d.push_back(dataLoad1D(segment, cl, un, buffer));
                        }
                    }
                }
                next_hardware_element(cl, un, ln);
            }

            auto dma_commands = DMA_COMMANDS::DMA_DESCRIPTOR::startBroadcastLoad(dmas_1d, dmas_2d);
            cmd_cnt.dma += (int) dma_commands.size();
            commands.insert(std::end(commands), std::begin(dma_commands), std::end(dma_commands));

            return;
        }   // special 1D case

        std::vector<DMA_COMMANDS::DMA_DESCRIPTOR> dmas_1d, dmas_2d;
        dmas_1d.reserve(2 * VPRO_CFG::parallel_Lanes); // kernel and bias load for each lane

        unsigned int cl = 0, un = 0, ln = 0; // cluster, unit and lane of segment
        for (unsigned int i = 0; i < VPRO_CFG::parallel_Lanes; i++) {
            SEGMENT &segment = *segments[i + seg_cnt];
            if (!segment.dummy) {
                // Load coefficients for input and output of this segment to respective hardware element
                dmas_1d.push_back(kernelLoad(segment, cl, un, ln, buffer));
                if (segment.isFirst) {
                    dmas_1d.push_back(biasLoad(segment, cl, un, ln, buffer));
                }

                // Load input data
                if (ln == 0) {
                    dmas_2d.push_back(dataLoad(segment, cl, un, buffer));
                }
            }
            next_hardware_element(cl, un, ln);
        }

        auto dma_commands = DMA_COMMANDS::DMA_DESCRIPTOR::startBroadcastLoad(dmas_1d, dmas_2d);
        cmd_cnt.dma += (int) dma_commands.size();
        commands.insert(std::end(commands), std::begin(dma_commands), std::end(dma_commands));
    }

    void Conv2D::store(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer) {
        // FIXME move to Layer::store()
        if (kernel_length == 1 && pool_size[0] == 1 && stride == 1 && groups == 1 && parallel_outchannels_per_lane > 1 && upsampling_scale == 1) {
            assert(parallel_outchannels_per_lane >= 1);
            assert(parallel_inchannels_per_lane == 1); // TODO
            unsigned int cl = 0, un = 0, ln = 0; // cluster, unit and lane of segment
            for (unsigned lane = 0; lane < VPRO_CFG::parallel_Lanes; lane++) {
                for (int n_iterate = 0; n_iterate < parallel_outchannels_per_lane; ++n_iterate) {
                    SEGMENT &segment = *segments[seg_cnt + lane * parallel_outchannels_per_lane + n_iterate];
                    if (!segment.dummy && segment.isLast) {
                        cmd_cnt.dma++;
                        auto dma = dataStore(segment, cl, un, ln, buffer);
                        dma.dma.lm_addr += n_iterate*seg.out.w*seg.out.h;
                        commands.push_back(dma);
                    }
                }
                next_hardware_element(cl, un, ln);
            }
            return;
        }

        Conv::store(segments, seg_cnt, buffer);
    }

    void Conv::compute(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer, BUFFER &store_buffer) {
        // segments[seg_cnt]: 1st segment within set (i.e. cluster 0, unit 0, lane 0)

        // layout: segments[sets][VPRO_CFG::CLUSTERS][VPRO_CFG::UNITS][VPRO_CFG::LANES][parallel_outchannels_per_lane] (flattened)
        
        // use first non-dummy segment as prototype
        SEGMENT *segment;
        unsigned int si = seg_cnt;
        do {
            assert((si < seg_cnt + VPRO_CFG::parallel_Lanes * parallel_outchannels_per_lane) && "only dummy segments in this set (i.e. nothing to do)");
            segment = segments[si];
            si++;
        } while (segment->dummy);

        // which lanes have non-dummy segments?
        uint32_t lane_mask = 0;
        for (unsigned int lane = 0; lane < VPRO_CFG::LANES; lane++) {
            for (uint si = seg_cnt + lane * parallel_outchannels_per_lane; si < seg_cnt + VPRO_CFG::parallel_Lanes * parallel_outchannels_per_lane; si += VPRO_CFG::LANES * parallel_outchannels_per_lane) {
                if (!segments[si]->dummy) {
                    lane_mask |= 1 << lane;
                    break;
                }
            }
        }
        
        // convolution
        BIF::COMMAND_VPRO mem_layout;
        commands.push_back(convVPRO(*segment, buffer, lane_mask, mem_layout));
        cmd_cnt.vpro++;

        // post-processing
        if (segment->isLast) {
            poolActivationVPRO(mem_layout);
            
            // transfer result from RF to LM
            cmd_cnt.vpro++;
            commands.push_back(shiftStoreVPRO(mem_layout, store_buffer));
        }
    }

    void Conv::generateCommands() {

        kernel_x = kernel_length;
        kernel_y = kernel_length;
        RF_KERNEL_BASE = RF_DISCARD_ADDR - (kernel_x * kernel_y);
        RF_BIAS_BASE = RF_KERNEL_BASE - 1;
        RF_RELU_6_BASE = RF_BIAS_BASE - 1;

        FusedFunc::generateCommands();
    }

}; // namespace CNN_LAYER
