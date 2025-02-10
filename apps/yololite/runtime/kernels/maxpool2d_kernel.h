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
#ifndef MAXPOOL2D_KERNEL_H
#define MAXPOOL2D_KERNEL_H

#include "bif.h"
#include "vpro_functions.h"
#include <vpro.h>
#include "eisv.h"

#if RV_VPRO_EXT == 1 and not defined(SIMULATION)
using namespace VPRO_RISC_EXT_VPRO;
using namespace VPRO_RISC_EXT_DMA;
#endif

inline void _maxpool2d_vector(const BIF::LAYER &layer, const uint16_t buffer) {
    auto offset_in = 0;
    auto offset_out = 0;

    for (size_t y_out = 0; y_out < layer.seg_out_h; ++y_out) {
        // compute one line of output
        // for z = [0:layer.seg_out_w - 1]
        //   for y = [0:kernel_y-1]
        //     for x = [0:kernel_x-1]
        //       chained = LM[y_out*in_w*stride + x*dilation_rate_w + y*seg_in_w*dilation_rate_h + z*stride]
        VPRO::DIM3::LOADSTORE::loads(buffer,
                                     offset_in, layer.dilation_rate_w, layer.seg_in_w * layer.dilation_rate_h, layer.stride, // src: offset, alpha, beta, gamma
                                     kernel_x-1, kernel_y-1, layer.seg_out_w - 1); // x_end, y_end, z_end

        for (int x_out = 0; x_out <= layer.seg_out_w - 1; x_out++) {
            VPRO::DIM3::PROCESSING::max_vector_value(L0_1,
                                                     DST_ADDR(offset_out, 0, 0, 1),
                                                     SRC1_LS_3D,
                                                     kernel_x-1, kernel_y-1, 0, // x_end, y_end, z_end
                                                     false, true);
            offset_out++;
        }
        offset_in += layer.seg_in_w * layer.stride; // input: stride lines down in LM per output line
    }
}

inline void _maxpool2d(const BIF::LAYER &layer, const uint16_t buffer) {
    for (unsigned int in_ofs_y = 0; in_ofs_y < kernel_y; in_ofs_y++) {
        VPRO::DIM3::LOADSTORE::loads(buffer + in_ofs_y * layer.seg_in_w, 0,
                                     layer.stride, layer.seg_in_w*layer.stride, 1, // src: offset, alpha, beta, gamma
                                     layer.seg_out_w-1, layer.seg_out_h-1, kernel_x-1); // x_end, y_end, z_end

        unsigned int in_ofs_x = 0;
        if (in_ofs_y == 0) {
            // 1st element: copy instead of max
            VPRO::DIM3::PROCESSING::add(L0_1,
                                        DST_ADDR(0, 1, layer.seg_out_w, 0),
                                        SRC1_LS_3D,
                                        SRC2_IMM_3D(0),
                                        layer.seg_out_w-1, layer.seg_out_h-1, 0, // x_end, y_end, z_end
                                        false, true);
            in_ofs_x++;
        }

        for (; in_ofs_x < kernel_x; in_ofs_x++) {
            VPRO::DIM3::PROCESSING::max(L0_1,
                                        DST_ADDR(0, 1, layer.seg_out_w, 0),
                                        SRC1_LS_3D,
                                        SRC2_ADDR(0, 1, layer.seg_out_w, 0),
                                        layer.seg_out_w-1, layer.seg_out_h-1, 0, // x_end, y_end, z_end
                                        false, true);
        }
    }
}


#endif // MAXPOOL2D_KERNEL_H
