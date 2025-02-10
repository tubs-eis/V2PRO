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
// #########################################################
// # Test for Load Store Lane using dynamic chained offset #
// #                                                       #
// # Sven Gesper, EIS, Tu Braunschweig, 2023               #
// #########################################################

#include <stdint.h>
#include <algorithm>
#include <vpro.h>

volatile uint16_t __attribute__ ((section (".vpro"))) test_offsets[8];

volatile int16_t __attribute__ ((section (".vpro"))) test_data[256];
volatile int16_t __attribute__ ((section (".vpro"))) result_array[8];

/**
 * Main
 */
int main(int argc, char *argv[]) {
    sim_init(main, argc, argv);

    // copy/create data, as .vpro section has no initial data
    uint16_t tmp[8] = { 0x0001, 0x0023, 0x00A2, 0x00B7, 0x00CC, 0x009D, 0x0049, 0x0090 };

    // Init test data
    for (uint16_t i = 0; i < 256; i++) {
        test_data[i] = 255 - i;
        if (i < 8)
            test_offsets[i] = tmp[i];
    }

    // Upload test data, offsets
    dma_e2l_1d(1, 1, uintptr_t(test_data), 0, 256);
    dma_e2l_1d(1, 1, uintptr_t(test_offsets), 1024, 8);
    dma_wait_to_finish(0xffffffff);
    vpro_sync();

    // Copy offsets to regs
    VPRO::DIM2::LOADSTORE::load(
        1024, 0, 1, 0,
        7, 0
    );
    VPRO::DIM2::PROCESSING::add(
        L1,
        DST_ADDR(0, 1, 0),
        SRC1_LS_2D,
        SRC2_IMM_2D(0),
        7, 0
    );

    vpro_wait_busy(0xffffffff, 0xffffffff);

    VPRO::DIM2::PROCESSING::add(
        L1,
        DST_ADDR(0, 1, 0),
        SRC1_ADDR(0, 1, 0),
        SRC2_IMM_2D(0),
        7, 0,
        true
    );

    VPRO::DIM3::LOADSTORE::dynamic_load(
        L1,
        0, 0, 0, 0,
        7, 0, 0
    );

    VPRO::DIM2::PROCESSING::add(
        L0,
        DST_ADDR(0, 1, 0),
        SRC_LS_2D,
        SRC2_IMM_2D(0),
        7, 0
    );
    // 254 , 220, 93, ...


    // store back to LM
    VPRO::DIM3::PROCESSING::add(L0,
                                DST_ADDR(0, 1, 8, 0), SRC1_ADDR(0, 1, 8, 0), SRC2_IMM_3D(0),
                                7, 0, 0, true);

    VPRO::DIM3::LOADSTORE::store(2048,
                                 0, 1, 8, 0,
                                 7, 0, 0,
                                 L0);

    vpro_sync();

    // Transfer back to result array
    dma_loc1D_to_ext1D(0, intptr_t(result_array), LM_BASE_VU(0) + 2048, 8);
    dma_wait_to_finish(0xffffffff);
    vpro_sync();

    dcma_flush();

    /**
     * Check result correctnes (ISS)
     */
    // C-Code reference (MIPS executes this)
    auto reference_result = new int16_t[8];
    for (int i = 0; i < 8; i++) {
        reference_result[i] = 255-test_offsets[i];
    }
//    bool fail = false;
    for (int i = 0; i < 8; i++) {
        if (reference_result[i] != result_array[i]) {
            printf_error("Result is not same as reference! [Index: %i]\n", i);
            printf_error("Reference: %i, result: %i\n", reference_result[i], result_array[i]);
//            fail = true;
        } else {
            printf_success("Reference: %i  = result: %i\n", reference_result[i], result_array[i]);
        }
    }

    sim_stop();
    return 0;
}
