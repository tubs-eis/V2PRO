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
#ifndef DCONV_CONV_KERNEL_H
#define DCONV_CONV_KERNEL_H

#include "bif.h"
#include "vpro_functions.h"
#include <vpro.h>
#include "eisv.h"

inline void _dconv_conv(const BIF::LAYER &layer, const uint16_t buffer) {
    assert(layer.stride == 1);
    auto offset_in = buffer;
    auto offset_out = 0;

    auto in_w = layer.seg_in_w;
    auto out_w = layer.seg_out_w;
    auto out_h = layer.seg_out_h;

    for (size_t y = 0; y < out_h; ++y) {
        VPRO::DIM3::LOADSTORE::loads(offset_in,
                                        0, 1, 0, kernel_x,
                                        kernel_x - 1, 0, layer.seg_out_w - 1);
        VPRO::DIM3::PROCESSING::mach_init_addr(L0_1,  // shift right by 3
                                                DST_ADDR(offset_out, 0, 0, 1),
                                                SRC1_LS_3D,
                                                SRC2_ADDR(RF_KERNEL_BASE, 1, 0, 0),    // 1015
                                                kernel_x - 1, 0, layer.seg_out_w - 1,
                                                RF_BIAS_BASE, 0, 0, 0,  // 1014
                                                false, true);
        offset_in += in_w;
        offset_out += out_w;
    }
}

inline void _dconv_conv_add(const BIF::LAYER &layer, const uint16_t buffer) {
    assert(layer.stride == 1);
    auto offset_in = buffer;
    auto offset_out = 0;

    auto in_w = layer.seg_in_w;
    auto out_w = layer.seg_out_w;
    auto out_h = layer.seg_out_h;

    for (size_t y = 0; y < out_h; ++y) {
        VPRO::DIM3::LOADSTORE::loads(offset_in,
                                        0, 1, 0, kernel_x,
                                        kernel_x - 1, 0, layer.seg_out_w - 1);
        VPRO::DIM3::PROCESSING::mach_init_addr(L0_1,  // shift right by 3
                                                DST_ADDR(offset_out, 0, 0, 1),
                                                SRC1_LS_3D,
                                                SRC2_ADDR(RF_KERNEL_BASE, 1, 0, 0),    // 1015
                                                kernel_x - 1, 0, layer.seg_out_w - 1,
                                                offset_out, 0, 0, 1,  // previous value ( = dst )
                                                false, true);    // update flags
        offset_in += in_w;
        offset_out += out_w;
    }
}

#endif // CONV2D_KERNEL_H
