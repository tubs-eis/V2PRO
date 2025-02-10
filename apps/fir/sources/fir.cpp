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

#include "fir.h"

#include <vpro.h>

int16_t __attribute__ ((section (".vpro"))) input_data[input_size + 2 * coefficients]{};
int16_t __attribute__ ((section (".vpro"))) output_data[input_size + 2 * coefficients]{};
int16_t __attribute__ ((section (".vpro"))) h[coefficients]{};
int32_t reference_data[input_size + 2 * coefficients]{};
int16_t __attribute__ ((section (".vpro"))) zeros[8192]{};

constexpr uint32_t block_size = input_size/2;

void vpro_fir(uint32_t buffer_offset){

    // complex addressing uses 600 blocks
    assert(block_size == 600);

    // FIR
    VPRO::DIM3::PROCESSING::mach_init_imm(L0,
                                          DST_ADDR(0, 0, 1, 60), SRC1_LS_3D, SRC2_ADDR(1024 - 8, 1, 0, 0),
                                          7, 59, 9,
                                          0);
    VPRO::DIM3::PROCESSING::mach_init_imm(L1,
                                          DST_ADDR(0, 0, 1, 60), SRC1_LS_3D, SRC2_ADDR(1024 - 8, 1, 0, 0),
                                          7, 59, 9,
                                          0);
    VPRO::DIM3::LOADSTORE::loads(buffer_offset,
                                 0, 1, 1, 60,
                                 7, 59, 9);

    // add both partial results (+8 index offset in L1)
    VPRO::DIM2::PROCESSING::add(L0,
                                DST_ADDR(0, 1, 60), SRC1_ADDR(0, 1, 60), SRC2_IMM_2D(0),
                                59, 9, true);
    VPRO::DIM2::PROCESSING::add(L1,
                                DST_ADDR(8, 1, 60), SRC1_ADDR(8, 1, 60), SRC2_CHAINING_LEFT_2D,
                                59, 9, true);

    VPRO::DIM2::LOADSTORE::store(buffer_offset + 2*block_size + coefficients,
                                 0, 1, 60,
                                 59, 9, L1);

    // second part

    // reset rf, only for accumulation region relevant
    VPRO::DIM2::PROCESSING::add(L0_1,
                                DST_ADDR(600, 1, 0), SRC1_IMM_2D(0), SRC2_IMM_2D(0),
                                7, 0);

    // FIR
    VPRO::DIM3::PROCESSING::mach_init_imm(L0,
                                          DST_ADDR(0, 0, 1, 60), SRC1_LS_3D, SRC2_ADDR(1024 - 8, 1, 0, 0),
                                          7, 59, 9,
                                          0);
    VPRO::DIM3::PROCESSING::mach_init_imm(L1,
                                          DST_ADDR(0, 0, 1, 60), SRC1_LS_3D, SRC2_ADDR(1024 - 8, 1, 0, 0),
                                          7, 59, 9,
                                          0);
    VPRO::DIM3::LOADSTORE::loads(buffer_offset + block_size,
                                 0, 1, 1, 60,
                                 7, 59, 9);

//    // 8 to be added to +8 region
    VPRO::DIM2::LOADSTORE::loads(buffer_offset + 2*block_size + coefficients + block_size - coefficients/2,
                                 0, 1, 0,
                                 7, 0);
    VPRO::DIM2::PROCESSING::add(L1,
                                DST_ADDR(0, 1, 0), SRC1_ADDR(0, 1, 0), SRC2_LS_2D,
                                7, 0);

    VPRO::DIM2::LOADSTORE::store(buffer_offset + 2*block_size + coefficients + block_size - coefficients/2,
                                 0, 1, 0,
                                 7, 0, L1);
    VPRO::DIM2::PROCESSING::add(L1,
                                DST_ADDR(0, 1, 0), SRC1_ADDR(0, 1, 0), SRC2_IMM_2D(0),
                                7, 0, true);

    // add both partial results (+8 index offset in L1)
    VPRO::DIM2::PROCESSING::add(L0,
                                DST_ADDR(0, 1, 60), SRC1_ADDR(0, 1, 60), SRC2_IMM_2D(0),
                                59, 9, true);
    VPRO::DIM2::PROCESSING::add(L1,
                                DST_ADDR(8, 1, 60), SRC1_ADDR(8, 1, 60), SRC2_CHAINING_LEFT_2D,
                                59, 9, true);
    VPRO::DIM2::LOADSTORE::store(buffer_offset + 2*block_size + coefficients + block_size,
                                 0, 1, 60,
                                 59, 9, L1);
}
