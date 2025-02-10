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
#ifndef CONCAT_KERNEL_H
#define CONCAT_KERNEL_H

#include "bif.h"
#include "vpro_functions.h"
#include <vpro.h>
#include "eisv.h"





inline void _concatenate(const BIF::LAYER &layer, const uint16_t buffer, const uint16_t out_buffer, const uint32_t x_end, const uint32_t y_end,
                            uint32_t right_shift) {
    
    VPRO::DIM2::LOADSTORE::loads(
        buffer, 
        0, 
        1, 
        layer.seg_in_w, 
        x_end, 
        y_end);

    VPRO::DIM2::PROCESSING::shift_ar(
        L0,
        DST_ADDR(0, 1, layer.seg_in_w),
        SRC1_LS_2D,
        SRC2_IMM_2D(right_shift),
        x_end,
        y_end);

    VPRO::DIM2::PROCESSING::add(
        L0,
        DST_DISCARD_2D,
        SRC1_ADDR(0, 1, layer.seg_in_w),
        SRC2_IMM_2D(0),
        x_end,
        y_end, 
        true);

    VPRO::DIM2::LOADSTORE::store(
        out_buffer,
        0, 
        1, 
        layer.seg_in_w,
        x_end,
        y_end, 
        L0);
}

#endif
