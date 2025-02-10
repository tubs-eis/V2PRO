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
// # Tutorial for the VPRO Instruction usage              #
// #                                                      #
// # Sven Gesper, EIS, Tu Braunschweig, 2022              #
// ########################################################

#include <stdint.h>
#include <stdio.h>      /* printf, scanf, puts, NULL */
//#include <cstdlib>     /* srand, rand */
//#include <ctime>       /* time */
//#include <limits>

#include <vpro.h>
#include <eisv.h>
#include "riscv/eisV_hardware_info.hpp"
#include "fir.h"

/**
 * Main
 */
int main(int argc, char *argv[]) {
    sim_init(main, argc, argv);
    sim_min_req(VPRO_CFG::CLUSTERS, VPRO_CFG::UNITS, 2);
    aux_print_hardware_info("FIR");
    printf("Clusters: %i, units: %i, lanes: %i \n", VPRO_CFG::CLUSTERS, VPRO_CFG::UNITS, VPRO_CFG::LANES);

    /**
     * Input Generation
     */
    // Generate input data and coefficients
//    srand(time(NULL));
    for (int i = 0; i < input_size; i++) {
        auto &d = input_data[i];
        d = i; //rand() % std::numeric_limits<int8_t>::max();
    }
    for (int i = 0; i < coefficients; i++) {
        auto &d = h[i];
        d = i; //rand() % std::numeric_limits<int8_t>::max();
    }
    /**
     * Reference
     */
    for (int i = 0; i < input_size; i++) {
        auto &r = reference_data[i];
        for (int c = 0; c < coefficients; c++) {
            if ((i + c) < input_size) {
                r += h[c] * input_data[i + c];
            }
        }
        r = int16_t(r >> back_shift_factor);
    }

    /**
     * Reset / Initialization
     */
    // broadcast to all
    vpro_set_cluster_mask(0xFFFFFFFF);
    vpro_set_unit_mask(0xFFFFFFFF);
    vpro_mac_h_bit_shift(back_shift_factor);  // TODO: implementation add in last step with shifted precision adds error
    vpro_set_mac_reset_mode(VPRO::Y_INCREMENT);
    vpro_set_mac_init_source(VPRO::IMM);

    // reset LM
    dma_e2l_1d(0xffff, 0xffff, intptr_t(zeros), 0, 1024);
    dma_e2l_1d(0xffff, 0xffff, intptr_t(zeros), 1024, 1024);
    dma_e2l_1d(0xffff, 0xffff, intptr_t(zeros), 2048, 1024);
    dma_e2l_1d(0xffff, 0xffff, intptr_t(zeros), 3072, 1024);
    dma_e2l_1d(0xffff, 0xffff, intptr_t(zeros), 4096, 1024);
    dma_e2l_1d(0xffff, 0xffff, intptr_t(zeros), 5120, 1024);
    dma_e2l_1d(0xffff, 0xffff, intptr_t(zeros), 6144, 1024);
    dma_e2l_1d(0xffff, 0xffff, intptr_t(zeros), 7168, 1024);

    // reset rf
    VPRO::DIM2::PROCESSING::add(L0_1,
                                DST_ADDR(0, 1, 32), SRC1_IMM_2D(0), SRC2_IMM_2D(0),
                                31, 31);


    // cache prepare
    vpro_fir();
    dma_e2l_1d(0x1, 0x1, intptr_t(input_data), 0, input_size);
    dma_loc1D_to_ext1D(0, intptr_t(output_data), LM_BASE_VU(0) + input_size + coefficients, input_size);


    // load coeff
    dma_e2l_1d(0xffff, 0xffff, intptr_t(h), 8192 - coefficients, coefficients);
    vpro_dma_sync();

    // split coefficients to both lanes
    VPRO::DIM2::LOADSTORE::loads(8192 - coefficients,
                                 0, 0, 1,
                                 0, coefficients - 1);
    VPRO::DIM2::PROCESSING::add(L0_1,
                                DST_ADDR(1024 - coefficients, 0, 1), SRC1_LS_2D, SRC2_IMM_2D(0),
                                0, coefficients - 1);
    // move up in L0
    VPRO::DIM2::PROCESSING::or_(L0,
                                DST_ADDR(1024 - coefficients/2, 0, 1), SRC1_ADDR(1024 - coefficients, 0, 1), SRC2_IMM_2D(0),
                                0, coefficients/2 - 1);

    // or: just to specific lane (sequential by blocking) -> hw bug of blocking
//    VPRO::DIM2::PROCESSING::add(L0,
//                                DST_ADDR(1024 - coefficients/2, 0, 1), SRC1_LS_2D, SRC2_IMM_2D(0),
//                                0, coefficients/2 - 1, false, false, true);  // blocking
//    VPRO::DIM2::PROCESSING::add(L1,
//                                DST_ADDR(1024 - coefficients/2, 0, 1), SRC1_LS_2D, SRC2_IMM_2D(0),
//                                0, coefficients/2 - 1);

    vpro_lane_sync();

    /**
     * FIR Start
     */
    // load input data in 8*repetive blocks
    // split in two operations, to not overfill RF (1200/2 per operation)
    // shift back by back_shift_factor

    bool doublebuffering = true;

    sim_stat_reset();
    aux_reset_all_stats();

    if (!doublebuffering) {
        // load this block data
//        dma_e2l_1d(0x1, 0x1, intptr_t(input_data), 0, input_size);
        for (int c = 0; c < VPRO_CFG::CLUSTERS; ++c) {
            for (int u = 0; u < VPRO_CFG::UNITS; ++u) {
                dma_ext1D_to_loc1D(c, intptr_t(input_data) + input_size*2*c*u, LM_BASE_VU(u) + 0, input_size);
            }
        }
        vpro_dma_sync();

        vpro_fir();
        vpro_lane_sync();

        // DMA store
//        dma_loc1D_to_ext1D(0, intptr_t(output_data), LM_BASE_VU(0) + input_size + coefficients, input_size);
        for (int c = 0; c < VPRO_CFG::CLUSTERS; ++c) {
            for (int u = 0; u < VPRO_CFG::UNITS; ++u) {
                dma_loc1D_to_ext1D(c, intptr_t(output_data), LM_BASE_VU(u) + input_size + coefficients, input_size);
            }
        }
        vpro_dma_sync();
    } else {
        int load_buffer = 0;
        int calc_buffer = 0;
        dma_e2l_1d(0xffff, 0xffff, intptr_t(input_data), load_buffer, input_size);
        vpro_dma_sync();

        calc_buffer = 0;
        load_buffer = 4096;

        dma_e2l_1d(0xffff, 0xffff, intptr_t(input_data), load_buffer, input_size);
        vpro_fir(calc_buffer);
        vpro_sync();

        sim_stat_reset();
        aux_reset_all_stats();

        for (int c = 0; c < VPRO_CFG::CLUSTERS; ++c) {
            for (int u = 0; u < VPRO_CFG::UNITS; ++u) {
                dma_loc1D_to_ext1D(c, intptr_t(output_data), LM_BASE_VU(u) + calc_buffer + input_size + coefficients,
                                   input_size);
            }
        }

        calc_buffer = 4096;
        load_buffer = 0;

//        dma_e2l_1d(0xffff, 0xffff, intptr_t(input_data), load_buffer, input_size);
        for (int c = 0; c < VPRO_CFG::CLUSTERS; ++c) {
            for (int u = 0; u < VPRO_CFG::UNITS; ++u) {
                dma_ext1D_to_loc1D(c, intptr_t(input_data) + input_size*2*c*u, LM_BASE_VU(u) + 0, input_size);
            }
        }

        vpro_fir(calc_buffer);
        vpro_sync();
//
//        calc_buffer = 0;
//        load_buffer = 4096;
    }

    // finish
//    dcma_flush();


    auto cnt = (((uint64_t(aux_get_sys_time_hi())) << 32) + uint64_t(aux_get_sys_time_lo()));
    aux_print_statistics();
    printf("[FIR] Sys-Time (Risc-V Cycles): %llu (%llu ms = %llu us)\n",
           (unsigned long long)cnt,
           (unsigned long long)(1000 * cnt / get_gpr_risc_freq()),
           (unsigned long long)(1000 * 1000 * cnt / get_gpr_risc_freq()));

    /**
     * Verification
     */
    int fail = 0;
    for (int i = 0; i < input_size; ++i) {
        if (abs(output_data[i] - reference_data[i]) >
            1) {  // TODO: error of 1 allowed, as shift occurs in VPRO before final add
//            if (i >= 1180)
            printf_error("[Verification] failed on [%d]. Reference: %d, Result: %d\n", i, reference_data[i],
                         output_data[i]);
            fail++;
        } else {
//            printf_success("[Verification] success on [%d]. Reference: %d, Result: %d\n", i, reference_data[i],
//                         output_data[i]);
        }
    }
    if (fail > 0) {
        printf_error("[Verification] failed: %d!\n", fail);
    } else {
        printf_success("[Verification] success!\n");
    }


    sim_stop();
    return 0;
}
