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
#ifndef CONV2D_KERNEL_H
#define CONV2D_KERNEL_H

#include "bif.h"
#include "vpro_functions.h"
#include <vpro.h>
#include "eisv.h"

#if RV_VPRO_EXT == 1 and not defined(SIMULATION)
using namespace VPRO_RISC_EXT_VPRO;
using namespace VPRO_RISC_EXT_DMA;
#endif

inline void _conv(const BIF::LAYER &layer, const uint16_t buffer) {
    if (layer.parallel_outchannels_per_lane > 1){   // implies kernel_size == 1, pool == 1, stride == 1

#if RV_VPRO_EXT == 1 and not defined(SIMULATION)
        c_vpro_lw<5, VPRO_PARAMETER_INDIZES::nowhere, VPRO_PARAMETER_INDIZES::src2_imm, Trigger>(0, SRC2_IMM_3D(buffer));
        c_vpro_lw<6, VPRO_PARAMETER_INDIZES::nowhere, VPRO_PARAMETER_INDIZES::src1_all, Trigger>(0, complex_ADDR_3D(SRC_SEL_LS, RF_BIAS_BASE, 0, 0, 1));
#else
        assert(layer.seg_out_w * layer.parallel_outchannels_per_lane >= 5);  // TODO: vector length compensate
        assert((unsigned int) layer.seg_out_w - 1 <= MAX_X_END);
        assert((unsigned int) layer.seg_out_h - 1 <= MAX_Y_END);
        assert((unsigned int) layer.seg_in_w <= MAX_BETA);
        assert((unsigned int) layer.seg_out_w <= MAX_BETA);
        assert((unsigned int) layer.seg_out_w*layer.seg_out_h <= MAX_GAMMA);
        VPRO::DIM3::LOADSTORE::loads(buffer, 0, 1, layer.seg_in_w, 0,
                                     layer.seg_out_w - 1,
                                     layer.seg_out_h - 1,
                                     layer.parallel_outchannels_per_lane - 1);

        VPRO::DIM3::PROCESSING::mach_init_addr(L0_1,
                                               DST_ADDR(0, 1, layer.seg_out_w, layer.seg_out_w*layer.seg_out_h),
                                               SRC1_LS_3D,
                                               SRC2_ADDR(RF_KERNEL_BASE, 0, 0, 1),    // 1015
                                               layer.seg_out_w - 1,
                                               layer.seg_out_h - 1,
                                               layer.parallel_outchannels_per_lane - 1,
                                               RF_BIAS_BASE, 0, 0, 1,  // 1014
                                               false, true);
#endif
    } else if (kernel_x == 1) {
        assert((unsigned int) layer.seg_out_w - 1 <= MAX_X_END);
        assert((unsigned int) layer.seg_out_h - 1 <= MAX_Y_END);
        assert((unsigned int) layer.stride*layer.seg_in_w <= MAX_BETA);
        assert((unsigned int) layer.seg_out_w <= MAX_BETA);
        VPRO::DIM2::LOADSTORE::loads(buffer, 0, layer.stride, layer.stride*layer.seg_in_w,
                                     layer.seg_out_w - 1, layer.seg_out_h - 1);

        VPRO::DIM3::PROCESSING::mach_init_addr(L0_1,
                                               DST_ADDR(0, 1, layer.seg_out_w, 0),
                                               SRC1_LS_3D,
                                               SRC2_ADDR(RF_KERNEL_BASE, 0, 0, 0),    // 1015
                                               layer.seg_out_w - 1, layer.seg_out_h - 1, 0,
                                               RF_BIAS_BASE, 0, 0, 0,  // 1014
                                               false, true);

        insertNops(vector_length_compensate);

    } else {
        auto offset_in = 0;
        auto offset_out = 0;

        // FIXME speed optimization: why sometimes use local copies and sometimes struct vars?
        auto in_w = layer.seg_in_w;
        auto out_w = layer.seg_out_w;
        auto out_h = layer.seg_out_h;
#if RV_VPRO_EXT == 1 and not defined(SIMULATION)
        auto stride = layer.stride;
#endif

        for (size_t y_out = 0; y_out < out_h; ++y_out) {
            // compute one line of output
            // for z = [0:layer.seg_out_w - 1]
            //   for y = [0:kernel_y-1]
            //     for x = [0:kernel_x-1]
            //       RF[y_out*out_w + z] += RF[RF_KERNEL_BASE + x + y*kernel_x] * LM[y_out*in_w*stride + x*1 + y*seg_in_w + z*stride]
#if RV_VPRO_EXT == 1 and not defined(SIMULATION)
            // implicit parameters set in create_conv_template_functions()
            c_vpro_lw<5, VPRO_PARAMETER_INDIZES::src1_all, VPRO_PARAMETER_INDIZES::src2_imm, Trigger>(
                    SRC1_ADDR(offset_in, layer.dilation_rate_w, in_w*layer.dilation_rate_w, stride), SRC2_IMM_3D(buffer));
            c_vpro_lw<6, VPRO_PARAMETER_INDIZES::dst_all, VPRO_PARAMETER_INDIZES::src1_all, Trigger>(
                    DST_ADDR(offset_out, 0, 0, 1), complex_ADDR_3D(SRC_SEL_LS, RF_BIAS_BASE, 0, 0, 0));
#else
            VPRO::DIM3::LOADSTORE::loads(buffer,
                                         offset_in, 1 * layer.dilation_rate_w, layer.seg_in_w * layer.dilation_rate_h, layer.stride, // src: offset, alpha, beta, gamma
                                         kernel_x-1, kernel_y-1, layer.seg_out_w - 1); // x_end, y_end, z_end
            VPRO::DIM3::PROCESSING::mach_init_addr(L0_1,  // shift right by 3 (?)
                                                   DST_ADDR(offset_out, 0, 0, 1),
                                                   SRC1_LS_3D,
                                                   SRC2_ADDR(RF_KERNEL_BASE, 1, kernel_x, 0),    // 1015
                                                   kernel_x-1, kernel_y-1, layer.seg_out_w - 1, // x_end, y_end, z_end
                                                   RF_BIAS_BASE, 0, 0, 0,  // 1014
                                                   false, true);
#endif
            offset_in += in_w * layer.stride; // input: stride lines down in LM per output line
            offset_out += out_w;
        }
    }
}

inline void _conv_add(const BIF::LAYER &layer, const uint16_t buffer) {
    if (layer.parallel_outchannels_per_lane > 1){   // implies kernel_size == 1, pool == 1, stride == 1

#if RV_VPRO_EXT == 1 and not defined(SIMULATION)
        c_vpro_lw<5, VPRO_PARAMETER_INDIZES::nowhere, VPRO_PARAMETER_INDIZES::src2_imm, Trigger>(0, SRC2_IMM_3D(buffer));
        c_vpro_lw<6, VPRO_PARAMETER_INDIZES::nowhere, VPRO_PARAMETER_INDIZES::src1_all, Trigger>(0, complex_ADDR_3D(SRC_SEL_LS, 0, 1, layer.seg_out_w, layer.seg_out_w*layer.seg_out_h));
#else
        assert(layer.seg_out_w * layer.parallel_outchannels_per_lane >= 5);  // TODO: vector length compensate
        assert((unsigned int) layer.seg_out_w - 1 <= MAX_X_END);
        assert((unsigned int) layer.seg_out_h - 1 <= MAX_Y_END);
        assert((unsigned int) layer.seg_in_w <= MAX_BETA);
        assert((unsigned int) layer.seg_out_w <= MAX_BETA);
        assert((unsigned int) layer.seg_out_w*layer.seg_out_h <= MAX_GAMMA);
        VPRO::DIM3::LOADSTORE::loads(buffer, 0, 1, layer.seg_in_w, 0,
                                     layer.seg_out_w - 1,
                                     layer.seg_out_h - 1,
                                     layer.parallel_outchannels_per_lane - 1);

        VPRO::DIM3::PROCESSING::mach_init_addr(L0_1,
                                               DST_ADDR(0, 1, layer.seg_out_w, layer.seg_out_w*layer.seg_out_h),
                                               SRC1_LS_3D,
                                               SRC2_ADDR(RF_KERNEL_BASE, 0, 0, 1),    // 1015
                                               layer.seg_out_w - 1,
                                               layer.seg_out_h - 1,
                                               layer.parallel_outchannels_per_lane - 1,
                                               0, 1, layer.seg_out_w, layer.seg_out_w*layer.seg_out_h,  // 1014
                                               false, true);
#endif
    } else if (kernel_x == 1) {
        VPRO::DIM2::LOADSTORE::loads(buffer, 0, layer.stride, layer.stride * layer.seg_in_w,
                                     layer.seg_out_w - 1, layer.seg_out_h - 1);

        VPRO::DIM3::PROCESSING::mach_init_addr(L0_1,
                                               DST_ADDR(0, 1, layer.seg_out_w, 0),
                                               SRC1_LS_3D,
                                               SRC2_ADDR(RF_KERNEL_BASE, 0, 0, 0),    // 1015
                                               layer.seg_out_w - 1, layer.seg_out_h - 1, 0,
                                               0, 1, layer.seg_out_w, 0,  // previous value
                                               false, true);

        insertNops(vector_length_compensate);
        
    } else { // kernel > 1, kernel build vector
        auto offset_in = 0;
        auto offset_out = 0;

        auto in_w = layer.seg_in_w;
        auto out_w = layer.seg_out_w;
        auto out_h = layer.seg_out_h;
#if RV_VPRO_EXT == 1 and not defined(SIMULATION)
        auto stride = layer.stride;
#endif

        for (size_t y = 0; y < out_h; ++y) {
#if RV_VPRO_EXT == 1 and not defined(SIMULATION)
            c_vpro_lw<5, VPRO_PARAMETER_INDIZES::src1_all, VPRO_PARAMETER_INDIZES::src2_imm, Trigger>(
                    SRC1_ADDR(offset_in, layer.dilation_rate_w, in_w*layer.dilation_rate_w, stride), SRC2_IMM_3D(buffer));
            c_vpro_lw<6, VPRO_PARAMETER_INDIZES::dst_all, VPRO_PARAMETER_INDIZES::src1_all, Trigger>(
                    DST_ADDR(offset_out, 0, 0, 1), complex_ADDR_3D(SRC_SEL_LS, offset_out, 0, 0, 1));
#else
            VPRO::DIM3::LOADSTORE::loads(buffer,
                                         offset_in, 1 * layer.dilation_rate_w, layer.seg_in_w * layer.dilation_rate_h, layer.stride,
                                         kernel_x-1, kernel_y-1, layer.seg_out_w - 1);
            VPRO::DIM3::PROCESSING::mach_init_addr(L0_1,  // shift right by 3
                                                   DST_ADDR(offset_out, 0, 0, 1),
                                                   SRC1_LS_3D,
                                                   SRC2_ADDR(RF_KERNEL_BASE, 1, kernel_x, 0),    // 1015
                                                   kernel_x-1, kernel_y-1, layer.seg_out_w - 1,
                                                   offset_out, 0, 0, 1,  // previous value ( = dst )
                                                   false, true);    // update flags
#endif
            offset_in += in_w * layer.stride;
            offset_out += out_w;
        }
    }
}

#endif // CONV2D_KERNEL_H
