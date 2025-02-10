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
// Created by gesper on 30.03.22.
//

#ifndef ISA_INTRINSIC_LIB_ISS_AUX_H
#define ISA_INTRINSIC_LIB_ISS_AUX_H

#include <vpro/vpro_globals.h>
#include "core_wrapper.h"
#include "simulator/ISS.h"
#include "vpro/vpro_special_register_enums.h"

void vpro_sync();

// ***************************************************************************************************************
// DMA elementary function prototypes
// ***************************************************************************************************************
void dma_set_pad_widths(uint32_t top, uint32_t right, uint32_t bottom, uint32_t left);

void dma_set_pad_value(int16_t value);

void dma_wait_to_finish(uint32_t cluster_mask = 0xffffffff);
void vpro_dma_sync();

void dma_dcache_short_command(const void* ptr);

void dma_block_size(const uint32_t& size);

void dma_block_addr_trigger(const void* ptr);

void rv_dcache_flush();
void rv_dcache_clear();
void rv_icache_flush();
void rv_icache_clear();

void dma_e2l_1d(uint32_t cluster_mask,
    uint32_t unit_mask,
    intptr_t ext_base,
    uint32_t loc_base,
    uint32_t x_size);

void dma_e2l_2d(uint32_t cluster_mask,
    uint32_t unit_mask,
    intptr_t ext_base,
    uint32_t loc_base,
    uint32_t x_size,
    uint32_t y_size,
    uint32_t x_stride,
    const bool pad_flags[4] = default_flags);

void dma_l2e_1d(uint32_t cluster_mask,
    uint32_t unit_mask,
    intptr_t ext_base,
    uint32_t loc_base,
    uint32_t x_size);

void dma_l2e_2d(uint32_t cluster_mask,
    uint32_t unit_mask,
    intptr_t ext_base,
    uint32_t loc_base,
    uint32_t x_size,
    uint32_t y_size,
    uint32_t x_stride);

void dma_print_access_counters();
void dma_reset_access_counters();

// ***************************************************************************************************************
// DCMA elementary function prototypes
// ***************************************************************************************************************
void dcma_flush();

void dcma_reset();

// ***************************************************************************************************************
// VPRO Status/Control Register
// ***************************************************************************************************************
void vpro_wait_busy(uint32_t cluster_mask = 0xffffffff, uint32_t unit_mask = 0xffffffff);
void vpro_lane_sync();

void vpro_set_cluster_mask(uint32_t idmask = 0xffffffff);
void vpro_set_unit_mask(uint32_t idmask = 0xffffffff);
void vpro_set_idmask(uint32_t idmask = 0xffffffff);

uint32_t vpro_get_cluster_mask();
uint32_t vpro_get_unit_mask();

void vpro_mac_h_bit_shift(uint32_t shift = 24);
uint32_t vpro_mac_h_bit_shift();

void vpro_mul_h_bit_shift(uint32_t shift = 24);
uint32_t vpro_mul_h_bit_shift();

void vpro_set_mac_init_source(VPRO::MAC_INIT_SOURCE source = VPRO::MAC_INIT_SOURCE::ZERO);
VPRO::MAC_INIT_SOURCE vpro_get_mac_init_source();

void vpro_set_mac_reset_mode(VPRO::MAC_RESET_MODE mode = VPRO::MAC_RESET_MODE::Y_INCREMENT);
VPRO::MAC_RESET_MODE vpro_get_mac_reset_mode();

/**
 * 4 IO Write / 3D addressing mode
*/
void __vpro(uint32_t id,
    uint32_t blocking,
    uint32_t is_chain,
    uint32_t func,
    uint32_t flag_update,
    uint32_t dst,
    uint32_t src1,
    uint32_t src2,
    uint32_t x_end,
    uint32_t y_end,
    uint32_t z_end);

/**
 * 3 IO Write / 2D addressing mode
*/
void __vpro(uint32_t id,
    uint32_t blocking,
    uint32_t is_chain,
    uint32_t func,
    uint32_t flag_update,
    uint32_t dst,
    uint32_t src1,
    uint32_t src2,
    uint32_t x_end,
    uint32_t y_end);

// ***************************************************************************************************************
// Simulator dummies
// ***************************************************************************************************************
void sim_finish_and_wait_step(int clusters);

void sim_wait_step(bool finish = false, const char* msg = "");

/**
 * Starts a simulation thread (Gui as argc/v defines)
 * @param main_fkt pointer to the main function (TODO: automatic fetch)
 * @param argc
 * @param argv
 * @return
 */
int sim_init(int (*main_fkt)(int, char**), int& argc, char* argv[]);

void sim_start();

/**
 * finishes all remaining cycles (VPRO and DMA commands) and exits simulation loop
 * Gui still open
 * @param silent whether to print stats after finished simulation [default: false/printing]
 */
void sim_stop(bool silent = false);

bool sim_min_req(int cluster_cnt, int unit_cnt, int lane_cnt);

void sim_dump_local_memory(uint32_t cluster, uint32_t unit);

void sim_dump_register_file(uint32_t cluster, uint32_t unit, uint32_t lane);

void sim_dump_queue(uint32_t cluster, uint32_t unit);

void sim_stat_reset();

void sim_printf(const char* format);

template <typename... Args>
void sim_printf(const char* format, Args... args) {
    printf("#SIM_PRINTF: ");
    printf(format, args...);
    printf("\n");
};

uint32_t get_gpr_vpro_freq();
uint32_t get_gpr_risc_freq();
uint32_t get_gpr_dma_freq();
uint32_t get_gpr_axi_freq();
uint32_t get_gpr_git_sha();
uint32_t get_gpr_hw_build_date();
uint32_t get_gpr_clusters();
uint32_t get_gpr_units();
uint32_t get_gpr_laness();
uint32_t get_gpr_DCMA_off();
uint32_t get_gpr_DCMA_nr_rams();
uint32_t get_gpr_DCMA_line_size();
uint32_t get_gpr_DCMA_associativity();

#endif  // ISA_INTRINSIC_LIB_ISS_AUX_H
