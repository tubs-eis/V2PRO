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
// ########################################################
// # example app for MIPS, using some IO/Data instruction #
// #                                                      #
// # Sven Gesper, EIS, Tu Braunschweig, 2021              #
// ########################################################

#include <stdint.h>
#include <algorithm>
#include <vpro.h>
#include "riscv/eisV_hardware_info.hpp"

/**
 * Test Data Variables
 */
constexpr int NUM_TEST_ENTRIES = 64;
volatile int16_t __attribute__ ((section (".vpro"))) test_array_1[NUM_TEST_ENTRIES];
volatile int16_t __attribute__ ((section (".vpro"))) test_array_2[NUM_TEST_ENTRIES];
volatile int16_t __attribute__ ((section (".vpro"))) result_array[NUM_TEST_ENTRIES];

/**
 * Main
 */
int main(int argc, char *argv[]) {
    sim_init(main, argc, argv);
    //aux_print_hardware_info("Template App");

    // broadcast to all
    vpro_set_cluster_mask(0xFFFFFFFF);
    vpro_set_unit_mask(0xFFFFFFFF);
    vpro_set_mac_init_source(VPRO::MAC_INIT_SOURCE::IMM);
    vpro_set_mac_reset_mode(VPRO::MAC_RESET_MODE::X_INCREMENT);
    vpro_mac_h_bit_shift(0);
    vpro_mul_h_bit_shift(0);

    for (int16_t i = 0; i < NUM_TEST_ENTRIES; i++) {
        test_array_1[i] = i;
        test_array_2[i] = i;
    }

    // DMA & VPRO Instructions
    dma_ext1D_to_loc1D(0, intptr_t(test_array_1), LM_BASE_VU(0) + 0, NUM_TEST_ENTRIES);
    dma_ext1D_to_loc1D(0, intptr_t(test_array_2), LM_BASE_VU(0) + NUM_TEST_ENTRIES, NUM_TEST_ENTRIES);

    
    //dma_loc1D_to_ext2D(0, intptr_t(result_array), LM_BASE_VU(0) + NUM_TEST_ENTRIES * 2, 3, NUM_TEST_ENTRIES, 1);
    dma_wait_to_finish(0xffffffff);

    //printf("print pointers... %u %u\n", intptr_t(test_array_1), intptr_t(test_array_2));

    /*
//
//    VPRO::DIM2::LOADSTORE::loads(0,
//                                 0, 1, 8,
//                                 7, 7);
//
//    VPRO::DIM2::PROCESSING::add(L0,
//                                DST_ADDR(0, 1, 8), SRC1_LS_2D, SRC2_IMM_2D(0),
//                                7, 7);
//
//    VPRO::DIM2::LOADSTORE::loads(NUM_TEST_ENTRIES,
//                                 0, 1, 8,
//                                 7, 7);
//
//    VPRO::DIM2::PROCESSING::add(L0,
//                                DST_ADDR(0, 1, 8), SRC1_LS_2D, SRC2_ADDR(0, 1, 8),
//                                7, 7);
//
//    VPRO::DIM2::PROCESSING::add(L0,
//                                DST_ADDR(0, 1, 8), SRC1_ADDR(0, 1, 8), SRC2_IMM_2D(0),
//                                7, 7, true);
//
//    VPRO::DIM2::LOADSTORE::store(NUM_TEST_ENTRIES * 2,
//                                 0, 1, 8,
//                                 7, 7,
//                                 L0);
    */
    

    VPRO::DIM3::LOADSTORE::loads(0,
                                 0, 1, 8, 0,
                                 7, 7, 0);

    VPRO::DIM3::PROCESSING::add(L0,
                                DST_ADDR(0, 1, 8, 0), SRC1_LS_3D, SRC2_IMM_3D(0),
                                7, 7, 0);                            

    VPRO::DIM3::LOADSTORE::loads(NUM_TEST_ENTRIES,
                                 0, 1, 8, 0,
                                 7, 7, 0);

    VPRO::DIM3::PROCESSING::mach_init_imm(L0,
                                DST_ADDR(0, 1, 8, 0), SRC1_LS_3D, SRC2_ADDR(0, 1, 8, 0),
                                7, 7, 0,
                                0);

    VPRO::DIM3::PROCESSING::add(L0,
                                DST_ADDR(0, 1, 8, 0), SRC1_ADDR(0, 1, 8, 0), SRC2_IMM_3D(0),
                                7, 7, 0, true);

    VPRO::DIM3::LOADSTORE::store(NUM_TEST_ENTRIES * 2,
                                 0, 1, 8, 0,
                                 7, 7, 0,
                                 L0);

    vpro_wait_busy();
    vpro_wait_busy();
    vpro_wait_busy();


    dma_loc1D_to_ext1D(0, intptr_t(result_array), LM_BASE_VU(0) + NUM_TEST_ENTRIES * 2, NUM_TEST_ENTRIES);
    dma_loc1D_to_ext2D(0, intptr_t(result_array), LM_BASE_VU(0) + NUM_TEST_ENTRIES * 2, 1 - 1, NUM_TEST_ENTRIES, 1);
    dma_wait_to_finish(0xffffffff);

    dcma_flush();

    /**
     * Check result correctnes (ISS)
     */
    // C-Code reference (MIPS executes this)
    auto reference_result = new int16_t[NUM_TEST_ENTRIES];
    for (int i = 0; i < NUM_TEST_ENTRIES; i++) {
        reference_result[i] = test_array_1[i] * test_array_2[i];
    }
//    bool fail = false;
    for (int i = 0; i < NUM_TEST_ENTRIES; i++) {
        if (reference_result[i] != result_array[i]) {
            printf_error("Result is not same as reference! [Index: %i]\n", i);
            printf_error("Reference: %i, result: %i\n", reference_result[i], result_array[i]);
//            fail = true;
        } else {
            printf_success("Reference: %i  = result: %i\n", reference_result[i], result_array[i]);
        }
    }

//    printf_info("Data A: \n");
//    for (short i: test_array_1) {
//        printf_info("%6i, ", i);
//    }
//    printf_info("\nData B: \n");
//    for (short i: test_array_2) {
//        printf_info("%6i, ", i);
//    }
//    printf_info("\nResult Data: \n");
//    for (int i = 0; i < NUM_TEST_ENTRIES; i++) {
//        printf_info("%6i, ", result_array[i]);
//    }
//    printf_info("\nReference Data: \n");
//    for (int i = 0; i < NUM_TEST_ENTRIES; i++) {
//        printf_info("%6i, ", reference_result[i]);
//    }

    sim_stop();
    return 0;
}
