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
#ifndef LOAD_KERNEL_H
#define LOAD_KERNEL_H

#include "bif.h"
#include "vpro_functions.h"
#include <vpro.h>
#include "eisv.h"

inline void _kernel_load_right(const BIF::LAYER &layer, const uint16_t &kernel_base, const uint8_t &lane, const int shift) {
#if RV_VPRO_EXT == 1 and not defined(SIMULATION)
    c_vpro_lw<2, VPRO_PARAMETER_INDIZES::src2_imm, VPRO_PARAMETER_INDIZES::nowhere, Trigger>(SRC2_IMM_3D(kernel_base), 0);
    c_vpro_lw<3, VPRO_PARAMETER_INDIZES::id, VPRO_PARAMETER_INDIZES::nowhere, Trigger>((lane == 0) ? L0 : L1, 0);
#else
    if (layer.parallel_outchannels_per_lane > 1){   // implies kernel_size == 1, pool == 1, stride == 1
        VPRO::DIM3::LOADSTORE::loads(kernel_base,
                                     0, 1, kernel_y, kernel_x*kernel_y,
                                     kernel_x - 1, kernel_y - 1, layer.parallel_outchannels_per_lane - 1);

        VPRO::DIM3::PROCESSING::shift_ar((lane == 0) ? L0 : L1,
                                         DST_ADDR(RF_KERNEL_BASE, 1, kernel_y, kernel_x*kernel_y),
                                         SRC1_LS_3D,
                                         SRC2_IMM_3D(shift),
                                         kernel_x - 1,
                                         kernel_y - 1,
                                         layer.parallel_outchannels_per_lane - 1,
                                         false, false, true);
    } else {
        VPRO::DIM2::LOADSTORE::loads(kernel_base, 0, 1, kernel_y,
                                     kernel_x - 1, kernel_y - 1);

        VPRO::DIM2::PROCESSING::shift_ar((lane == 0) ? L0 : L1,
                                         DST_ADDR(RF_KERNEL_BASE, 1, kernel_y),
                                         SRC1_LS_2D,
                                         SRC2_IMM_2D(shift),
                                         kernel_x - 1,
                                         kernel_y - 1,
                                         false, false, true);
    }
#endif
}

inline void _bias_load(const BIF::LAYER &layer, const uint16_t &bias_base, const uint8_t lane){
#if RV_VPRO_EXT == 1 and not defined(SIMULATION)
    c_vpro_lw<0, VPRO_PARAMETER_INDIZES::src2_imm, VPRO_PARAMETER_INDIZES::nowhere, Trigger>(SRC2_IMM_3D(bias_base), 0);
    c_vpro_lw<1, VPRO_PARAMETER_INDIZES::id, VPRO_PARAMETER_INDIZES::nowhere, Trigger>((lane == 0) ? L0 : L1, 0);
#else
    if (layer.parallel_outchannels_per_lane > 1){   // implies kernel_size == 1, pool == 1, stride == 1
        VPRO::DIM3::LOADSTORE::loads(bias_base,
                                     0, 0, 0, 1,
                                     0, 0, layer.parallel_outchannels_per_lane - 1);

        if (layer.bias_shift_right >= 0){
            VPRO::DIM3::PROCESSING::shift_ar((lane == 0) ? L0 : L1,
                                             DST_ADDR(RF_BIAS_BASE, 0, 0, 1),
                                             SRC1_LS_3D,
                                             SRC2_IMM_3D(layer.bias_shift_right),
                                             0, 0, layer.parallel_outchannels_per_lane - 1,
                                             false, false, true);
        } else {
            VPRO::DIM3::PROCESSING::mull((lane == 0) ? L0 : L1,
                                         DST_ADDR(RF_BIAS_BASE, 0, 0, 1),
                                         SRC1_LS_3D,
                                         SRC2_IMM_3D(1u << (-layer.bias_shift_right)),
                                         0, 0, layer.parallel_outchannels_per_lane - 1,
                                         false, false, true);
        }
    } else {
        //    __load_shift_left(LS, LM_BIAS_BASE, 0, 0, 0, 0, 0, 0);
        VPRO::DIM2::LOADSTORE::loads(bias_base, 0, 0, 0, 0, 0);
        if (layer.bias_shift_right >= 0){
            VPRO::DIM2::PROCESSING::shift_ar((lane == 0) ? L0 : L1,
                                             DST_ADDR(RF_BIAS_BASE, 0, 0),
                                             SRC1_LS_2D,
                                             SRC2_IMM_2D(layer.bias_shift_right),
                                             0, 0,
                                             false, false, true);
        } else {
            VPRO::DIM2::PROCESSING::mull((lane == 0) ? L0 : L1,
                                         DST_ADDR(RF_BIAS_BASE, 0, 0),
                                         SRC1_IMM_2D(1u << (-layer.bias_shift_right)),
                                         SRC2_LS_2D,
                                         0, 0,
                                         false, false, true);
        }
        //    TODO: eval HW execution (MIPS shift always?) vs load_shift_left with -layer.bias_shift_right as shift value
    }
#endif
}
#endif // LOAD_KERNEL_H
