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
 * @file core_wrapper.cpp
 * @author Sven Gesper, IMS, Uni Hannover, 2019
 * @date 24.07.19
 *
 * VPRO instruction & system simulation library
 */

#include "core_wrapper.h"
#include <string>
#include <vector>
#include "iss_aux.h"
#include "simulator/ISS.h"

ISS* core_ = new ISS();

void io_write(uint32_t addr, uint32_t value) {
    core_->io_write(addr, value);
}

uint32_t io_read(uint32_t addr) {
    return core_->io_read(addr);
}

// *** Main Memory *** //
int bin_file_send(uint32_t addr, int num_bytes, char const* file_name) {
    return core_->bin_file_send(addr, num_bytes, file_name);
}

int bin_file_dump(uint32_t addr, int num_bytes, char const* file_name) {
    return core_->bin_file_dump(addr, num_bytes, file_name);
}

uint8_t* bin_file_return(uint32_t addr, int num_bytes) {
    return core_->bin_file_return(addr, num_bytes);
}

uint16_t* get_main_memory(uint32_t addr, int num_elements_16bit) {
    return core_->get_main_memory(addr, num_elements_16bit);
}

uint16_t* get_mm_element(uint32_t addr) {
    return core_->get_main_memory(addr, 1);
}

uint8_t* bin_rf_return(uint32_t cluster, uint32_t unit, uint32_t lane) {
    return core_->bin_rf_return(cluster, unit, lane);
}

uint8_t* bin_lm_return(uint32_t cluster, uint32_t unit) {
    return core_->bin_lm_return(cluster, unit);
}

void aux_memset(uint32_t base_addr, uint8_t value, uint32_t num_bytes) {
    core_->aux_memset(base_addr, value, num_bytes);
}

// *** System *** //
void aux_print_debugfifo(uint32_t data) {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    if (debug & DEBUG_INSTRUCTIONS) {
        printf("Sim print debug fifo \n");
    }
    printf("#SIM DEBUG_FIFO: 0x%08x\n", data);
}

void aux_dev_null(uint32_t data) {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    if (debug & DEBUG_INSTRUCTIONS) {
        printf("Sim print dev null \n");
    }
    if (debug & DEBUG_DEV_NULL) {
        printf("#SIM DEV_NULL: 0x%08x\n", data);
    }
}

void aux_flush_dcache() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    if (debug & DEBUG_INSTRUCTIONS) {
        printf("Sim flush dcache \n");
    }
    // just a dummy
}

void aux_wait_cycles(int cycles) {
#ifdef ISS_STANDALONE
    while (cycles > 0) {
        core_->runUntilRiscReadyForCmd();
        cycles--;
    }
#endif
}

void aux_clr_dcache() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
}

void aux_clr_icache() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
}

void aux_clr_sys_time() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    core_->aux_sys_time = 0;
    // just a dummy
    if (debug & DEBUG_INSTRUCTIONS) {
        printf("Sim aux clr sys time \n");
    }
}

void aux_clr_cycle_cnt() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    core_->aux_cycle_counter = 0;
    // just a dummy
    if (debug & DEBUG_INSTRUCTIONS) {
        printf("Sim aux clr cycle caunt \n");
    }
}

uint32_t aux_get_cycle_cnt() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    if (debug & DEBUG_INSTRUCTIONS) {
        printf("Sim get cycle cnt %i \n", uint32_t(core_->aux_cycle_counter));
    }
    return core_->aux_cycle_counter;
}

void aux_send_system_runtime() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    printf("\n#SIM aux_sys_time send: 0x%08x\n", uint32_t(core_->aux_sys_time));
    // just a dummy
    if (debug & DEBUG_INSTRUCTIONS) {
        printf("Sim send system runtime \n");
    }
}

uint32_t aux_get_sys_time_lo() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    return (uint32_t)(core_->aux_sys_time);
}

uint32_t aux_get_sys_time_hi() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    return (core_->aux_sys_time) >> 32;
}

// any lane is active / vpro somewhere act
void aux_clr_CNT_VPRO_LANE_ACT() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    core_->aux_cnt_lane_act = 0;
}

uint32_t aux_get_CNT_VPRO_LANE_ACT() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    return core_->aux_cnt_lane_act;
}

// any dma is active
void aux_clr_CNT_VPRO_DMA_ACT() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    core_->aux_cnt_dma_act = 0;
}

uint32_t aux_get_CNT_VPRO_DMA_ACT() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    return core_->aux_cnt_dma_act;
}

// any lane and any dam active
void aux_clr_CNT_VPRO_BOTH_ACT() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    core_->aux_cnt_both_act = 0;
}

uint32_t aux_get_CNT_VPRO_BOTH_ACT() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    return core_->aux_cnt_both_act;
}

// cycles in vpro clk
void aux_clr_CNT_VPRO_TOTAL() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    core_->aux_cnt_vpro_total = 0;
}

uint32_t aux_get_CNT_VPRO_TOTAL() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    return core_->aux_cnt_vpro_total;
}

// cycles in risc clk
void aux_clr_CNT_RISC_TOTAL() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    core_->aux_cnt_riscv_total = 0;
}

uint32_t aux_get_CNT_RISC_TOTAL() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    return core_->aux_cnt_riscv_total;
}

// eisv not synchronizing
void aux_clr_CNT_RISC_ENABLED() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    core_->aux_cnt_riscv_enabled = 0;
}

uint32_t aux_get_CNT_RISC_ENABLED() {
#ifdef ISS_STANDALONE
    core_->runUntilRiscReadyForCmd();
#endif
    return core_->aux_cnt_riscv_enabled;
}

void aux_reset_all_stats() {
    //    aux_clr_sys_time();
    aux_clr_CNT_VPRO_LANE_ACT();
    aux_clr_CNT_VPRO_DMA_ACT();
    aux_clr_CNT_VPRO_BOTH_ACT();
    aux_clr_CNT_VPRO_TOTAL();
    aux_clr_CNT_RISC_TOTAL();
    aux_clr_CNT_RISC_ENABLED();

    dma_reset_access_counters();
}

void aux_print_statistics(int64_t total_clock_aux) {
    uint64_t lane = aux_get_CNT_VPRO_LANE_ACT();
    uint64_t dma = aux_get_CNT_VPRO_DMA_ACT();
    uint64_t both = aux_get_CNT_VPRO_BOTH_ACT();
    uint64_t dma_o = dma - both;
    uint64_t lane_o = lane - both;
    uint64_t tot;
    uint64_t tot2;
    if (total_clock_aux > 0) {
        tot = total_clock_aux;
        tot2 = total_clock_aux;
    } else {
        tot = aux_get_CNT_VPRO_TOTAL();
        tot2 = aux_get_CNT_RISC_TOTAL();
    }

    printf("\nAll Cycles in EIS-V Domain:\n");
    printf("  [VPRO AUX]  (any) Lane active:               %lu Cycles \n", lane);
    printf("  [VPRO AUX]  (any) DMA active:                %lu Cycles \n", dma);
    printf("  [VPRO AUX]  (any) Lane AND (any) DMA active: %lu Cycles \n", both);
    printf("  [VPRO AUX]  Total:                           %lu Cycles \n", tot);
    printf("  [EIS-V AUX] Total:                           %lu Cycles \n", tot2);
    printf("\n");
    printf("  [VPRO AUX]  (any) Lane Busy: %lu.%lu%% \n",
        (100 * lane) / tot,
        (10000 * lane) / tot - ((100 * lane) / tot) * 100);
    printf("  [VPRO AUX]  (any) DMA Busy:  %lu.%lu%% \n",
        (100 * dma) / tot,
        (10000 * dma) / tot - ((100 * dma) / tot) * 100);
    printf("\n");
    printf("  [VPRO AUX]  (only) Lane:     %lu.%lu%% \n",
        (100 * lane_o) / tot,
        (10000 * lane_o) / tot - ((100 * lane_o) / tot) * 100);
    printf("  [VPRO AUX]  (both) DMA+Lane: %lu.%lu%% \n",
        (100 * both) / tot,
        (10000 * both) / tot - ((100 * both) / tot) * 100);
    printf("  [VPRO AUX]  (only) DMA:      %lu.%lu%% \n",
        (100 * dma_o) / tot,
        (10000 * dma_o) / tot - ((100 * dma_o) / tot) * 100);
    printf("  [VPRO AUX]  (only) EIS-V:    %lu.%lu%% \n",
        (100 * (tot - lane_o - both - dma_o)) / tot,
        (10000 * (tot - lane_o - both - dma_o)) / tot -
            ((100 * (tot - lane_o - both - dma_o)) / tot) * 100);
    printf("\n");

    dma_print_access_counters();

    printf("Clock Domains\n");
    printf("  [EIS-V]: %s%04.2lf MHz%s (%02.2lf ns Period)\n",
        MAGENTA,
        1000 / core_->getRiscClockPeriod(),
        RESET_COLOR,
        core_->getRiscClockPeriod());
    printf("  [AXI]:   %s%04.2lf MHz%s (%02.2lf ns Period)\n",
        MAGENTA,
        1000 / core_->getAxiClockPeriod(),
        RESET_COLOR,
        core_->getAxiClockPeriod());
    printf("  [DCMA]:  %s%04.2lf MHz%s (%02.2lf ns Period)\n",
        MAGENTA,
        1000 / core_->getDCMAClockPeriod(),
        RESET_COLOR,
        core_->getDCMAClockPeriod());
    printf("  [DMA]:   %s%04.2lf MHz%s (%02.2lf ns Period)\n",
        MAGENTA,
        1000 / core_->getDMAClockPeriod(),
        RESET_COLOR,
        core_->getDMAClockPeriod());
    printf("  [VPRO]:  %s%04.2lf MHz%s (%02.2lf ns Period)\n",
        MAGENTA,
        1000 / core_->getVPROClockPeriod(),
        RESET_COLOR,
        core_->getVPROClockPeriod());
    printf("\n");
}
