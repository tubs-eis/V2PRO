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
#ifndef ACTIVATION_KERNEL_H
#define ACTIVATION_KERNEL_H

#include "bif.h"
#include "vpro_functions.h"
#include <vpro.h>
#include "eisv.h"
#include "activation_sigmoid_kernel.h"
#include "store_kernel.h"

using namespace BIF;

// VPRO pipeline compensation
// --------------------------
// Most activations reqire a sequence of multiple instructions.
// Problem with small data blocks: NOPs between instructions required.
// Solution: netgen increases zend, yend or xend
// ==> wait cycles appended to previous instruction, unwanted writes go to unused memory

inline void _printdbg(const char *func, const char *file, const int line, const char *msg) {
#ifdef SIMULATION
    printf("\n================== %s\n%s:%d: %s\n", msg, file, line, func);
    debug |= DEBUG_USER_DUMP;
    int cluster = 0;
    int unit = 0;
    int lane = 0;

    vpro_lane_sync();
    //                    sim_dump_local_memory(0, 0);
    sim_dump_register_file(cluster, unit, lane);
    //core_->clusters[cluster]->dumpRegisterFile(unit, lane);
    //core_->clusters[cluster]->dumpRegisterFile(unit, lane); // private
}
#define printdbg(MSG) _printdbg(__PRETTY_FUNCTION__, __FILE__, __LINE__, MSG)
#else
}
#define printdbg(MSG)
#endif


inline void _maxpool2x2(uint32_t xend, uint32_t yend, uint32_t zend, uint32_t ch_stride, uint32_t rf_base = 0, LANE lanes = L0_1) {
    int w = xend+1;
    // int h = yend+1;
    // 2x2 max pooling
    // input mem layout: [h][w]
    // output mem layout: [h/2][w/2]

    // assert(!(w % 2));  netgen increases w or h for pipeline compensation
    // assert(!(h % 2));  -> w or h may be odd

    // col first has a smaller beta than row first (w vs. 2w) -> larger widths allowed

    // combine pairs of columns (=successive pixels), shrink image in x-dir
    VPRO::DIM3::PROCESSING::max(lanes,
                                DST_ADDR (rf_base    , 1, w >> 1, ch_stride), // store result gapless
                                SRC1_ADDR(rf_base    , 2, w     , ch_stride),
                                SRC2_ADDR(rf_base + 1, 2, w     , ch_stride),
                                xend >> 1, yend, zend,
                                false, true);

    // update w to reflect the new memory layout [h][w/2]
    uint32_t w2 = w; // speed optimization: save /2*2
    w >>= 1;

    // combine pairs of rows
    // row(n) = max(row(2n), row(2n+1)), i.e. 0=max(0,1), 1=max(2,3), 2=max(4,5) ...
    VPRO::DIM3::PROCESSING::max(lanes,
                                DST_ADDR (rf_base    , 1, w , ch_stride),
                                SRC1_ADDR(rf_base    , 1, w2, ch_stride),
                                SRC2_ADDR(rf_base + w, 1, w2, ch_stride),
                                w - 1, yend >> 1, zend,
                                false, true);
}

inline void _act_relu(uint32_t xend, uint32_t yend, uint32_t zend, uint32_t ch_stride, uint32_t rf_base = 0, LANE lanes = L0_1) {
    VPRO::DIM3::PROCESSING::max(lanes,
                                DST_ADDR (rf_base, 1, xend+1, ch_stride),
                                SRC1_ADDR(rf_base, 1, xend+1, ch_stride),
                                SRC2_IMM_3D(0),
                                xend, yend, zend);
}

inline void _relu_6_load(const int32_t shift_value, LANE lanes = L0_1) {
    VPRO::DIM3::PROCESSING::mull(lanes,
                                 DST_ADDR(RF_RELU_6_BASE, 0, 0, 0),
                                 SRC1_IMM_3D(1 << shift_value), // FIXME immediate too large for large shift_values?
                                 SRC2_IMM_3D(6),
                                 0, 0, 0);
}

inline void _act_relu6(uint32_t xend, uint32_t yend, uint32_t zend, uint32_t ch_stride, uint32_t rf_base = 0, LANE lanes = L0_1) {
    _act_relu(xend, yend, zend, ch_stride, rf_base, lanes);
    VPRO::DIM3::PROCESSING::min(lanes,
                                DST_ADDR (rf_base, 1, xend+1, ch_stride),
                                SRC1_ADDR(rf_base, 1, xend+1, ch_stride),
                                SRC2_ADDR(RF_RELU_6_BASE, 0, 0, 0),
                                xend, yend, zend);
}

inline void _act_leakyrelu(int32_t alpha, uint32_t xend, uint32_t yend, uint32_t zend, uint32_t ch_stride, uint32_t rf_base = 0, LANE lanes = L0_1) {
    if (alpha == 0) { alpha = VPRO_CONST::leak[18]; } // FIXME why? move to netgen
    VPRO::DIM3::PROCESSING::mulh_neg(lanes,
                                     DST_ADDR (rf_base, 1, xend+1, ch_stride),
                                     SRC1_ADDR(rf_base, 1, xend+1, ch_stride),
                                     SRC2_IMM_3D(alpha),
                                     xend, yend, zend);
}

inline void _act_sigmoid_fast(uint32_t rf_frac_bits, uint32_t xend, uint32_t yend, uint32_t zend, uint32_t ch_stride, uint32_t rf_base = 0, LANE lanes = L0_1) {
  // Calculation of sigmoid approximation in registerfile
  // sim_printf("rf_frac_bits: %d, store_shift_right: %d -> %d\n", rf_frac_bits, layer.store_shift_right, rf_frac_bits - layer.store_shift_right);
    if (rf_frac_bits > 14) {
        int w = xend+1;
        VPRO::DIM3::PROCESSING::shift_ar(lanes,
                                         DST_ADDR (rf_base, 1, w, ch_stride),
                                         SRC1_ADDR(rf_base, 1, w, ch_stride),
                                         SRC2_IMM_3D(rf_frac_bits - 14),
                                         xend, yend, zend);
        _sigmoid_fast<14>(xend, yend, zend, ch_stride, rf_base, lanes);
    } else {
        switch (rf_frac_bits) {
        case 10:
            _sigmoid_fast<10>(xend, yend, zend, ch_stride, rf_base, lanes);
            break;
        case 11:
            _sigmoid_fast<11>(xend, yend, zend, ch_stride, rf_base, lanes);
            break;
        case 12:
            _sigmoid_fast<12>(xend, yend, zend, ch_stride, rf_base, lanes);
            break;
        case 13:
            _sigmoid_fast<13>(xend, yend, zend, ch_stride, rf_base, lanes);
            break;
        case 14:
            _sigmoid_fast<14>(xend, yend, zend, ch_stride, rf_base, lanes);
            break;
            // maximum as mulh (x*x) only allows 18-bit for opb
        default:
            printf_error("Error rf_frac_bits < 10. Sigmoid approximation very inaccurate when rf_frac_bits < 10.\n");
        }
    }
}

//inline void _act_sigmoid_medium(lane_mem_layout &ml) {
//    //  FIXME: shift to sigmoid's .11 input bit precision...
//    auto &rf_frac_bits = vpro.bias_load_buffer_l0;
//    VPRO::DIM3::PROCESSING::shift_ar(lanes,
//                                     DST_ADDR (ml.rf_base, 0, 0, 1),
//                                     SRC1_ADDR(ml.rf_base, 0, 0, 1),
//                                     SRC2_IMM_3D(rf_frac_bits - 11),
//                                     0, 0, w * h - 1);
//    // rf fractional bits is 11!
//    _sigmoid_medium(layer, vpro.buffer, 11, (rf_frac_bits-layer.store_shift_right));
//}

inline void _act_swish(uint32_t rf_frac_bits, int32_t shift_right, uint32_t xend, uint32_t yend, uint32_t zend, uint32_t rf_ch_stride, uint32_t lm_ch_stride, 
                        uint32_t rf_base, uint32_t lm_base, int32_t lm_lane_stride, LANE lanes = L0_1) {
    // swish(x) = x * sigmoid(x)

    // copy x from RF to LM, reduce data width
    int lm_shift_right = 24 - 16; // FIXME number of bits in RF, LM
    _shift_store(lm_shift_right, xend, yend, zend, rf_ch_stride, lm_ch_stride,
                 rf_base, lm_base, lm_lane_stride, lanes);
    
    _act_sigmoid_fast(rf_frac_bits, xend, yend, zend, rf_ch_stride, rf_base, lanes);

    // Setting up VPRO multiplication with shift and sync
    vpro_lane_sync();   // prevents still running mulh in the sigmoid calculation to use new vpro_mul_h_bit_shift
    vpro_mul_h_bit_shift(shift_right); //n_frac_bits - output_frac_bits); // FIXME move computation to netgen, use shift_right instead

    // Loading temporary saved x from local memory and multiply it with register file values (x * sigmoid(x))
    int w = xend + 1;
    if (lanes & L0) { // LANE_0
        VPRO::DIM3::LOADSTORE::loads(lm_base,
                                     0, 1, w, lm_ch_stride,
                                     xend, yend, zend);

        VPRO::DIM3::PROCESSING::mulh(L0,
                                     DST_ADDR (0, 1, w, rf_ch_stride),
                                     SRC1_ADDR(0, 1, w, rf_ch_stride),
                                     SRC2_LS_3D,
                                     xend, yend, zend,
                                     false, false, true);
    }
    if (lanes & L1) { // LANE_1
        VPRO::DIM3::LOADSTORE::loads(lm_base + lm_lane_stride,
                                     0, 1, w, lm_ch_stride,
                                     xend, yend, zend);

        VPRO::DIM3::PROCESSING::mulh(L1,
                                     DST_ADDR (0, 1, w, rf_ch_stride),
                                     SRC1_ADDR(0, 1, w, rf_ch_stride),
                                     SRC2_LS_3D,
                                     xend, yend, zend,
                                     false, false, true);
    }
}

#endif // ACTIVATION_KERNEL_H
