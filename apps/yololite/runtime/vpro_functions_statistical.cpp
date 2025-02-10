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

#include "vpro_functions.h"

uint32_t RF_KERNEL_BASE;
uint32_t RF_BIAS_BASE;
uint32_t RF_RELU_6_BASE;
uint32_t kernel_x, kernel_y;
uint32_t vector_length, vector_length_compensate;

#if not defined(SIMULATION) and RV_VPRO_EXT == 1

#include <vpro.h>
#include <vpro/vpro_asm.h>
#include <vpro/dma_asm.h>

using namespace VPRO_RISC_EXT_VPRO;
using namespace VPRO_RISC_EXT_DMA;

template<int n>
void reset_dma() {
    c_dma_lw<n, DMA_PARAMETER_INDIZES::ext_addr, DMA_PARAMETER_INDIZES::int_addr, NoTrigger>(0, 0);
    c_dma_lw<n, DMA_PARAMETER_INDIZES::y_size, DMA_PARAMETER_INDIZES::type, NoTrigger>(0, 0);
    c_dma_lw<n, DMA_PARAMETER_INDIZES::x_size, DMA_PARAMETER_INDIZES::x_stride, NoTrigger>(0, 0);
    c_dma_lw<n, DMA_PARAMETER_INDIZES::cluster, DMA_PARAMETER_INDIZES::broadcast_mask, NoTrigger>(0, 0);
    c_dma_lw<n, DMA_PARAMETER_INDIZES::pad_flags, DMA_PARAMETER_INDIZES::nowhere, NoTrigger>(0, 0);
}

void reset_all_dma() {
    reset_dma<0>();
    reset_dma<1>();
    reset_dma<2>();
    reset_dma<3>();
    reset_dma<4>();
    reset_dma<5>();
    reset_dma<6>();
    reset_dma<7>();
}

void reset_all_vpro() {
    c_vpro_li<0, 0b1111111111111, NoTrigger>(0);
    c_vpro_li<1, 0b1111111111111, NoTrigger>(0);
    c_vpro_li<2, 0b1111111111111, NoTrigger>(0);
    c_vpro_li<3, 0b1111111111111, NoTrigger>(0);
    c_vpro_li<4, 0b1111111111111, NoTrigger>(0);
    c_vpro_li<5, 0b1111111111111, NoTrigger>(0);
    c_vpro_li<6, 0b1111111111111, NoTrigger>(0);
    c_vpro_li<7, 0b1111111111111, NoTrigger>(0);
}

void create_conv_template_functions(const BIF::LAYER &layer) {

    // all to 0
//    reset_all_dma();
    reset_all_vpro();

//    // shift ar
//    if (layer.store_shift_right > 0) {
//        c_vpro_lw<4, VPRO_PARAMETER_INDIZES::dst_all, VPRO_PARAMETER_INDIZES::func, NoTrigger>(DST_ADDR(0, 1, layer.seg_out_w, layer.seg_out_w * layer.seg_out_h), FUNC_SHIFT_AR);
//        c_vpro_lw<4, VPRO_PARAMETER_INDIZES::src1_all, VPRO_PARAMETER_INDIZES::nowhere, NoTrigger>(SRC1_ADDR(0, 1, layer.seg_out_w, layer.seg_out_w * layer.seg_out_h), 0);
//        c_vpro_lw<4, VPRO_PARAMETER_INDIZES::chain_blocking_update_flag, VPRO_PARAMETER_INDIZES::src2_imm, NoTrigger>(0b100, SRC2_IMM_3D(layer.store_shift_right));
//    } else {    // mull
//        c_vpro_lw<4, VPRO_PARAMETER_INDIZES::dst_all, VPRO_PARAMETER_INDIZES::func, NoTrigger>(DST_ADDR(0, 1, layer.seg_out_w, layer.seg_out_w * layer.seg_out_h), FUNC_MULL);
//        c_vpro_lw<4, VPRO_PARAMETER_INDIZES::src1_all, VPRO_PARAMETER_INDIZES::nowhere, NoTrigger>(SRC1_ADDR(0, 1, layer.seg_out_w, layer.seg_out_w * layer.seg_out_h), 0);
//        c_vpro_lw<4, VPRO_PARAMETER_INDIZES::chain_blocking_update_flag, VPRO_PARAMETER_INDIZES::src2_imm, NoTrigger>(0b100, SRC2_IMM_3D(1 << (-layer.store_shift_right)));
//    }
//
//    // store
//    c_vpro_lw<7, VPRO_PARAMETER_INDIZES::id, VPRO_PARAMETER_INDIZES::func, NoTrigger>(LS, FUNC_STORE);
//    c_vpro_lw<7, VPRO_PARAMETER_INDIZES::src1_all, VPRO_PARAMETER_INDIZES::nowhere, NoTrigger>(SRC1_ADDR(0, 1, layer.seg_out_w, layer.seg_out_w * layer.seg_out_h), 0);
//    c_vpro_lw<7, VPRO_PARAMETER_INDIZES::chain_blocking_update_flag, VPRO_PARAMETER_INDIZES::src2_imm, NoTrigger>(0b100, SRC2_IMM_3D(layer.store_shift_right));

    if (layer.type == LAYERTYPE::CONV2 || layer.type == LAYERTYPE::CONV1 || layer.type == LAYERTYPE::CONV2_TRANSPOSE ||
        layer.type == LAYERTYPE::DCONV_CONV || layer.type == LAYERTYPE::POINTPILLARS) {
        if (layer.kernel_length != 1 || layer.parallel_outchannels_per_lane == 1) {
//    // VPRO LOADS Bias //dst = dont care
            c_vpro_lw<0, VPRO_PARAMETER_INDIZES::id, VPRO_PARAMETER_INDIZES::func, NoTrigger>(LS, FUNC_LOADS);
            c_vpro_lw<0, VPRO_PARAMETER_INDIZES::dst_all, VPRO_PARAMETER_INDIZES::src1_all, NoTrigger>(DST_ADDR(0, 0, 0, 0), SRC1_ADDR(0, 0, 0, 0));
            c_vpro_lw<0, VPRO_PARAMETER_INDIZES::chain_blocking_update_flag, VPRO_PARAMETER_INDIZES::x_y_z_end, NoTrigger>(0b100, 0);

//    // VPRO SHIFT_ Bias (shift al/ar) [one of the | depend on layer]
            if (layer.bias_shift_right <= 0) {
                c_vpro_lw<1, VPRO_PARAMETER_INDIZES::chain_blocking_update_flag, VPRO_PARAMETER_INDIZES::func, NoTrigger>(0b010, FUNC_MULL);
                c_vpro_lw<1, VPRO_PARAMETER_INDIZES::dst_all, VPRO_PARAMETER_INDIZES::src1_all, NoTrigger>(DST_ADDR(RF_BIAS_BASE, 0, 0, 0), SRC1_LS_3D);
                c_vpro_lw<1, VPRO_PARAMETER_INDIZES::src2_imm, VPRO_PARAMETER_INDIZES::x_y_z_end, NoTrigger>(SRC2_IMM_3D(1u << (-layer.bias_shift_right)), 0);
            } else {
                c_vpro_lw<1, VPRO_PARAMETER_INDIZES::chain_blocking_update_flag, VPRO_PARAMETER_INDIZES::func, NoTrigger>(0b010, FUNC_SHIFT_AR);
                c_vpro_lw<1, VPRO_PARAMETER_INDIZES::dst_all, VPRO_PARAMETER_INDIZES::src1_all, NoTrigger>(DST_ADDR(RF_BIAS_BASE, 0, 0, 0), SRC1_LS_3D);
                c_vpro_lw<1, VPRO_PARAMETER_INDIZES::src2_imm, VPRO_PARAMETER_INDIZES::x_y_z_end, NoTrigger>(SRC2_IMM_3D(layer.bias_shift_right), 0);
            }

//    // VPRO LOADS Kernel  //dst = dont care
            c_vpro_lw<2, VPRO_PARAMETER_INDIZES::id, VPRO_PARAMETER_INDIZES::func, NoTrigger>(LS, FUNC_LOADS);
            c_vpro_lw<2, VPRO_PARAMETER_INDIZES::dst_all, VPRO_PARAMETER_INDIZES::src1_all, NoTrigger>(DST_ADDR(0, 0, 0, 0), SRC1_ADDR(0, 1, kernel_y, 0));
            c_vpro_lw<2, VPRO_PARAMETER_INDIZES::chain_blocking_update_flag, VPRO_PARAMETER_INDIZES::x_y_z_end, NoTrigger>(0b100,
                                                                                                                           uint32_t(kernel_x - 1) << (10 + 6) |
                                                                                                                           uint32_t(kernel_y - 1) << 10);

//    // VPRO MULL Kernel (shift al)
            c_vpro_lw<3, VPRO_PARAMETER_INDIZES::dst_all, VPRO_PARAMETER_INDIZES::func, NoTrigger>(DST_ADDR(RF_KERNEL_BASE, 1, kernel_y, 0), FUNC_SHIFT_AR);
            c_vpro_lw<3, VPRO_PARAMETER_INDIZES::src1_all, VPRO_PARAMETER_INDIZES::x_y_z_end, NoTrigger>(SRC1_LS_3D,
                                                                                                         uint32_t(kernel_x - 1) << (10 + 6) | uint32_t(kernel_y - 1) << 10);
            c_vpro_lw<3, VPRO_PARAMETER_INDIZES::chain_blocking_update_flag, VPRO_PARAMETER_INDIZES::src2_imm, NoTrigger>(0b010, SRC2_IMM_3D(0));
        }
    }

    if (layer.type == LAYERTYPE::CONV2) {
        if (layer.kernel_length == 1 && layer.parallel_outchannels_per_lane > 1) {

//    // VPRO LOADS Bias //dst = dont care
            c_vpro_lw<0, VPRO_PARAMETER_INDIZES::id, VPRO_PARAMETER_INDIZES::func, NoTrigger>(LS, FUNC_LOADS);
            c_vpro_lw<0, VPRO_PARAMETER_INDIZES::dst_all, VPRO_PARAMETER_INDIZES::src1_all, NoTrigger>(DST_ADDR(0, 0, 0, 0), SRC1_ADDR(0, 0, 0, 1));
            c_vpro_lw<0, VPRO_PARAMETER_INDIZES::chain_blocking_update_flag, VPRO_PARAMETER_INDIZES::x_y_z_end, NoTrigger>(0b100, layer.parallel_outchannels_per_lane - 1);

//    // VPRO SHIFT_ Bias (shift al/ar) [one of the | depend on layer]
            if (layer.bias_shift_right <= 0) {
                c_vpro_lw<1, VPRO_PARAMETER_INDIZES::chain_blocking_update_flag, VPRO_PARAMETER_INDIZES::func, NoTrigger>(0b010, FUNC_MULL);
                c_vpro_lw<1, VPRO_PARAMETER_INDIZES::dst_all, VPRO_PARAMETER_INDIZES::src1_all, NoTrigger>(DST_ADDR(RF_BIAS_BASE, 0, 0, 1), SRC1_LS_3D);
                c_vpro_lw<1, VPRO_PARAMETER_INDIZES::src2_imm, VPRO_PARAMETER_INDIZES::x_y_z_end, NoTrigger>(SRC2_IMM_3D(1u << (-layer.bias_shift_right)),
                                                                                                             layer.parallel_outchannels_per_lane - 1);
            } else {
                c_vpro_lw<1, VPRO_PARAMETER_INDIZES::chain_blocking_update_flag, VPRO_PARAMETER_INDIZES::func, NoTrigger>(0b010, FUNC_SHIFT_AR);
                c_vpro_lw<1, VPRO_PARAMETER_INDIZES::dst_all, VPRO_PARAMETER_INDIZES::src1_all, NoTrigger>(DST_ADDR(RF_BIAS_BASE, 0, 0, 1), SRC1_LS_3D);
                c_vpro_lw<1, VPRO_PARAMETER_INDIZES::src2_imm, VPRO_PARAMETER_INDIZES::x_y_z_end, NoTrigger>(SRC2_IMM_3D(layer.bias_shift_right),
                                                                                                             layer.parallel_outchannels_per_lane - 1);
            }

//    // VPRO LOADS Kernel  //dst = dont care
            c_vpro_lw<2, VPRO_PARAMETER_INDIZES::id, VPRO_PARAMETER_INDIZES::func, NoTrigger>(LS, FUNC_LOADS);
            c_vpro_lw<2, VPRO_PARAMETER_INDIZES::dst_all, VPRO_PARAMETER_INDIZES::src1_all, NoTrigger>(DST_ADDR(0, 0, 0, 0), SRC1_ADDR(0, 1, kernel_y, kernel_x * kernel_y));
            c_vpro_lw<2, VPRO_PARAMETER_INDIZES::chain_blocking_update_flag, VPRO_PARAMETER_INDIZES::x_y_z_end, NoTrigger>(0b100,
                                                                                                                           uint32_t(kernel_x - 1) << (10 + 6) |
                                                                                                                           uint32_t(kernel_y - 1) << 10 |
                                                                                                                           uint32_t(layer.parallel_outchannels_per_lane - 1));

//    // VPRO MULL Kernel (shift al)
            c_vpro_lw<3, VPRO_PARAMETER_INDIZES::dst_all, VPRO_PARAMETER_INDIZES::func, NoTrigger>(DST_ADDR(RF_KERNEL_BASE, 1, kernel_y, kernel_x * kernel_y), FUNC_SHIFT_AR);
            c_vpro_lw<3, VPRO_PARAMETER_INDIZES::src1_all, VPRO_PARAMETER_INDIZES::x_y_z_end, NoTrigger>(SRC1_LS_3D,
                                                                                                         uint32_t(kernel_x - 1) << (10 + 6) | uint32_t(kernel_y - 1) << 10 |
                                                                                                         (layer.parallel_outchannels_per_lane - 1));
            c_vpro_lw<3, VPRO_PARAMETER_INDIZES::chain_blocking_update_flag, VPRO_PARAMETER_INDIZES::src2_imm, NoTrigger>(0b010, SRC2_IMM_3D(0));

            assert(layer.seg_out_w * layer.parallel_outchannels_per_lane >= 5);  // TODO: vector length compensate
            assert((unsigned int) layer.seg_out_w - 1 <= MAX_X_END);
            assert((unsigned int) layer.seg_out_h - 1 <= MAX_Y_END);
            assert((unsigned int) layer.seg_in_w <= MAX_BETA);
            assert((unsigned int) layer.seg_out_w <= MAX_BETA);
            assert((unsigned int) layer.seg_out_w * layer.seg_out_h <= MAX_GAMMA);

//    // VPRO LOADS (conv loop)
            c_vpro_lw<5, VPRO_PARAMETER_INDIZES::id, VPRO_PARAMETER_INDIZES::func, NoTrigger>(LS, FUNC_LOADS);
            c_vpro_lw<5, VPRO_PARAMETER_INDIZES::dst_all, VPRO_PARAMETER_INDIZES::x_y_z_end, NoTrigger>(0, (uint32_t(layer.seg_out_w - 1) << (10 + 6)) |
                                                                                                           (uint32_t(layer.seg_out_h - 1) << 10) |
                                                                                                           uint32_t(layer.parallel_outchannels_per_lane - 1));
            c_vpro_lw<5, VPRO_PARAMETER_INDIZES::chain_blocking_update_flag, VPRO_PARAMETER_INDIZES::src1_all, NoTrigger>(0b100, SRC1_ADDR(0, 1, layer.seg_in_w, 0));

//    // VPRO MACH (conv loop)
            c_vpro_lw<6, VPRO_PARAMETER_INDIZES::id, VPRO_PARAMETER_INDIZES::func, NoTrigger>(L0_1, FUNC_MACH);
            c_vpro_lw<6, VPRO_PARAMETER_INDIZES::src2_all, VPRO_PARAMETER_INDIZES::x_y_z_end, NoTrigger>(SRC2_ADDR(RF_KERNEL_BASE, 0, 0, 1),
                                                                                                         (uint32_t(layer.seg_out_w - 1) << (10 + 6)) |
                                                                                                         (uint32_t(layer.seg_out_h - 1) << 10) |
                                                                                                         uint32_t(layer.parallel_outchannels_per_lane - 1));
            c_vpro_lw<6, VPRO_PARAMETER_INDIZES::chain_blocking_update_flag, VPRO_PARAMETER_INDIZES::dst_all, NoTrigger>(0b001, DST_ADDR(0, 1, layer.seg_out_w,
                                                                                                                                         layer.seg_out_w * layer.seg_out_h));
            c_vpro_lw<6, VPRO_PARAMETER_INDIZES::nowhere, VPRO_PARAMETER_INDIZES::src1_all, NoTrigger>(0, complex_ADDR_3D(SRC_SEL_LS, RF_BIAS_BASE, 0, 0, 1));
        } else {
//    // VPRO LOADS (conv loop)
            c_vpro_lw<5, VPRO_PARAMETER_INDIZES::id, VPRO_PARAMETER_INDIZES::func, NoTrigger>(LS, FUNC_LOADS);
            c_vpro_lw<5, VPRO_PARAMETER_INDIZES::dst_all, VPRO_PARAMETER_INDIZES::x_y_z_end, NoTrigger>(0,
                                                                                                        uint32_t(kernel_x - 1) << (10 + 6) | uint32_t(kernel_y - 1) << 10 |
                                                                                                        uint32_t(layer.seg_out_w - 1));
            c_vpro_lw<5, VPRO_PARAMETER_INDIZES::chain_blocking_update_flag, VPRO_PARAMETER_INDIZES::nowhere, NoTrigger>(0b100, 0);

//    // VPRO MACH (conv loop)
            c_vpro_lw<6, VPRO_PARAMETER_INDIZES::id, VPRO_PARAMETER_INDIZES::func, NoTrigger>(L0_1, FUNC_MACH);
            c_vpro_lw<6, VPRO_PARAMETER_INDIZES::src2_all, VPRO_PARAMETER_INDIZES::x_y_z_end, NoTrigger>(SRC2_ADDR(RF_KERNEL_BASE, 1, kernel_x, 0),
                                                                                                         uint32_t(kernel_x - 1) << (10 + 6) | uint32_t(kernel_y - 1) << 10 |
                                                                                                         uint32_t(layer.seg_out_w - 1));
            c_vpro_lw<6, VPRO_PARAMETER_INDIZES::chain_blocking_update_flag, VPRO_PARAMETER_INDIZES::nowhere, NoTrigger>(0b001, 0);
        }        
    } else if (layer.type == LAYERTYPE::CONV2_TRANSPOSE) {
        // VPRO loads
        c_vpro_lw<5, VPRO_PARAMETER_INDIZES::id, VPRO_PARAMETER_INDIZES::func, NoTrigger>(LS, FUNC_LOADS);
        c_vpro_lw<5, VPRO_PARAMETER_INDIZES::chain_blocking_update_flag, VPRO_PARAMETER_INDIZES::src1_all, NoTrigger>
            (0b100, SRC1_ADDR(0, 1, layer.seg_in_w, 1));

        // VPRO mach
        c_vpro_lw<6, VPRO_PARAMETER_INDIZES::id, VPRO_PARAMETER_INDIZES::func, NoTrigger>(L0_1, FUNC_MACH);
        c_vpro_lw<6, VPRO_PARAMETER_INDIZES::chain_blocking_update_flag, VPRO_PARAMETER_INDIZES::nowhere, NoTrigger>
            (0b001, 0);
    }
}

#endif
