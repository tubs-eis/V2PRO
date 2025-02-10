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
// # tests for indirect addressing                        #
// #                                                      #
// # Daniel Köhler, Bosch, 2023                           #
// ########################################################

#include <stdint.h>
#include <algorithm>
#include <vpro.h>
#include <string>
//#include "iss_aux.h"

#define N_OFFSETS 8
uint16_t __attribute__ ((section (".vpro"))) offsets[N_OFFSETS];//  = {0, 2, 5, 6, 10, 17, 14, 10};
int16_t __attribute__ ((section (".vpro"))) bins[N_OFFSETS];//     = {0, 2, 2, 4, 5, 2, 4, 3};
int16_t __attribute__ ((section (".vpro"))) bin_data[N_OFFSETS];// = {4, 3, 6, 2, 5, 5, 1, 0};
int16_t __attribute__ ((section (".vpro"))) result_data[1024], test_data[1024];

void init_rf(uint32_t size) {
    VPRO::DIM3::PROCESSING::add(
        L0_1,
        DST_ADDR(0, 0, 0, 1),
        SRC1_IMM_3D(0),
        SRC2_IMM_3D(0),
        0, 0, size-1
    );
}

void load_data(intptr_t ext_addr, uint32_t lm_addr, uint32_t rf_addr, uint32_t size, LANE lane=L0) {

    dma_e2l_1d(0x1, 0x1, ext_addr, lm_addr, size);
    dma_wait_to_finish(0xffffffff);

    VPRO::DIM3::LOADSTORE::load(
        lm_addr, 0, 0, 0, 1,
        0, 0, size-1
    );
    VPRO::DIM3::PROCESSING::add(
        lane,
        DST_ADDR(rf_addr, 0, 0, 1),
        SRC1_LS_3D,
        SRC2_IMM_3D(0),
        0, 0, size-1
    );
    vpro_wait_busy(0xffffffff, 0xffffffff);
}

void chain_offsets(uint32_t rf_addr, uint32_t x_size, unsigned int stall_cycles=0, unsigned int repeats=1) {
    if (stall_cycles > 0) {
        // add some nops for L0 to stall
        VPRO::DIM3::PROCESSING::add(
            L0,
            DST_ADDR(0, 0, 0, 0),
            SRC1_ADDR(0, 0, 0, 0),
            SRC2_IMM_3D(0),
            0, 0, stall_cycles-1);
    }
    VPRO::DIM3::PROCESSING::add(
        L0,
        DST_ADDR(rf_addr, 0, 0, 1),
        SRC1_ADDR(rf_addr, 0, 0, 1),
        SRC2_IMM_3D(0),
        repeats-1, 0, x_size-1,
        true
    );
}

void store_results(intptr_t ext_addr, uint32_t lm_addr, uint32_t rf_addr, uint32_t size) {
    VPRO::DIM3::PROCESSING::add(
        L1,
        DST_ADDR(rf_addr, 0, 0, 1),
        SRC1_ADDR(rf_addr, 0, 0, 1),
        SRC2_IMM_3D(0),
        0, 0, size-1,
        true);
    VPRO::DIM3::LOADSTORE::store(lm_addr, rf_addr, 0, 0, 1, 0, 0, size-1, L1);
    vpro_wait_busy(0xffffffff, 0xffffffff);

    dma_l2e_1d(01, 01, ext_addr, 0, size);
    dma_wait_to_finish(0xffffffff);
    dcma_flush();
}

bool compare_results(int16_t result[], int16_t reference[], uint32_t size) {
    bool equal = true;
    for (uint32_t i = 0; i < size; i++) {
        if (result[i] != reference[i]) {
            equal = false;
            printf("Mismatch at %d: %d, expected %d!\n", i, result[i], reference[i]);
        }
        equal &= (result[i] == reference[i]);
    }
    return equal;
}

bool test_scatter(unsigned int stall_cycles=0) {
    // check whether chaining of offsets for dst address works properly,
    // i.e. scatter data to the specified offsets
    uint32_t value = 0xdead;

    init_rf(VPRO_CFG::RF_SIZE);
    load_data(intptr_t(offsets), 0x0, 0x0, N_OFFSETS);
    chain_offsets(0x0, N_OFFSETS, stall_cycles);

    // chain offsets to destination
    VPRO::DIM3::PROCESSING::add(
        L1,
        DST_INDIRECT_LEFT(),
        SRC1_IMM_3D(0),
        SRC2_IMM_3D(value),
        N_OFFSETS-1, 0, 0);

    store_results(intptr_t(result_data), 0x0, 0x0, VPRO_CFG::RF_SIZE);
    int16_t reference[1024] = {0};

    for (int i=0; i < N_OFFSETS; i++) {
        reference[offsets[i]] = value;
    }

    return compare_results(result_data, reference, VPRO_CFG::RF_SIZE);
}

bool test_gather(bool use_src1=true, unsigned int stall_cycles=0) {
    // check whether chaining of offsets for src1/src2 address works properly,
    // i.e. gather data at the specified offsets

    init_rf(VPRO_CFG::RF_SIZE);

    load_data(intptr_t(offsets), 0x0, 0x0, N_OFFSETS);

    // load test data
    for (unsigned int i=0; i < VPRO_CFG::RF_SIZE; i++)
        test_data[i] = i;
    load_data(intptr_t(test_data), 0x0, 0x0, VPRO_CFG::RF_SIZE, L1);

    chain_offsets(0x0, N_OFFSETS, stall_cycles);

    // chain offsets to src
    if (use_src1) {
        VPRO::DIM3::PROCESSING::add(
            L1,
            DST_ADDR(0, 0, 0, 1),
            SRC1_INDIRECT_LEFT(),
            SRC2_IMM_3D(0),
            0, 0, N_OFFSETS-1);
    } else {
        VPRO::DIM3::PROCESSING::add(
            L1,
            DST_ADDR(0, 0, 0, 1),
            SRC1_IMM_3D(0),
            SRC2_INDIRECT_RIGHT(),
            0, 0, N_OFFSETS-1);
    }

    store_results(intptr_t(result_data), 0x0, 0x0, N_OFFSETS);

    int16_t reference[VPRO_CFG::RF_SIZE] = {0};
    for (int i=0; i < N_OFFSETS; i++) {
        reference[i] = test_data[offsets[i]];
    }
    return compare_results(result_data, reference, N_OFFSETS);
}

bool test_scatter_with_data_chaining(unsigned int stall_cycles=0) {
    // check whether simultaneous chaining of offsets from L0 and data from LS works properly,
    // i.e. scatter data to the specified offsets

    init_rf(VPRO_CFG::RF_SIZE);
    load_data(intptr_t(offsets), 0x0, 0x0, N_OFFSETS);

    for (int i=0; i < N_OFFSETS; i++) {
        test_data[i] = i;
    }
    dma_e2l_1d(0x1, 0x1, intptr_t(test_data), 0x0, N_OFFSETS);
    dma_wait_to_finish(0xffffffff);
    chain_offsets(0x0, N_OFFSETS, stall_cycles);

    // chain data
    VPRO::DIM3::LOADSTORE::load(
        0, 0, 0, 0, 1,
        0, 0, N_OFFSETS-1
    );

    // chain offsets to destination and data to src 2
    VPRO::DIM3::PROCESSING::add(
        L1,
        DST_INDIRECT_LEFT(),
        SRC1_IMM_3D(0),
        SRC2_LS_3D,
        N_OFFSETS-1, 0, 0);

    store_results(intptr_t(result_data), 0x0, 0x0, VPRO_CFG::RF_SIZE);

    int16_t reference[1024] = {0};
    for (int i=0; i < N_OFFSETS; i++) {
        reference[offsets[i]] = test_data[i];
    }
    return compare_results(result_data, reference, VPRO_CFG::RF_SIZE);
}

bool test_scatter_from_ls() {
    // check whether chaining the offsets from L/S lane works properly,
    // i.e. scatter data to the specified offsets
    uint32_t value = 0xdead;

    init_rf(VPRO_CFG::RF_SIZE);

    // load offsets from L/S lane
    dma_e2l_1d(0x1, 0x1, intptr_t(offsets), 0x0, N_OFFSETS);
    dma_wait_to_finish(0xffffffff);
    VPRO::DIM3::LOADSTORE::load(
        0x0, 0, 0, 0, 1,
        0, 0, N_OFFSETS-1
    );

    // chain offsets to destination
    VPRO::DIM3::PROCESSING::add(
        L1,
        DST_INDIRECT_LS(),
        SRC1_IMM_3D(0),
        SRC2_IMM_3D(value),
        N_OFFSETS-1, 0, 0);

    store_results(intptr_t(result_data), 0x0, 0x0, VPRO_CFG::RF_SIZE);

    int16_t reference[1024] = {0};
    for (int i=0; i < N_OFFSETS; i++) {
        reference[offsets[i]] = value;
    }
    return compare_results(result_data, reference, VPRO_CFG::RF_SIZE);
}

bool test_bin_count() {
    // count occurences per bin

    unsigned int repeats = 4;
    init_rf(VPRO_CFG::RF_SIZE);
    load_data(intptr_t(bins), 0x0, 0x0, N_OFFSETS);
    chain_offsets(0x0, N_OFFSETS, 0, repeats);

    // chain offsets to destination
    VPRO::DIM3::PROCESSING::add(
        L1,
        DST_INDIRECT_LEFT(),
        SRC1_INDIRECT_LEFT(),
        SRC2_IMM_3D(1),
        repeats-1, 0, N_OFFSETS-1);

    store_results(intptr_t(result_data), 0x0, 0x0, VPRO_CFG::RF_SIZE);

    int16_t reference[1024] = {0};
    for (int i=0; i < N_OFFSETS; i++) {
        reference[bins[i]] += 1;
    }
    return compare_results(result_data, reference, VPRO_CFG::RF_SIZE);
}

bool test_bin_maxpool() {
    // perform max pooling per bin

    unsigned int repeats = 4;
    uint32_t data_rf = VPRO_CFG::RF_SIZE / 2;
    init_rf(VPRO_CFG::RF_SIZE);
    load_data(intptr_t(bins), 0x0, 0x0, N_OFFSETS);
    load_data(intptr_t(bin_data), 0x0, data_rf, N_OFFSETS, L1);
    chain_offsets(0x0, N_OFFSETS, 0, repeats);

    // chain offsets to destination
    VPRO::DIM3::PROCESSING::max(
        L1,
        DST_INDIRECT_LEFT(),
        SRC1_INDIRECT_LEFT(),
        SRC2_ADDR(data_rf, 0, 0, 1),
        repeats-1, 0, N_OFFSETS-1);

    store_results(intptr_t(result_data), 0x0, 0x0, VPRO_CFG::RF_SIZE);

    int16_t reference[1024] = {0};
    for (int i=0; i < N_OFFSETS; i++) {
        reference[bins[i]] = std::max(reference[bins[i]], bin_data[i]);
    }
    return compare_results(result_data, reference, VPRO_CFG::RF_SIZE/2);
}

void print_status(const std::string &name, bool result) {
    if (result) {
        printf_success("%s: successful!\n", name.c_str());
    } else {
        printf_error("%s: unsuccessful!\n", name.c_str());
    }
}


int main(int argc, char *argv[]) {

    sim_init(main, argc, argv);

    offsets[0] = 0;
    offsets[1] = 2;
    offsets[2] = 5;
    offsets[3] = 6;
    offsets[4] = 10;
    offsets[5] = 17;
    offsets[6] = 14;
    offsets[7] = 10;

    bins[0] = 0;
    bins[1] = 2;
    bins[2] = 2;
    bins[3] = 4;
    bins[4] = 5;
    bins[5] = 2;
    bins[6] = 4;
    bins[7] = 3;

    bin_data[0] = 4;
    bin_data[1] = 3;
    bin_data[2] = 6;
    bin_data[3] = 2;
    bin_data[4] = 5;
    bin_data[5] = 5;
    bin_data[6] = 1;
    bin_data[7] = 0;

    print_status("scatter", test_scatter());
    print_status("scatter with stall", test_scatter(100));
    print_status("scatter with data chaining", test_scatter_with_data_chaining());
    print_status("scatter with chaining from LS", test_scatter_from_ls());
    print_status("gather src1", test_gather());
    print_status("gather src1 with stall", test_gather(100));
    print_status("gather src2", test_gather(false));
    print_status("gather src2 with stall", test_gather(100));
    print_status("bincount", test_bin_count());
    print_status("maxpool", test_bin_maxpool());

    sim_stop();
    return 0;
}
