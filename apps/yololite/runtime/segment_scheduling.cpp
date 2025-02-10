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
#include "inttypes.h"
#include <algorithm>

#include "segment_scheduling.h"
#include "vpro_functions.h"
#include <vpro.h>
#include "eisv.h"
#include "kernels.h"

#if RV_VPRO_EXT == 1 and not defined(SIMULATION)
#include <vpro/vpro_asm.h>
#include <vpro/dma_asm.h>
using namespace VPRO_RISC_EXT_VPRO;
using namespace VPRO_RISC_EXT_DMA;
#endif

using namespace BIF;

// remove from runtime executable if limited (smaller size)
// improves performance by eliminating code which is not needed by cnn application (if statement doesnt need to be checked)
//#define LIMIT_IMPL

#ifdef LIMIT_IMPL

/*// Mobilenetv2
#define IMPL_MAXPOOL2D         0
#define IMPL_ADD               1
#define IMPL_AVGPOOL2D         0
#define IMPL_CONCAT            0
#define IMPL_CONV1D            0
#define IMPL_CONV2D            1
#define IMPL_CONVTRANS         0
#define IMPL_DCONV             0
#define IMPL_DEPTH2SPACE       0
#define IMPL_DYNAMIC_AXIS      0
#define IMPL_GLOBALAVGPOOL     0
#define IMPL_GLOBALMAXPOOL     0
#define IMPL_LEAKY             0
#define IMPL_MUL               0
#define IMPL_POINTPILLARS      0
#define IMPL_POOL              0
#define IMPL_RELU6             1
#define IMPL_SCATTER_TO_GRID   0
#define IMPL_SIGMOID           0
#define IMPL_SWISH             0

/* Yololite
*/
#define IMPL_MAXPOOL2D         0
#define IMPL_ADD               0
#define IMPL_AVGPOOL2D         0
#define IMPL_CONCAT            0
#define IMPL_CONV1D            0
#define IMPL_CONV2D            1    //
#define IMPL_CONVTRANS         0
#define IMPL_DCONV             0
#define IMPL_DEPTH2SPACE       0
#define IMPL_DYNAMIC_AXIS      0
#define IMPL_GLOBALAVGPOOL     0
#define IMPL_GLOBALMAXPOOL     0
#define IMPL_LEAKY             1    // 
#define IMPL_MUL               0
#define IMPL_POINTPILLARS      0
#define IMPL_POOL              1    //
#define IMPL_RELU6             0
#define IMPL_SCATTER_TO_GRID   0
#define IMPL_SIGMOID           0
#define IMPL_SWISH             0

/* DCT NET
#define IMPL_MAXPOOL2D         0
#define IMPL_ADD               1
#define IMPL_AVGPOOL2D         0
#define IMPL_CONCAT            0
#define IMPL_CONV1D            0
#define IMPL_CONV2D            1
#define IMPL_CONVTRANS         1
#define IMPL_DCONV             0
#define IMPL_DEPTH2SPACE       0
#define IMPL_DYNAMIC_AXIS      0
#define IMPL_GLOBALAVGPOOL     1
#define IMPL_GLOBALMAXPOOL     0
#define IMPL_LEAKY             0
#define IMPL_MUL               1
#define IMPL_POINTPILLARS      0
#define IMPL_POOL              0
#define IMPL_RELU6             0
#define IMPL_SCATTER_TO_GRID   0
#define IMPL_SIGMOID           1
#define IMPL_SWISH             1
*/

#else // all layer types available

#define IMPL_MAXPOOL2D         1
#define IMPL_ADD               1
#define IMPL_AVGPOOL2D         1
#define IMPL_CONCAT            1
#define IMPL_CONV1D            1
#define IMPL_CONV2D            1
#define IMPL_CONVTRANS         1
#define IMPL_DCONV             1
#define IMPL_DEPTH2SPACE       1
#define IMPL_DYNAMIC_AXIS      1
#define IMPL_GLOBALAVGPOOL     1
#define IMPL_GLOBALMAXPOOL     1
#define IMPL_LEAKY             1
#define IMPL_MUL               1
#define IMPL_POINTPILLARS      1
#define IMPL_POOL              1
#define IMPL_RELU6             1
#define IMPL_SCATTER_TO_GRID   1
#define IMPL_SIGMOID           1
#define IMPL_SWISH             1

#endif

//#define RV_PRINT_SEGMENT_CNT
//#define SEGMENT_SCHEDULING_VERBOSE

#ifdef SEGMENT_SCHEDULING_VERBOSE
inline void print_cmd_segment(const COMMAND_SEGMENT &cmd, int cmd_idx) {
  //    const auto *vpro_cmd = reinterpret_cast<const COMMAND_VPRO *>(seg.data);
  //    const auto *dma_cmd = reinterpret_cast<const COMMAND_DMA::COMMAND_DMA *>(seg.data);
    printf("[%i] @%p: ", cmd_idx, &cmd);
    switch (cmd.type.type) {
        case VPRO_CMD:
           // FIXME
            printf("VPRO, ");
            if (cmd.vpro.command == conv_start)
                printf("conv_start, ");
            else if (cmd.vpro.command == conv_add)
                printf("conv_add, ");
            else if (cmd.vpro.command == relu_pool)
                printf("relu_pool, ");
            else if (cmd.vpro.command == shift_store)
                printf("shift_store, ");
            else if (cmd.vpro.command == shift_store_upsample)
                printf("shift_store_upsample, ");
            else if (cmd.vpro.command == add)
                printf("add, ");
            else if (cmd.vpro.command == mul)
                printf("mul, ");
            printf("lane: 0x%x, buffer: 0x%x, xend: %i, yend: %i, zend: %i, offset: %i\n",
                   cmd.vpro.lane, cmd.vpro.buffer, cmd.vpro.xend, cmd.vpro.yend, cmd.vpro.zend,
                   cmd.vpro.offset);
            break;
        case DMA_CMD:
            printf("DMA, ");
            if (cmd.dma.direction == e2l1D)
                printf("e2l1D, ");
            else if (cmd.dma.direction == e2l2D)
                printf("e2l2D, ");
            else if (cmd.dma.direction == l2e1D)
                printf("l2e1D, ");
            else if (cmd.dma.direction == l2e2D)
                printf("l2e2D, ");
            else if (cmd.dma.direction == loop)
                printf("LOOOOOOOOOOOP, ");
            printf("cluster: 0x%x, unit_mask: 0x%" PRIx32 ", mm_addr: 0x%" PRIx32 ", lm_addr: 0x%" PRIx32 ", y_leap: %i, x_size: %i, y_size: %i\n",
                   cmd.dma.cluster, cmd.dma.unit_mask, cmd.dma.mm_addr, cmd.dma.lm_addr, cmd.dma.y_leap,
                   cmd.dma.x_size, cmd.dma.y_size);
            break;
        case VPRO_WAIT:
            printf("SYNC VPRO\n");
            break;
        case DMA_WAIT:
            printf("SYNC DMA\n");
            break;
        case BOTH_SYNC:
            printf("SYNC Both\n");
            break;
        case DMA_BLOCK:
            printf("DMA Block, size: %i\n", cmd.dma.unit_mask);
            break;

        default: ;
    }
}
#endif


// progressbar
void printProgress(int done, int goal, int width, int &last_progress, bool force_update = false) {
#ifdef SIMULATION
  int active_chars = round(1.0*done/goal*width);

  if (active_chars != last_progress || force_update) {
    last_progress = active_chars;
    printf("\r Commands left: %6i / %i [", goal - done, goal);

    for (int i = 0; i < width; i++) {
      printf(i < active_chars ? "#" : " ");
    }
    printf("] %5.1f%%", 100.0*done/goal);
  }
#endif
}

conv_transpose_table_entry conv_transpose_table[MAX_CONV_TRANSPOSE_STRIDE*MAX_CONV_TRANSPOSE_STRIDE];

void calcLayer(LAYER &layer, const COMMAND_SEGMENT *segments, const uint32_t seg_size) {

#ifdef SIMULATION
    uint32_t startclock = aux_get_sys_time_lo();
#else
    VPRO_BUSY_MASK_CL = 0xffffffff;
#endif

    // Address distance between the starts of two channels in register file.
    // Set by functions that place data into register file (e.g. conv, add, ...)
    // Used by e.g. pooling, activation, ...
    //    lane_mem_layout ml;
//    struct {
//        uint32_t w{0};
//        uint32_t h{0};
//        uint32_t ch{0};
//        uint32_t ch_stride{0};
//    } rfdl; // register file memory layout
//#define set_rf_ch_stride(S) do { rfdl.ch_stride = layer.parallel_outchannels_per_lane > 1 ? (S) : 0; } while (0) // 0 is functionally irrelevant for parallel_outchannels_per_lane, only prevents gamma out of bounds error in sim
//#define set_rfdl(W, H, CH) do {rfdl.w = W; rfdl.h = H; rfdl.ch = CH; set_rf_ch_stride((W)*(H)); } while (0)    

    switch (layer.type) {
#if(defined(LIMIT_IMPL) && defined(IMPL_DYNAMIC_AXIS) || not defined(LIMIT_IMPL))
    case LAYERTYPE::DYNAMIC_AXIS:
        dynamic_shape::set(layer);
        return;
#endif
#if(defined(LIMIT_IMPL) && defined(IMPL_SCATTER_TO_GRID) || not defined(LIMIT_IMPL))
    case LAYERTYPE::SCATTER_TO_GRID:
        if (segments[0].scatter.use_vpro_dma) {
            scatter_to_grid_vpro_dma(layer, segments, seg_size);
        } else {
            scatter_to_grid_riscv(layer, segments, seg_size);
        }
        return;
#endif
    default:
        break; // do nothing
    }

    /**
     * PERFORM PAD
     */
    // configure the dma to pad loaded segments
    dma_set_pad_widths(layer.pad.top, layer.pad.right, layer.pad.bottom, layer.pad.left);
    dma_set_pad_value(layer.pad.value);

    // number of cycles required after a command until the _first_ element has been written (avoid wave-pipelining for short vectors)
    vector_length = (layer.seg_out_w) * (layer.seg_out_h) * (layer.parallel_outchannels_per_lane);
    if (vector_length < 6)
        vector_length_compensate = 6 - vector_length;
    else
        vector_length_compensate = 0;

    if (IMPL_LEAKY && layer.activation == LEAKY) {
        int16_t shift = layer.alpha_mulh_shift_right;
        if (layer.alpha == 0) { shift = 18; } // FIXME why? move to netgen
        vpro_mul_h_bit_shift(shift);
    }

    static uint16_t __attribute__((section(".data"))) point_counts[PP_N_SEGMENTS];
    static uint16_t __attribute__((section(".data"))) seg_offsets[PP_N_SEGMENTS];
    if (IMPL_POINTPILLARS && layer.type == LAYERTYPE::POINTPILLARS) {
        read_grid_segmentation(layer, point_counts, seg_offsets);
    }
    uint16_t max_zend = 0;
    uint16_t next_max_zend = 0;

    if (layer.type == LAYERTYPE::CONV1 ||
        layer.type == LAYERTYPE::CONV2 ||
        layer.type == LAYERTYPE::CONV2_TRANSPOSE ||
        layer.type == LAYERTYPE::DCONV_CONV ||
        layer.type == LAYERTYPE::POINTPILLARS ||
        layer.type == LAYERTYPE::MAXPOOL2D) {

//        printf("%s\n", layer.to_char());

        if (layer.type == LAYERTYPE::CONV1 || layer.type == LAYERTYPE::POINTPILLARS) {
            // for Conv1D/PointPillars kernel_x is the number of input channels 
            // and kernel weights for all input channels are loaded at once into the rf
            kernel_x = layer.in_channels;
        } else {
            kernel_x = layer.kernel_length;
        }        
        kernel_y = layer.kernel_length;
        RF_KERNEL_BASE = RF_DISCARD_ADDR - (kernel_x * kernel_y * layer.parallel_outchannels_per_lane);
        RF_BIAS_BASE = RF_KERNEL_BASE - layer.parallel_outchannels_per_lane;
        RF_RELU_6_BASE = RF_BIAS_BASE - 1;
//        printf("RF_RELU_6_BASE %d\n", RF_RELU_6_BASE);
//        printf("RF_KERNEL_BASE %d\n", RF_KERNEL_BASE);
//        printf("RF_BIAS_BASE %d\n", RF_BIAS_BASE);
//        printf("layer.parallel_outchannels_per_lane %d\n", layer.parallel_outchannels_per_lane);
//        printf("kernel_x %d\n", kernel_x);
//        printf("kernel_y %d\n", kernel_y);

//        sim_wait_step();

        //the result after MAC (convolution) is shifted right
        // to be stored in RF with 24-bit
        vpro_mac_h_bit_shift(layer.conv_result_shift_right);
        if (layer.kernel_length == 1){ // FIXME redundant, why?
            vpro_mac_h_bit_shift(layer.conv_result_shift_right);
        }

        if (layer.type == LAYERTYPE::CONV2 && layer.kernel_length == 1){
            vpro_set_mac_reset_mode(VPRO::MAC_RESET_MODE::X_INCREMENT); // always
        } else {
            vpro_set_mac_reset_mode(VPRO::MAC_RESET_MODE::Z_INCREMENT);
        }
        vpro_set_mac_init_source(VPRO::MAC_INIT_SOURCE::ADDR);  // for adding bias and accumulating of out channel in RF
    } else if (IMPL_DCONV && layer.type == LAYERTYPE::DCONV_DEFORM) {
        vpro_mac_h_bit_shift(layer.conv_result_shift_right);
        vpro_mul_h_bit_shift(layer.conv_result_shift_right);
    } else if (IMPL_ADD && layer.type == LAYERTYPE::ADD) {
        RF_RELU_6_BASE = RF_DISCARD_ADDR - 1;
    } else if (IMPL_MUL && layer.type == LAYERTYPE::MUL) {
        RF_RELU_6_BASE = RF_DISCARD_ADDR - 1;
        vpro_mul_h_bit_shift(layer.conv_result_shift_right);
    } else if (IMPL_GLOBALAVGPOOL && layer.type == LAYERTYPE::GLOBALAVGPOOL2D) {
        _global_avgpool2d_init_constants();
    }

    if (IMPL_CONVTRANS && layer.type == LAYERTYPE::CONV2_TRANSPOSE) {
        _conv_transpose_init_table(layer);
    }

    if (IMPL_AVGPOOL2D && layer.type == LAYERTYPE::AVGPOOL2D) {
        vpro_mac_h_bit_shift(layer.pool_avg_shiftr);  
        vpro_set_mac_reset_mode(VPRO::MAC_RESET_MODE::Z_INCREMENT);
        vpro_set_mac_init_source(VPRO::MAC_INIT_SOURCE::ZERO);
    }

    // Override globals as in DConvConv::generateCommands()
    if (IMPL_DCONV && layer.type == LAYERTYPE::DCONV_CONV) {
        RF_KERNEL_BASE = RF_DISCARD_ADDR - layer.kernel_length;
        RF_BIAS_BASE = RF_KERNEL_BASE - 1;
        RF_RELU_6_BASE = RF_BIAS_BASE - 1;
        kernel_x = layer.kernel_length;
        kernel_y = 1;
    }

    if (IMPL_RELU6 && layer.activation == RELU6) {
        // store shifted value of "6" to RF
        // is 6 representable in RF fixed-point format?
        if (layer.relu_6_shift_left <= 20) // FIXME replace hard-coded constant by RF wordlength-4
            _relu_6_load(layer.relu_6_shift_left);
        else // data > 6 not representable either -> clipping at 6 not required
            layer.activation = RECT;
    }

#if RV_VPRO_EXT == 1 and not defined(SIMULATION)
    create_conv_template_functions(layer);
#endif

#ifdef IS_SIMULATION
    printf_info("Total Segments: %i (%i [%3.2f %%, +%3.2f %%] with sense due to HW Configuration Overhead)\n",
                seg_size,
                seg_size, 100. * (float(seg_size) / float(seg_size)),
                100. * (float(seg_size - seg_size) / float(seg_size)));
    int last_progress = -1;
#endif  

    // RF Layout
    // KERNEL
    //    1024 (end) - 9 (3x3 kernel) => 1014-1023
    // BIAS
    //    1024 (end) - 9 (3x3 kernel) -1 => 1013
    // RELU6_value
    //    1012
    // DATA (CONV Out)
    //    0 - 1011

    // LM Layout
    // KERNEL
    //    (buffer_calc * 4096) + 4096 - (kernel_x * kernel_y * (lane+1));
    //      A: L0: 8192 - 1*9 (3x3 kernel) = 8183-8191
    //      A: L1: 8192 - 2*9 (3x3 kernel) = 8174-8182
    //      B: L0: 4096 - 1*9 (3x3 kernel) = 4087-4095
    //      B: L1: 4096 - 2*9 (3x3 kernel) = 4079-4086
    // BIAS
    //    (buffer_calc * 4096) + 4096 - (kernel_x * kernel_y * 2)- 1 - lane;
    //      A: L0: 8192 - 2*9 (3x3 kernel) - 1     = 8173
    //      A: L1: 8192 - 2*9 (3x3 kernel) - 1 - 1 = 8172
    //      B: L0: 4096 - 2*9 (3x3 kernel) - 1     = 4078
    //      B: L1: 4096 - 2*9 (3x3 kernel) - 1 - 1 = 4077
    // DATA
    //      IN: ?
    //      OUT: ?

    /**
     * EXECUTE SEGMENTs         <-- main Loop -->
     *
     * Fetches Segment for current layer (big loop)
     * Depending on command type, executes DMA or VPRO instructions
     *
     * DMA:
     *  dma offset is added (depending on dynamic weights array adresses in mips/vpro system)
     *  -> precalculated in configuration_generation app. only needed for SIM (conv pointer passed)
     *
     * VPRO:
     *  all functions are declared inline
     */

#ifdef RV_PRINT_SEGMENT_CNT
    int lst = 0;
    int dmas = 0;
    int vpros = 0;
    int dma_syncs = 0;
    int vpro_syncs = 0;
#endif

    intptr_t start_segment = intptr_t(&segments[0]);
    intptr_t end_segment = intptr_t(&segments[0]) + seg_size * sizeof(COMMAND_SEGMENT);

#ifndef SIMULATION
    // declaration of a uint32_t variabale here will force gcc application for risc-v only to load the dcache short address once!?
    uint32_t *dcache_short_hw = (uint32_t *)(IDMA_COMMAND_DCACHE_ADDR);
#endif

// #pragma GCC unroll 8
    // FIXME why intptr_t instead of
    for (intptr_t seg_cnt = start_segment; seg_cnt < end_segment; seg_cnt += sizeof(COMMAND_SEGMENT)) {
        // hint: seg_cnt is also incremented manually within loop
#define CMD ((COMMAND_SEGMENT *)seg_cnt)

        int cmd_idx = (seg_cnt - start_segment) / sizeof(COMMAND_SEGMENT);

#ifdef SIMULATION
        printProgress(cmd_idx, seg_size, 60, last_progress);
#elif defined(RV_PRINT_SEGMENT_CNT)
        //        uint32_t mask = 0xffffffff; // every segments
        //        uint32_t mask = 0xffffff80; // every 128 segments
        uint32_t mask = 0xfffffc00; // every 1024 segments
        if (((seg_size - cmd_idx) & mask) != lst) {
            printf("\r%7i",seg_size - cmd_idx);
            lst = (seg_size - cmd_idx) & mask;
        }
#endif
#ifdef SEGMENT_SCHEDULING_VERBOSE
        print_cmd_segment(*CMD, cmd_idx);
#endif

        if (CMD->type.type == DMA_BLOCK) {
            if (IMPL_POINTPILLARS && layer.type == LAYERTYPE::POINTPILLARS) {
                auto max_dma_size = dynamic_dma_block(layer, CMD, point_counts, seg_offsets);
                // vector length zend is inferred from maximum size of dynamic dmas (segment with most points)
                // update of max_zend is delayed due to double buffering: current dma block (load) corresponds to next set of vpro commands (compute)
                max_zend = next_max_zend;
                next_max_zend = std::max(max_dma_size - 1, 0);
            }
            const COMMAND_DMA &dmab = CMD->dma;
            uint block_size = dmab.unit_mask;
//            printf_warning("DMA BLOCK Segment! [size; %i]\n", block_size); // , start: %li, seg_cnt);
            dma_block_size(block_size);
            dma_block_addr_trigger((void *)(seg_cnt + sizeof(COMMAND_SEGMENT)));
            seg_cnt += block_size * sizeof(COMMAND_SEGMENT);
        } else if (CMD->type.type == VPRO_CMD) {
#if defined(RV_PRINT_SEGMENT_CNT)
            vpros++;
#endif
            const COMMAND_VPRO &vpro = CMD->vpro;
            int nops = vpro.nops;
            uint32_t zend = vpro.zend;
            if (IMPL_POINTPILLARS && layer.type == LAYERTYPE::POINTPILLARS && (
                vpro.command == VPRO_TYPE::conv1d_start ||
                vpro.command == VPRO_TYPE::conv1d_add ||
                vpro.command == VPRO_TYPE::relu_pool_scatter)) {
                    zend = max_zend; // for PointPillars zend is determined dynamically
                    nops = std::max(0, W2R_BUBBLE_CYCLES - max_zend);
                }
            if (IMPL_CONV2D && vpro.command == VPRO_TYPE::conv_start) {
                _bias_load(layer, vpro.bias_load_buffer_l0, 0);
                _bias_load(layer, vpro.bias_load_buffer_l1, 1);
                _kernel_load_right(layer, vpro.kernel_load_buffer_l0, 0, 0);
                _kernel_load_right(layer, vpro.kernel_load_buffer_l1, 1, 0);
                _conv(layer, vpro.buffer);
            } else if (IMPL_CONV2D && vpro.command == VPRO_TYPE::conv_add) {
                _kernel_load_right(layer, vpro.kernel_load_buffer_l0, 0, 0);
                _kernel_load_right(layer, vpro.kernel_load_buffer_l1, 1, 0);
                _conv_add(layer, vpro.buffer);
            } else if (IMPL_CONV1D && vpro.command == VPRO_TYPE::conv1d_start) {
                _bias_load(layer, vpro.bias_load_buffer_l0, 0);
                _bias_load(layer, vpro.bias_load_buffer_l1, 1);
                _kernel_load_right(layer, vpro.kernel_load_buffer_l0, 0, 0);
                _kernel_load_right(layer, vpro.kernel_load_buffer_l1, 1, 0);
                _conv1d(layer, zend, vpro.lm_base, vpro.rf_base);
            } else if (IMPL_CONV1D && vpro.command == VPRO_TYPE::conv1d_add) {
                _conv1d_add(layer, zend, vpro.lm_base, vpro.rf_base, vpro.in_ch_offset);
            } else if (IMPL_POINTPILLARS && vpro.command == VPRO_TYPE::relu_pool_scatter) {
                insertNops(nops);
                _act_relu(vpro.xend, vpro.yend, zend, vpro.rf_ch_stride, vpro.rf_base, (LANE)vpro.lane_mask);
                _pool_scatter(zend, vpro.lm_base, vpro.pp_index_buffer, vpro.rf_base);
            } else if (IMPL_POINTPILLARS && vpro.command == VPRO_TYPE::set_masks) {
                vpro_set_cluster_mask(vpro.cluster_mask);
                vpro_set_unit_mask(vpro.unit_mask);
                max_zend = std::max(point_counts[vpro.offset] - 1, 0);
            } else if (IMPL_POINTPILLARS && vpro.command == VPRO_TYPE::reset_indices) {
                reset_indices_lm(vpro.lm_base, vpro.zend);
            } else if (IMPL_CONVTRANS && vpro.command == VPRO_TYPE::conv_transpose_start) {
                _bias_load(layer, vpro.bias_load_buffer_l0, 0);
                _bias_load(layer, vpro.bias_load_buffer_l1, 1);
                _kernel_load_right(layer, vpro.kernel_load_buffer_l0, 0, 0);
                _kernel_load_right(layer, vpro.kernel_load_buffer_l1, 1, 0);
                _conv_transpose_start(layer, vpro.buffer);
            } else if (IMPL_CONVTRANS && vpro.command == VPRO_TYPE::conv_transpose_add) {
                _kernel_load_right(layer, vpro.kernel_load_buffer_l0, 0, 0);
                _kernel_load_right(layer, vpro.kernel_load_buffer_l1, 1, 0);
                _conv_transpose_add(layer, vpro.buffer);
            } else if (IMPL_ADD && vpro.command == VPRO_TYPE::add) {
                insertNops(vpro.nops);
                _elwise(vpro.command, layer.elwise_0_left_shift, layer.elwise_1_left_shift, vpro.broadcast_map, vpro.xend, vpro.yend, vpro.rf_base, vpro.lm_base);
            } else if (IMPL_MUL && vpro.command == VPRO_TYPE::mul) {
                insertNops(vpro.nops);

                // _mul() and _act_leakyrelu() use mul_h_bit_shift
                // mul_h_bit_shift needs to be prepared here in the 2nd ff. sets
                int16_t leaky_shift = layer.alpha_mulh_shift_right;
                if (IMPL_LEAKY && layer.activation == LEAKY) {
                    leaky_shift = layer.alpha_mulh_shift_right;
                    if (layer.alpha == 0) { leaky_shift = 18; } // FIXME why? move to netgen
                    if (layer.conv_result_shift_right != leaky_shift) {
                        vpro_lane_sync();
                        vpro_mul_h_bit_shift(layer.conv_result_shift_right);
                        //                        printf("==== vpro_mul_h_bit_shift(%d), readback: %d\n", layer.conv_result_shift_right, vpro_mul_h_bit_shift(void));
                    }
                }
                
                _elwise(vpro.command, layer.elwise_0_left_shift, layer.elwise_1_left_shift, vpro.broadcast_map, vpro.xend, vpro.yend, vpro.rf_base, vpro.lm_base);

                if (IMPL_LEAKY && layer.activation == LEAKY) {
                    if (layer.conv_result_shift_right != leaky_shift) {
                        vpro_lane_sync();
                        vpro_mul_h_bit_shift(leaky_shift);
                    }
                }
            } else if (IMPL_CONCAT && vpro.command == VPRO_TYPE::concatenate) {
                _concatenate(layer, vpro.buffer, vpro.offset, vpro.xend, vpro.yend, vpro.shift_right);

            } else if (IMPL_DEPTH2SPACE && vpro.command == VPRO_TYPE::depth_to_space) {
                _depth_to_space(layer, vpro.buffer, vpro.xend, vpro.yend);

            } else if (IMPL_AVGPOOL2D && vpro.command == VPRO_TYPE::avgpool2d) {
                _avgpool2d_load_kernel(layer, vpro.buffer, vpro.lane, vpro.kernel_load_buffer_l0, vpro.xend, vpro.yend);
                _avgpool2d_pool(layer, vpro.buffer, vpro.lane, vpro.xend, vpro.yend);
                _avgpool2d_store(layer, vpro.offset, vpro.lane, vpro.xend, vpro.yend);
            } else if (IMPL_MAXPOOL2D && vpro.command == VPRO_TYPE::max_pooling) {
                _maxpool2d(layer, vpro.buffer);
            } else if (IMPL_GLOBALAVGPOOL && vpro.command == VPRO_TYPE::global_avgpool2d_start) {

                vpro_lane_sync();
                vpro_set_mac_reset_mode(VPRO::MAC_RESET_MODE::ONCE);
                vpro_set_mac_init_source(VPRO::MAC_INIT_SOURCE::ZERO); // FIXME move elsewhere
                vpro_mac_h_bit_shift(16); // FIXME move elsewhere

                _global_avgpool2d_add(vpro.xend, vpro.yend, vpro.zend, vpro.lm_base);

                vpro_lane_sync();
                vpro_set_mac_reset_mode(VPRO::MAC_RESET_MODE::NEVER);

            } else if (IMPL_GLOBALAVGPOOL && vpro.command == VPRO_TYPE::global_avgpool2d_add) {
                _global_avgpool2d_add(vpro.xend, vpro.yend, vpro.zend, vpro.lm_base);
                //            } else if (vpro.command == VPRO_TYPE::gobal_avgpool2d_store) {
                //                _global_avgpool2d_store(layer, vpro.buffer);
            } else if (IMPL_GLOBALAVGPOOL && vpro.command == VPRO_TYPE::global_avgpool2d_store_intermediates) {
                _global_avgpool2d_store_intermediates(vpro.lm_base);
            } else if (IMPL_GLOBALAVGPOOL && vpro.command == VPRO_TYPE::global_avgpool2d_sum_intermediates) {
                _global_avgpool2d_sum_intermediates(vpro.n_summands, vpro.lm_base);
            } else if (IMPL_GLOBALAVGPOOL && vpro.command == VPRO_TYPE::global_avgpool2d_divide) {
                _global_avgpool2d_divide(vpro.pre_shift_right, vpro.multiplier, vpro.shift_right, vpro.lm_base);
            } else if (vpro.command == VPRO_TYPE::shift_store) {
                insertNops(vpro.nops);
                _shift_store(vpro.shift_right, vpro.xend, vpro.yend, vpro.zend, vpro.rf_ch_stride, vpro.lm_ch_stride,
                             vpro.rf_base, vpro.lm_base, vpro.lm_lane_stride, (LANE)vpro.lane_mask);
            } else if (vpro.command == VPRO_TYPE::shift_store_upsample) {
                insertNops(vpro.nops);
                _shift_store_upsample(vpro.shift_right, vpro.xend, vpro.yend, vpro.zend, vpro.rf_ch_stride, vpro.lm_ch_stride,
                             vpro.rf_base, vpro.lm_base, vpro.lm_lane_stride, (LANE)vpro.lane_mask);
            } else if (IMPL_POOL && vpro.command == VPRO_TYPE::maxpool2x2_fused) {
                insertNops(vpro.nops);
                _maxpool2x2(vpro.xend, vpro.yend, vpro.zend, vpro.rf_ch_stride, vpro.rf_base, (LANE)vpro.lane_mask);
            } else if (vpro.command == VPRO_TYPE::activation_fused) {
                insertNops(vpro.nops);
                if (layer.activation == RECT) {
                    _act_relu(vpro.xend, vpro.yend, vpro.zend, vpro.rf_ch_stride, vpro.rf_base, (LANE)vpro.lane_mask);
                } else if (IMPL_LEAKY && layer.activation == LEAKY) {
                    // uses mulh_bit_shift
                    _act_leakyrelu(layer.alpha, vpro.xend, vpro.yend, vpro.zend, vpro.rf_ch_stride, vpro.rf_base, (LANE)vpro.lane_mask);
                } else if (IMPL_RELU6 && layer.activation == RELU6) {
                    _act_relu6(vpro.xend, vpro.yend, vpro.zend, vpro.rf_ch_stride, vpro.rf_base, (LANE)vpro.lane_mask);
                } else if (IMPL_SIGMOID && layer.activation == SIGMOID) {
                    _act_sigmoid_fast(vpro.rf_frac_bits, vpro.xend, vpro.yend, vpro.zend, vpro.rf_ch_stride, vpro.rf_base, (LANE)vpro.lane_mask);
                } else if (IMPL_SWISH && layer.activation == SWISH) {
                    _act_swish(vpro.rf_frac_bits, vpro.shift_right, vpro.xend, vpro.yend, vpro.zend, vpro.rf_ch_stride, vpro.lm_ch_stride, vpro.rf_base, vpro.lm_base, vpro.lm_lane_stride, (LANE)vpro.lane_mask);
                } else {
                    printf("\n[Error] activation %d unknown or excluded (IMPL_...)\n", layer.activation);
                }
            } else if (IMPL_DCONV && vpro.command == VPRO_TYPE::dconv_deform_8x8) {
                for (int y = 0; y < 2; y++) {
                    for (int x = 0; x < 2; x++) {
                        // TODO(Jasper): These are currently duplicated in deform_kernel.h
                        constexpr uint16_t kernel_length = 9;
                        constexpr uint16_t block_width = 4;
                        constexpr uint16_t block_height = 4;
                        constexpr uint16_t chunk_width = 8;
                        //constexpr uint16_t chunk_height = 8;

                        uint16_t input_buffer = vpro.buffer;
                        uint16_t offset_buffer = vpro.deform_offset_buffer + y * block_height * chunk_width * 3 * kernel_length + x * block_width;
                        uint16_t output_buffer = vpro.deform_output_buffer + (y * block_height * chunk_width + x * block_width) * kernel_length;
                        uint16_t static_offset_buffer = layer.deform_static_offsets + (y * block_height * chunk_width + x * block_width) * kernel_length;

                        _deform_block_4x4(layer, input_buffer, offset_buffer, output_buffer, static_offset_buffer);
                    }
                }
            } else if (IMPL_DCONV && vpro.command == VPRO_TYPE::dconv_conv_start) {
                _bias_load(layer, vpro.bias_load_buffer_l0, 0);
                _bias_load(layer, vpro.bias_load_buffer_l1, 1);
                _kernel_load_right(layer, vpro.kernel_load_buffer_l0, 0, 0);
                _kernel_load_right(layer, vpro.kernel_load_buffer_l1, 1, 0);
                _dconv_conv(layer, vpro.buffer);
            } else if (IMPL_DCONV && vpro.command == VPRO_TYPE::dconv_conv_add) {
                _kernel_load_right(layer, vpro.kernel_load_buffer_l0, 0, 0);
                _kernel_load_right(layer, vpro.kernel_load_buffer_l1, 1, 0);
                _dconv_conv_add(layer, vpro.buffer);
            }
        } else if (CMD->type.type == BOTH_SYNC) {
#ifndef SIMULATION
            // load dcache line for segments to avoid dcache stall in next loop iterations
                    {
                        auto dcache_line_size_bytes = 4096; // 1024 Bytes in 64 x 128-bit words
                        [[maybe_unused]] volatile auto tmp = *(reinterpret_cast<const uint8_t *>(seg_cnt) + dcache_line_size_bytes);
                    }
#endif
            vpro_sync();
        } else if (CMD->type.type == DMA_WAIT) {
//            printf("[SYNC DMA]\n");
#if defined(RV_PRINT_SEGMENT_CNT)
            dma_syncs++;
#endif
#ifndef SIMULATION
            // load dcache line for segments to avoid dcache stall in next loop iterations
                    {
                        auto dcache_line_size_bytes = 4096; // 1024 Bytes in 64 x 128-bit words
                        [[maybe_unused]] volatile auto tmp = *(reinterpret_cast<const uint8_t *>(seg_cnt) + dcache_line_size_bytes);
                    }
#endif
            vpro_dma_sync();
        } else if (CMD->type.type == VPRO_WAIT) {
//            printf("[SYNC VPRO]\n");
#if defined(RV_PRINT_SEGMENT_CNT)
            vpro_syncs++;
#endif
#ifndef SIMULATION
            // load dcache line for segments to avoid dcache stall in next loop iterations
                    {
                        auto dcache_line_size_bytes = 4096; // 1024 Bytes in 64 x 128-bit words
                        [[maybe_unused]] volatile auto tmp = *(reinterpret_cast<const uint8_t *>(seg_cnt) + dcache_line_size_bytes);
                    }
#endif
            vpro_lane_sync();
//            vpro_wait_busy(0xffffffff, 0xffffffff);
        } else if (CMD->type.type == DMA_CMD) {
//            printf("[DMA]\n");
#if defined(RV_PRINT_SEGMENT_CNT)
            dmas++;
#endif
            dma_dcache_short_command((void*)(seg_cnt));
#if(defined(LIMIT_IMPL) && defined(IMPL_DCONV) || not defined(LIMIT_IMPL))
        } else if (IMPL_DCONV && CMD->type.type == DMA_SET_PADDING) {
            auto dma_padding = ((COMMAND_DMA_PADDING *)seg_cnt);
            dma_set_pad_widths(dma_padding->pad.top, dma_padding->pad.right, dma_padding->pad.bottom, dma_padding->pad.left);
            dma_set_pad_value(dma_padding->pad.value);
#endif
        } else {
            // not recognized segment type
            // check segment address, generation (endianess), values, ...
            aux_print_debugfifo(0xdead1009);
            printf("\n[Error] Segment type unknown!\n");
            printf("Segment.type 0x%8x\n", (unsigned int) CMD->type.type);
            printf("Segment[%u] @ 0x%8x\n", (unsigned int) ((seg_cnt - start_segment) / sizeof(COMMAND_SEGMENT)),
                   (unsigned int) uint32_t(seg_cnt));
            exit(1);
        } // case    / if
    } // for all cmd segments


#ifdef SIMULATION
    printProgress(seg_size, seg_size, 60, last_progress, true);
    printf("\n");
    printf("[LAYER %i] took %i cycles\n", layer.number, aux_get_cycle_cnt() - startclock);
    printf("[Clock] %i cycles\n", aux_get_cycle_cnt());
#elif defined(RV_PRINT_SEGMENT_CNT)
    printf("\r      0 Segments Remaining\n");
    printf("\tDMA Segments: %i\n", dmas);
    printf("\tVPRO Segments: %i\n", vpros);
    printf("\tDMA Sync Segments: %i\n", dma_syncs);
    printf("\tVPRO Sync Segments: %i\n", vpro_syncs);
#endif
}
