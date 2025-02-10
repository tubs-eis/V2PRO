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
/**
 * @file core_wrapper.h
 * @author Sven Gesper, IMS, Uni Hannover, 2019
 *
 * VPRO instruction & system simulation library
 */

#ifndef core_class_wrapper
#define core_class_wrapper

#include <stdlib.h>
#include "vpro/vpro_globals.h"

#if (SIMULATION == 1)

class ISS;
#include "iss_aux.h"

extern ISS* core_;

// TODO: eisv-functions (IO)

void io_write(uint32_t addr, uint32_t value);
uint32_t io_read(uint32_t addr);

// *** Main Memory *** //
int bin_file_send(uint32_t addr, int num_bytes, char const* file_name);
int bin_file_dump(uint32_t addr, int num_bytes, char const* file_name);

uint8_t* bin_file_return(uint32_t addr, int num_bytes);

uint16_t* get_main_memory(uint32_t addr, int num_elements);
uint16_t* get_mm_element(uint32_t addr);

uint8_t* bin_rf_return(uint32_t cluster, uint32_t unit, uint32_t lane);
uint8_t* bin_lm_return(uint32_t cluster, uint32_t unit);

void aux_memset(uint32_t base_addr, uint8_t value, uint32_t num_bytes);

// *** System *** //
void aux_print_debugfifo(uint32_t data);
void aux_dev_null(uint32_t data);

void aux_flush_dcache();
void aux_clr_dcache();

void aux_wait_cycles(int cycles);

void aux_clr_icache();
void aux_clr_sys_time();
void aux_clr_cycle_cnt();

uint32_t aux_get_cycle_cnt();
uint32_t aux_get_sys_time_lo();
uint32_t aux_get_sys_time_hi();

void aux_send_system_runtime();

// any lane is active / vpro somewhere act
void aux_clr_CNT_VPRO_LANE_ACT();
uint32_t aux_get_CNT_VPRO_LANE_ACT();

// any dma is active
void aux_clr_CNT_VPRO_DMA_ACT();
uint32_t aux_get_CNT_VPRO_DMA_ACT();

// any lane and any dam active
void aux_clr_CNT_VPRO_BOTH_ACT();
uint32_t aux_get_CNT_VPRO_BOTH_ACT();

// cycles in vpro clk
void aux_clr_CNT_VPRO_TOTAL();
uint32_t aux_get_CNT_VPRO_TOTAL();

// cycles in risc clk
void aux_clr_CNT_RISC_TOTAL();
uint32_t aux_get_CNT_RISC_TOTAL();

// eisv not synchronizing
void aux_clr_CNT_RISC_ENABLED();
uint32_t aux_get_CNT_RISC_ENABLED();

void aux_reset_all_stats();
void aux_print_statistics(int64_t total_clock_aux = -1);

#endif  //SIMULATION

#endif  //core_class_wrapper
