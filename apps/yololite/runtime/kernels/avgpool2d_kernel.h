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
#ifndef AVGPOOL2D_KERNEL_H
#define AVGPOOL2D_KERNEL_H

#include "bif.h"
#include "vpro_functions.h"
#include <vpro.h>
#include "eisv.h"

// inline void _avgpool2d(const BIF::LAYER &layer, const uint16_t buffer, const uint32_t x_end, const uint32_t y_end) {
//     VPRO::DIM2::LOADSTORE::loads(buffer, 0, layer.stride, layer.stride*layer.seg_in_w,
//                                      layer.seg_out_w - 1, layer.seg_out_h - 1);

inline void _avgpool2d_load_kernel(const BIF::LAYER &layer, const uint16_t buffer, const uint16_t lane, 
    const uint16_t kernel_load_buffer, const uint32_t x_end, const uint32_t y_end) {
        VPRO::DIM2::LOADSTORE::loads(kernel_load_buffer, 
                                        0, 
                                        1, 
                                        layer.seg_out_w,
                                        layer.seg_out_w - 1, 
                                        layer.seg_out_h - 1);
        // pool factors and results are stored in rf, input is chained to mac
        // dst offs=1 for storing pool factors, so first mac output does not overwrite its own pool factor
        VPRO::DIM2::PROCESSING::add(L0,
                                        DST_ADDR(1, 1, layer.seg_out_w), 
                                        SRC1_IMM_2D(0),
                                        SRC2_LS_2D,
                                        layer.seg_out_w - 1, 
                                        layer.seg_out_h - 1);


        //  add(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
        //         bool is_chain = false, bool update_flags = false, bool blocking = false
}


inline void _avgpool2d_pool(const BIF::LAYER &layer, const uint16_t buffer, const uint16_t lane, 
    const uint32_t x_end, const uint32_t y_end) {
    auto offset_in = 0;
    auto offset_out = 0;

    // FIXME speed optimization: why sometimes use local copies and sometimes struct vars?
    auto in_w = layer.seg_in_w;
    auto out_w = layer.seg_out_w;
    auto out_h = layer.seg_out_h;

    for (size_t y_out = 0; y_out < out_h; ++y_out) {
        // compute one line of output
        // for z = [0:layer.seg_out_w - 1]
        //   for y = [0:kernel_y-1]
        //     for x = [0:kernel_x-1]
        //       RF[y_out*out_w + z] += RF[RF_KERNEL_BASE + x + y*kernel_x] * LM[y_out*in_w*stride + x*1 + y*seg_in_w + z*stride]

        VPRO::DIM3::LOADSTORE::loads(buffer + offset_in, 0, 1, layer.seg_in_w, layer.pool_stride_w, // src: offset, alpha, beta, gamma
                                        layer.pool_size_w - 1, 
                                        layer.pool_size_h - 1, 
                                        layer.seg_out_w - 1); // x_end, y_end, z_end

        //  mach(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end, uint32_t z_end,
        //              bool is_chain = false, bool update_flags = false, bool blocking = false

        VPRO::DIM3::PROCESSING::mach(L0,  // shift right by 3 (?)
                                        DST_ADDR(offset_out, 0, 0, 1),
                                        SRC1_LS_3D,
                                        SRC2_ADDR(offset_out + 1, 0, 0, 1),    // 1015
                                        layer.pool_size_w - 1, 
                                        layer.pool_size_h - 1, 
                                        layer.seg_out_w - 1,
                                        false, false, true); // x_end, y_end, z_end
                                        
        offset_in += in_w * layer.pool_stride_h; // input: stride lines down in LM per output line
        offset_out += out_w;
    }
}


inline void _avgpool2d_store(const BIF::LAYER &layer, const uint16_t buffer, const uint16_t lane,  // FIXME replace by standard shift_store
    const uint32_t x_end, const uint32_t y_end) {

    if(layer.store_shift_right >= 0) {
        VPRO::DIM2::PROCESSING::add(L0,
                                        DST_DISCARD_2D,
                                        SRC1_IMM_2D(layer.store_shift_right),
                                        SRC2_ADDR(0, 1, layer.seg_out_w),
                                        layer.seg_out_w - 1, 
                                        layer.seg_out_h - 1,
                                        true);
    } else {
        VPRO::DIM2::PROCESSING::mull(L0,
                                        DST_DISCARD_2D,
                                        SRC1_IMM_2D(1 << (-layer.store_shift_right)),
                                        SRC2_ADDR(0, 1, layer.seg_out_w),
                                        layer.seg_out_w - 1, 
                                        layer.seg_out_h - 1,
                                        true);
    }
        VPRO::DIM2::LOADSTORE::store(buffer, 
                                        0,
                                        1, 
                                        layer.seg_out_w,
                                        layer.seg_out_w - 1, 
                                        layer.seg_out_h - 1,
                                        L0);
   
}



// global average pooling MAC accu and intermediate data layout
//
//        4         3         2         1         0
// 765432109876543210987654321098765432109876543210 bit value 2^x
//                                 fedcba9876543210 lm[0]: bits 15.. 0
//                 fedcba9876543210                 lm[1]: bits 31..16
// fedcba9876543210----------                       lm[2]: bits 47..32 (2 MSBs are lost during << 10 in RF)

inline void _global_avgpool2d_init_constants() {
    // RF[0..2] = (1, 2^16, 2^22); 2^22: largest representable positive power of 2
    // no nops required, does not read anything from RF
    VPRO::DIM2::PROCESSING::add(L0,
                                DST_ADDR(0, 0, 0),
                                SRC1_IMM_2D(1),
                                SRC2_IMM_2D(0),
                                0, 0);
    VPRO::DIM2::PROCESSING::add(L0,
                                DST_ADDR(1, 0, 0),
                                SRC1_IMM_2D(1<<16),
                                SRC2_IMM_2D(0),
                                0, 0);
    VPRO::DIM2::PROCESSING::mull(L0,
                                 DST_ADDR(2, 0, 0),
                                 SRC1_IMM_2D(1<<11),
                                 SRC2_IMM_2D(1<<11),
                                 0, 0);
}

// sum stays in accu
// no way to multiply full-width accu with something
inline void _global_avgpool2d_add(int xend, int yend, int zend, uint32_t lm_base) {
    // assumption: mac_reset_mode is ONCE in the first run, NEVER in all others
    // no nops required, does not read anything from RF
    VPRO::DIM3::LOADSTORE::loads(lm_base,
                                 0, 1, xend+1, zend ? (xend+1)*(yend+1) : 0,
                                 xend, yend, zend);

    VPRO::DIM3::PROCESSING::mach(L0,
                                 DST_DISCARD_3D,
                                 SRC1_LS_3D,
                                 SRC2_IMM_3D(1),
                                 xend, yend, zend);
}

// FIXME incomplete, untested, currently unused
inline void _global_avgpool2d_store_intermediates(uint32_t lm_base) {
    // assumption: mac_reset_mode == NEVER
    // no nops required, does not read anything from RF
    VPRO::DIM3::PROCESSING::macl(L0,
                                 DST_DISCARD_3D,
                                 SRC1_IMM_3D(0),
                                 SRC2_IMM_3D(0),
                                 0, 0, 0, true);

    VPRO::DIM3::LOADSTORE::store(lm_base, 0,
                                 0, 0, 0, // alpha, beta, gamma
                                 0, 0, 0, // [x,y,z]_end
                                 L0);

    vpro_lane_sync();
    vpro_mac_h_bit_shift(16);

    VPRO::DIM3::PROCESSING::mach(L0,
                                 DST_DISCARD_3D,
                                 SRC1_IMM_3D(0),
                                 SRC2_IMM_3D(0),
                                 0, 0, 0, true);

    VPRO::DIM3::LOADSTORE::store(lm_base, 1,
                                 0, 0, 0,
                                 0, 0, 0,
                                 L0);

    vpro_lane_sync();
    vpro_mac_h_bit_shift(32);

    VPRO::DIM3::PROCESSING::mach(L0,
                                 DST_DISCARD_3D,
                                 SRC1_IMM_3D(0),
                                 SRC2_IMM_3D(0),
                                 0, 0, 0, true);

    VPRO::DIM3::LOADSTORE::store(lm_base, 2,
                                 0, 0, 0,
                                 0, 0, 0,
                                 L0);

}

// sum up list of 48 bit intermediates, result in L0.accu
// FIXME incomplete, untested, currently unused
inline void _global_avgpool2d_sum_intermediates(int n_summands, uint32_t lm_base) {
    // no nops required, does not read anything from RF

    vpro_lane_sync();
    vpro_set_mac_reset_mode(VPRO::MAC_RESET_MODE::ONCE);

    // L0.accu += LM[3*i]*1 + LM[3*i+1]*2^16 for i = 0..n_summands-1
    VPRO::DIM3::LOADSTORE::load(lm_base, 0, // unsigned, no sign extension 16->24 bit
                                 1, 0, 3,
                                 1, 0, n_summands-1);

    VPRO::DIM3::PROCESSING::mach(L0,
                                 DST_DISCARD_3D,
                                 SRC1_LS_3D,
                                 SRC2_ADDR(0, 1, 0, 0), // constants 1, 2^16
                                 1, 0, n_summands-1,
                                 false, false, true);

    vpro_lane_sync();
    vpro_set_mac_reset_mode(VPRO::MAC_RESET_MODE::NEVER);

    // L0.accu += LM[3*i+2]*2^32 for i = 0..n_summands-1
    VPRO::DIM3::LOADSTORE::loads(lm_base, 2, // signed, copy sign bit from [15] to [23..16]
                                  0, 0, 3,
                                  0, 0, n_summands-1);
    // RF[2] can only encode unsigned 2^22 -> multiply data with 2^10 in chained L1 -> 2 MSBs of LM[3*i+2] are discarded
    VPRO::DIM3::PROCESSING::mull(L1,
                                 DST_DISCARD_3D,
                                 SRC1_IMM_3D(1 << (32-22)),
                                 SRC2_LS_3D,
                                 0, 0, n_summands-1,
                                 true); // continue chain
    VPRO::DIM3::PROCESSING::mulh(L0,
                                 DST_DISCARD_3D,
                                 SRC1_CHAINING_LEFT_3D, // input 1
                                 SRC2_ADDR(2, 0, 0, 0), // input 0; constant 2^22
                                 0, 0, n_summands-1);
}


inline void _global_avgpool2d_divide(int pre_shift_right, int multiplier, int store_shift_right, uint32_t lm_base) {
    vpro_lane_sync();
    vpro_mac_h_bit_shift(pre_shift_right);
    vpro_mul_h_bit_shift(store_shift_right);

    // store 24 bits from accu to RF[3]
    VPRO::DIM2::PROCESSING::mach(L0,
                                 DST_ADDR(3, 0, 0),
                                 SRC1_IMM_2D(0),
                                 SRC2_IMM_2D(0),
                                 0, 0);

    insertNops(W2R_BUBBLE_CYCLES);

    // multiply RF[3] with multiplier, shift right, chain result to LM[0]
    VPRO::DIM2::PROCESSING::mulh(L0,
                                 DST_DISCARD_2D,
                                 SRC1_ADDR(3, 0, 0),
                                 SRC2_IMM_2D(multiplier),
                                 0, 0,
                                 true); // chaining
    

    VPRO::DIM2::LOADSTORE::store(lm_base, 0,
                                 0, 0,
                                 0, 0,
                                 L0);
}

#endif //AVGPOOL2D_KERNEL_H
