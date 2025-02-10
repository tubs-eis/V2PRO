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


#ifndef VPRO_FUNCTIONS_H
#define VPRO_FUNCTIONS_H

#include <cstdint>
#include "bif.h"
#include <vpro.h>

namespace VPRO_CONST {
    constexpr int32_t leak[25] = {0,     0,      0,      1,     2,      3,
                                  6,     13,     26,     51,    102,    205,
                                  410,   819,    1638,   3277,  6554,   13107,
                                  26214, 52429,  104858, 209715,419430, 838861,
                                  1677722};
}

extern uint32_t RF_KERNEL_BASE;
extern uint32_t RF_BIAS_BASE;
extern uint32_t RF_RELU_6_BASE;
extern uint32_t kernel_x, kernel_y;
extern uint32_t vector_length, vector_length_compensate;

#define DST_DISCARD_2D DST_ADDR_3(RF_DISCARD_ADDR, 0, 0)
#define DST_DISCARD_3D DST_ADDR_4(RF_DISCARD_ADDR, 0, 0, 0)

// maxpool, activations etc. read more data than required instead of inserting nops between instructions; prevent uninitialized register access message
inline void initOvercalcMemSim() {
    VPRO::DIM2::PROCESSING::add(L0_1,
                                DST_ADDR(0, 1, 0),
                                SRC1_IMM_2D(0),
                                SRC2_IMM_2D(0),
                                32-1, 0);
}

inline void insertNops(int count, LANE lanes = L0_1) {
    if (!count)
        return;

    VPRO::DIM2::PROCESSING::add(lanes,
                                DST_DISCARD_2D,
                                SRC1_IMM_2D(0),
                                SRC2_IMM_2D(0),
                                count-1, 0);
}


#if not defined(SIMULATION) and RV_VPRO_EXT == 1
void create_conv_template_functions(const BIF::LAYER &layer);
#endif

#endif // VPRO_FUNCTIONS_H
