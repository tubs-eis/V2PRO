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
//
// Created by gesper on 08.08.22.
//

#ifndef FIR_FIR_H
#define FIR_FIR_H

#include <cstdint>

constexpr int input_size = 1200;
constexpr int coefficients = 16;

extern int16_t input_data[input_size + 2 * coefficients] __attribute__ ((section (".vpro")));
extern int16_t output_data[input_size + 2 * coefficients] __attribute__ ((section (".vpro")));
extern int16_t h[coefficients] __attribute__ ((section (".vpro")));
extern int32_t reference_data[input_size + 2 * coefficients];
extern int16_t zeros[8192] __attribute__ ((section (".vpro")));

constexpr int back_shift_factor = 2;


/**
 *    FIR Calculation: \
 *
 *    ---------------------------------------- \
 *           i(0)  i(1)  i(2)  i(3)  i(4)  ... \
 *             x     x     x     x      x  ... \
 *    o(0) = h(15)+h(14)+h(13)+h(12)+h(11)+... \
 *    ---------------------------------------- \
 *           i(1)  i(2)  i(3)  i(4)  i(5)  ... \
 *             x     x     x     x      x  ... \
 *    o(1) = h(15)+h(14)+h(13)+h(12)+h(11)+... \
 *    ---------------------------------------- \
 *    ... \
 *
 *    Multiplications: coefficients * input_size   = 16 * 1200   = 19200 \
 *    Additions:       coefficients-1 * input_size = 16-1 * 1200 = 18000 \
 *
 *      done in 2 parts: \
 *          600 elements (first block) loaded and multiplied with coefficients \
 *              broadcast load, L0 multiplies with c0-c7, L1 multiplies with c8-c15 \
 *          add L0 to L1 (8 elements offset) -> store \
 *
 *          600 elements (second block) loaded and multiplied with coefficients \
 *              broadcast load, L0 multiplies with c0-c7, L1 multiplies with c8-c15 \
 *          add first block last 8 to first 8 elements of L1 (overlap) \
 *          add L0 to L1 (8 elements offset) -> store   \
 */
void vpro_fir(uint32_t buffer_offset = 0);

#endif //FIR_FIR_H
