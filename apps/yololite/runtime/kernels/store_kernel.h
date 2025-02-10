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
#ifndef STORE_KERNEL_H
#define STORE_KERNEL_H

#include "bif.h"
#include "vpro_functions.h"
#include <vpro.h>
#include "eisv.h"


inline void _shift_store(int shift_right, uint32_t xend, uint32_t yend, uint32_t zend,
                         uint32_t rf_ch_stride, uint32_t lm_ch_stride,
                         uint32_t rf_base, uint32_t lm_base, int32_t lm_lane_stride, LANE lanes) {

    if (lanes & L0) {
        if (shift_right > 0) {
            VPRO::DIM3::PROCESSING::shift_ar(L0,
                                             DST_DISCARD_3D,
                                             SRC1_ADDR(rf_base, 1, xend+1, rf_ch_stride),
                                             SRC2_IMM_3D(shift_right),
                                             xend, yend, zend,
                                             true);
        } else {
            VPRO::DIM3::PROCESSING::mull(L0,
                                         DST_DISCARD_3D,
                                         SRC1_ADDR(rf_base, 1, xend+1, rf_ch_stride),
                                         SRC2_IMM_3D(1 << (-shift_right)),
                                         xend, yend, zend,
                                         true);
        }
        VPRO::DIM3::LOADSTORE::store(lm_base, 0,
                                     1, xend+1, lm_ch_stride,
                                     xend, yend, zend,
                                     L0);
    }

    if (lanes & L1) {
        if (shift_right > 0) {
            VPRO::DIM3::PROCESSING::shift_ar(L1,
                                             DST_DISCARD_3D,
                                             SRC1_ADDR(rf_base, 1, xend+1, rf_ch_stride),
                                             SRC2_IMM_3D(shift_right),
                                             xend, yend, zend,
                                             true);
        } else {
            VPRO::DIM3::PROCESSING::mull(L1,
                                         DST_DISCARD_3D,
                                         SRC1_ADDR(rf_base, 1, xend+1, rf_ch_stride),
                                         SRC2_IMM_3D(1 << (-shift_right)),
                                         xend, yend, zend,
                                         true);
        }
        VPRO::DIM3::LOADSTORE::store(lm_base + lm_lane_stride, 0,
                                     1, xend+1, lm_ch_stride,
                                     xend, yend, zend,
                                     L1);
    }
}

inline void _shift_store_upsample(int shift_right, uint32_t xend, uint32_t yend, uint32_t zend,
                         uint32_t rf_ch_stride, uint32_t lm_ch_stride,
                         uint32_t rf_base, uint32_t lm_base, int32_t lm_lane_stride, LANE lanes) {

    assert(zend == 0);

    // FIXME can this be rewritten with 4*(xend + 1) or similar as gamma (i.e. swap yend/zend, beta/gamma)?
    // Would allow larger segment sizes and remove the corresponding condition in getSegmentSize()
    if (lanes & L0) {
        // read result from RF, repeat 4 times: [4][h][w]
        if (shift_right > 0) {
            VPRO::DIM3::PROCESSING::shift_ar(L0,
                                             DST_DISCARD_3D,
                                             SRC1_ADDR(rf_base, 1, xend + 1, 0),
                                             SRC2_IMM_3D(shift_right),
                                             xend, yend, 3,
                                             true);
        } else {
            VPRO::DIM3::PROCESSING::mull(L0,
                                         DST_DISCARD_3D,
                                         SRC1_ADDR(rf_base, 1, xend + 1, 0),
                                         SRC2_IMM_3D(1 << (-shift_right)),
                                         xend, yend, 3,
                                         true);
        }
        // write top row to LM
        VPRO::DIM3::LOADSTORE::store(lm_base,
                                     0, 2, 4*(xend + 1), 1,
                                     xend, yend, 1,
                                     L0);
        // write bottom row to LM
        VPRO::DIM3::LOADSTORE::store(lm_base,
                                     2*(xend + 1), 2, 4*(xend + 1), 1,
                                     xend, yend, 1,
                                     L0);
    }

    if (lanes & L1) {
        if (shift_right > 0) {
            VPRO::DIM3::PROCESSING::shift_ar(L1,
                                             DST_DISCARD_3D,
                                             SRC1_ADDR(rf_base, 1, xend + 1, 0),
                                             SRC2_IMM_3D(shift_right),
                                             xend, yend, 3,
                                             true);
        } else {
            VPRO::DIM3::PROCESSING::mull(L1,
                                         DST_DISCARD_3D,
                                         SRC1_ADDR(rf_base, 1, xend + 1, 0),
                                         SRC2_IMM_3D(1 << (-shift_right)),
                                         xend, yend, 3,
                                         true);
        }
        VPRO::DIM3::LOADSTORE::store(lm_base + lm_lane_stride,
                                     0, 2, 4*(xend + 1), 1,
                                     xend, yend, 1,
                                     L1);
        VPRO::DIM3::LOADSTORE::store(lm_base + lm_lane_stride,
                                     2*(xend + 1), 2, 4*(xend + 1), 1,
                                     xend, yend, 1,
                                     L1);
    }
}
// TODO: for kernel_size == 1, pool == 1, stride == 1 -> use merged shift_store for parallel_outchannels_per_lane instead of individual store commands (netgen fix required!)

#endif // STORE_KERNEL_H
