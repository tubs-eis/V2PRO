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
// ################################################################
// # eisv_aux.h - Auxiliary functions for the EIS-V processor     #
// #                                                              #
// # Stephan Nolting, Uni Hannover, 2014-2017                     #
// ################################################################

#ifndef eisv_aux_h
#define eisv_aux_h

// IO map
#include <stdio.h>
#include "eisv_defs.h"
#include "eisv_gp_registers.h"
#include "../vpro/vpro_defs.h"

#ifndef SIMULATION
#include "riscv-csr.hpp"
#include "../vpro/vpro_aux.h"

inline uint32_t aux_get_cycle_cnt();

inline void aux_clr_cycle_cnt();

inline void aux_clr_icache();

inline void aux_clr_dcache();

inline void aux_flush_dcache();

inline void aux_print_debugfifo(uint32_t data);

inline uint32_t aux_dev_null(uint32_t data);

inline void aux_wait_cycles(uint32_t n);

inline void aux_clr_sys_time();

inline uint32_t aux_get_sys_time_lo();

inline uint32_t aux_get_sys_time_hi();

inline void aux_send_system_runtime();

void aux_memset(uint32_t base_addr, uint8_t value, uint32_t num_bytes);

inline unsigned  int read_csr(int csr_num) {
    unsigned int result;
    asm("csrr %0, %1" : "=r"(result) : "I"(csr_num));
    return result;
}


inline void aux_clr_icache_miss_cnt(){
    CNT_ICACHE_MISS = 0;
}
inline void aux_clr_icache_hit_cnt(){
    CNT_ICACHE_HIT = 0;
}
inline void aux_clr_dcache_miss_cnt(){
    CNT_DCACHE_MISS = 0;
}
inline void aux_clr_dcache_hit_cnt(){
    CNT_DCACHE_HIT = 0;
}

inline uint32_t aux_icache_miss_cnt(){
    return CNT_ICACHE_MISS;
}
inline uint32_t aux_icache_hit_cnt(){
    return CNT_ICACHE_HIT;
}
inline uint32_t aux_dcache_miss_cnt(){
    return CNT_DCACHE_MISS;
}
inline uint32_t aux_dcache_hit_cnt(){
    return CNT_DCACHE_HIT;
}

// ***************************************************************************************************************
// CPU control macros
// ***************************************************************************************************************

// ---------------------------------------------------------------------------------
// get CPU-internal cycle counter
// ---------------------------------------------------------------------------------
inline uint32_t aux_get_cycle_cnt() {

    // CP0, R9, SEL0 = CPU cycle counter
//    printf("aux_get_cycle_cnt NOT in EISV\n");

    //asm volatile (".set noat\r\n" "mfc0 %0, $9, 0" : "=r" (res));

    // RISCV CSR Counters
//    0xC00 - (HPM) Cycle Counter
//    0xC02 - (HPM) Instructions-Retired Counter
//    0xC03 - (HPM) Performance-Monitoring Counter 3
//    0xC1F - (HPM) Performance-Monitoring Counter 31
//    0xC80 - (HPM) Upper 32 Cycle Counter
//    0xC82 - (HPM) Upper 32 Instructions-Retired Counter
//    0xC83 - (HPM) Upper 32 Performance-Monitoring Counter 3
//    0xC9F - (HPM) Upper 32 Performance-Monitoring Counter 31

    unsigned int h;
    asm("csrr %0, 0xc80" : "=r"(h));
    unsigned int l;
    asm("csrr %0, 0xc00" : "=r"(l));

    uint64_t cnt = (uint64_t(h) << 32) + l;

//            read_csr(0xC80);
//    cnt = (cnt << 32) + read_csr(0xC00);
    printf("\tMaybe @0xC00: %llu [l, %u| h %u]\n", cnt, l, h);
//
//    uint64_t cnt2 = read_csr(0xC82);
//    cnt2 = (cnt2 << 32) + read_csr(0xC02);
//    printf("\tMaybe @0xC02: %lu [retired instructions]\n", cnt2);
    return 0;
}

// ---------------------------------------------------------------------------------
// reset CPU-internal cycle counter
// ---------------------------------------------------------------------------------
inline void aux_clr_cycle_cnt() {
    printf("aux_clr_cycle_cnt NOT in EISV\n");
    //asm volatile (".set noat\r\n" "mtc0  $0, $9, 0");
}

// ---------------------------------------------------------------------------------
// Clear instruction cache
// ---------------------------------------------------------------------------------
inline void aux_clr_icache() {
    aux_wait_cycles(2);
    RV_ICACHE_CLEAR = 0;
    aux_wait_cycles(2);
}

// ---------------------------------------------------------------------------------
// Clear data cache
// ---------------------------------------------------------------------------------
inline void aux_clr_dcache() {
    aux_wait_cycles(2);
    RV_DCACHE_CLEAR = 0;
    aux_wait_cycles(2);
}

// ---------------------------------------------------------------------------------
// Flush data cache
// ---------------------------------------------------------------------------------
inline void aux_flush_dcache() {
    aux_wait_cycles(2);
    RV_DCACHE_FLUSH = 0;
    aux_wait_cycles(2);
}

// ***************************************************************************************************************
// Misc macros
// ***************************************************************************************************************

// ---------------------------------------------------------------------------------
// write data word to DEBUG FIFO
// ---------------------------------------------------------------------------------
inline void aux_print_debugfifo(uint32_t data) {

    DEBUG_FIFO = data;
}

// ---------------------------------------------------------------------------------
// Dummy write / read
// ---------------------------------------------------------------------------------
inline uint32_t aux_dev_null(uint32_t data) {

    DEV_NULL = data;
    return DEV_NULL;
}

// ---------------------------------------------------------------------------------
// Wait ~n CPU cycles
// ---------------------------------------------------------------------------------
inline void aux_wait_cycles(uint32_t n) {

    while (n--)
            asm volatile ("nop");
}

// ---------------------------------------------------------------------------------
// Clear system time counter
// ---------------------------------------------------------------------------------
inline void aux_clr_sys_time() {
    SYS_TIME_CNT_LO = 0;
}

// ---------------------------------------------------------------------------------
// Get system time counter (lower part)
// Read FIRST when reading a full 64-bit word!
// ---------------------------------------------------------------------------------
inline uint32_t aux_get_sys_time_lo() {
    return SYS_TIME_CNT_LO;
}

// ---------------------------------------------------------------------------------
// Get system time counter (higher part)
// Read LOW PART FIRST when reading a full 64-bit word!
// ---------------------------------------------------------------------------------
inline uint32_t aux_get_sys_time_hi() {
    return SYS_TIME_CNT_HI;
}

// any lane is active / vpro somewhere act
inline void aux_clr_CNT_VPRO_LANE_ACT() { CNT_VPRO_LANE_ACT = 0; }

inline uint32_t aux_get_CNT_VPRO_LANE_ACT() { return CNT_VPRO_LANE_ACT; }

// any dma is active
inline void aux_clr_CNT_VPRO_DMA_ACT() { CNT_VPRO_DMA_ACT = 0; }

inline uint32_t aux_get_CNT_VPRO_DMA_ACT() { return CNT_VPRO_DMA_ACT; }

// any lane and any dam active
inline void aux_clr_CNT_VPRO_BOTH_ACT() { CNT_VPRO_BOTH_ACT = 0; }

inline uint32_t aux_get_CNT_VPRO_BOTH_ACT() { return CNT_VPRO_BOTH_ACT; }

// cycles in vpro clk
inline void aux_clr_CNT_VPRO_TOTAL() { CNT_VPRO_TOTAL = 0; }

inline uint32_t aux_get_CNT_VPRO_TOTAL() { return CNT_VPRO_TOTAL; }

// cycles in risc clk
inline void aux_clr_CNT_RISC_TOTAL() { CNT_RISC_TOTAL = 0; }

inline uint32_t aux_get_CNT_RISC_TOTAL() { return CNT_RISC_TOTAL; }

// eisv not synchronizing
inline void aux_clr_CNT_RISC_ENABLED() { CNT_RISC_ENABLED = 0; }

inline uint32_t aux_get_CNT_RISC_ENABLED() { return -1; }

// ---------------------------------------------------------------------------------
// Send system runtime (64-bit) to debug FIFO
// This is the counterpart of the host function "get_system_runtime()"
// ---------------------------------------------------------------------------------
inline void aux_send_system_runtime() {
    aux_print_debugfifo(aux_get_sys_time_lo());
    aux_print_debugfifo(aux_get_sys_time_hi());
}

inline void aux_reset_all_stats(){
    aux_clr_sys_time();
    // aux_clr_cycle_cnt();

    aux_clr_CNT_VPRO_LANE_ACT();
    aux_clr_CNT_VPRO_DMA_ACT();
    aux_clr_CNT_VPRO_BOTH_ACT();
    aux_clr_CNT_VPRO_TOTAL();
    aux_clr_CNT_RISC_TOTAL();
    aux_clr_CNT_RISC_ENABLED();

    using namespace riscv::csr;
    // 0 = cycle
    // 1 = unused
    // 2 = instret
    mhpmcounter3_ops::write(0); // ?
    mhpmcounter4_ops::write(0); // ?
    mhpmcounter5::write(0);
    mhpmcounter6::write(0);
    mhpmcounter7::write(0);
    mhpmcounter8::write(0);
    mhpmcounter9::write(0);
    mhpmcounter10::write(0);
    mhpmcounter11::write(0);
    mhpmcounter12::write(0);
    mhpmcounter13::write(0);
    mhpmcounter14::write(0);
    mhpmcounter15::write(0);

    asm("CSRRW x0, mcycle, x0");
    asm("CSRRW x0, mcycleh, x0");
    asm("CSRRW x0, minstret, x0");
    asm("CSRRW x0, minstreth, x0");

    aux_clr_icache_miss_cnt();
    aux_clr_icache_hit_cnt();
    aux_clr_dcache_miss_cnt();
    aux_clr_dcache_hit_cnt();

    dma_reset_access_counters();
}

inline void aux_print_statistics(int64_t total_clock_aux = -1){
    using namespace riscv::csr;

    uint64_t lane = aux_get_CNT_VPRO_LANE_ACT();
    uint64_t dma = aux_get_CNT_VPRO_DMA_ACT();
    uint64_t both = aux_get_CNT_VPRO_BOTH_ACT();
    uint64_t dma_o = dma - both;
    uint64_t lane_o = lane - both;
    uint64_t tot;
    uint64_t tot2;
    if (total_clock_aux > 0){
        tot = total_clock_aux;
        tot2 = total_clock_aux;
    } else {
        tot = aux_get_CNT_VPRO_TOTAL();
        tot2 = aux_get_CNT_RISC_TOTAL();
    }

    uint64_t total_c = cycle_ops::read() + (uint64_t(cycleh_ops::read()) << 32);
    uint64_t total_instr = instret_ops::read() + (uint64_t(instreth_ops::read()) << 32);

    uint32_t risc_ld_haz = mhpmcounter13::read();
    uint32_t risc_mul_haz = mhpmcounter12::read();
    uint32_t risc_jr_haz = mhpmcounter14::read();
    uint32_t risc_load = mhpmcounter5::read();
    uint32_t risc_store = mhpmcounter6::read();
    uint32_t risc_imiss = mhpmcounter4::read();
    uint32_t risc_dmiss = mhpmcounter3::read();
    uint32_t risc_jump = mhpmcounter7::read();
    uint32_t risc_branch = mhpmcounter8::read();
    uint32_t risc_branch_taken = mhpmcounter9::read();
    uint32_t risc_compressed = mhpmcounter10::read();
    uint32_t risc_multicycle = mhpmcounter11::read();
    uint32_t risc_csr = mhpmcounter15::read();

    printf("\nAll Cycles in EIS-V Domain:\n");
    printf("  [VPRO AUX]  (any) Lane active:               %llu Cycles \n", lane);
    printf("  [VPRO AUX]  (any) DMA active:                %llu Cycles \n", dma);
    printf("  [VPRO AUX]  (any) Lane AND (any) DMA active: %llu Cycles \n", both);
    printf("  [VPRO AUX]  Total:                           %llu Cycles \n", tot);
    printf("  [EIS-V AUX] Total:                           %llu Cycles \n", tot2);
    printf("\n");
    printf("  [VPRO AUX]  (any) Lane Busy: %llu.%llu%% \n", (100*lane)/tot, (10000*lane)/tot - ((100*lane)/tot)*100);
    printf("  [VPRO AUX]  (any) DMA Busy:  %llu.%llu%% \n", (100*dma)/tot, (10000*dma)/tot - ((100*dma)/tot)*100);
    printf("\n");
    printf("  [VPRO AUX]  (only) Lane:     %llu.%llu%% \n", (100*lane_o)/tot, (10000*lane_o)/tot - ((100*lane_o)/tot)*100);
    printf("  [VPRO AUX]  (both) DMA+Lane: %llu.%llu%% \n", (100*both)/tot, (10000*both)/tot - ((100*both)/tot)*100);
    printf("  [VPRO AUX]  (only) DMA:      %llu.%llu%% \n", (100*dma_o)/tot, (10000*dma_o)/tot - ((100*dma_o)/tot)*100);
    printf("  [VPRO AUX]  (only) EIS-V:    %llu.%llu%% \n", (100*(tot-lane_o-both-dma_o))/tot, (10000*(tot-lane_o-both-dma_o))/tot - ((100*(tot-lane_o-both-dma_o))/tot)*100);
    printf("\n");
    printf("\n");

// all results for copy paste ...
//    printf("\e[95m %iU %llu.%llu %llu.%llu %llu.%llu\n\e[m", get_gpr_units(),
//           (100*dma_o)/tot, (10000*dma_o)/tot - ((100*dma_o)/tot)*100,
//           (100*both)/tot, (10000*both)/tot - ((100*both)/tot)*100,
//           (100*lane_o)/tot, (10000*lane_o)/tot - ((100*lane_o)/tot)*100);
//    printf("\n");

    printf("  [EIS-V CSR] Busy: %lli Instructions/ %lli Cycles = %lli.%lli %%\n", total_instr, total_c, (100*total_instr)/total_c, (10000*total_instr)/total_c - ((100*total_instr)/total_c)*100);
    printf("  [EIS-V CSR] Load: %li,  Store: %li [Instructions]\n", risc_load, risc_store);
    printf("  [EIS-V CSR] Jumps: %li [Instructions]\n", risc_jump);
    printf("  [EIS-V CSR] Branches: %li, Taken: %li (%lli.%lli %%) [Instructions]\n", risc_branch, risc_branch_taken, (100*uint64_t(risc_branch_taken))/risc_branch, (10000*uint64_t(risc_branch_taken))/risc_branch - ((100*risc_branch_taken)/risc_branch)*100);
    printf("  [EIS-V CSR] Compressed: %li (%lli.%lli %%) [Instructions]\n", risc_compressed, (100*uint64_t(risc_compressed))/uint32_t(total_instr), (10000*uint64_t(risc_compressed))/uint32_t(total_instr) - ((100*risc_compressed)/uint32_t(total_instr))*100);
    printf("  [EIS-V CSR] CSR: %li [Instructions]\n", risc_csr);
    printf("  [EIS-V CSR] RISC-V I Wait/Miss: %li Cycles\n", risc_imiss);
    printf("  [EIS-V CSR] RISC-V D Wait/Miss: %li Cycles\n", risc_dmiss);
    printf("  [Cache] I Cache Miss: %li, Hit: %li\n", aux_icache_miss_cnt(), aux_icache_hit_cnt());
    printf("  [Cache] D Cache Miss: %li, Hit: %li\n",  aux_dcache_miss_cnt(), aux_dcache_hit_cnt());
    printf("  [EIS-V CSR] Multicycle stalls (DIV): %li Cycles\n", risc_multicycle);
    printf("  [EIS-V CSR] Hazard Cycles: Load: %li, Jump-Return: %li, Mult: %li\n", risc_ld_haz, risc_jr_haz, risc_mul_haz);
    printf("\n");

    dma_print_access_counters();

//    uint32_t t;
//    t = misa_ops::read();
//    printf("misa_ops: %i | %x\n", t, t);
//    t = mvendorid_ops::read();
//    printf("mvendorid_ops: %i | %x\n", t, t);
//    t = marchid_ops::read();
//    printf("marchid_ops: %i | %x\n", t, t);
//    t = mimpid_ops::read();
//    printf("mimpid_ops: %i | %x\n", t, t);
//    t = mhartid_ops::read();
//    printf("mhartid_ops: %i | %x\n", t, t);
//
//    t = mstatus_ops::read();
//    printf("mstatus_ops: %i | %x\n", t, t);
//    t = mtvec_ops::read();
//    printf("mtvec_ops: %i | %x\n", t, t);
//    t = mip_ops::read();
//    printf("mip_ops: %i | %x\n", t, t);
//    t = mscratch_ops::read();
//    printf("mscratch_ops: %i | %x\n", t, t);
//    t = mepc_ops::read();
//    printf("mepc_ops: %i | %x\n", t, t);
//    t = mcause_ops::read();
//    printf("mcause_ops: %i | %x\n", t, t);
//    t = mtval_ops::read();
//    printf("mtval_ops: %i | %x\n", t, t);
//
//    t = mcountinhibit_ops::read();
//    printf("mcountinhibit_ops: %i | %x\n", t, t);
//
//    t = cycle_ops::read();
//    printf("cycle_ops: %i | %x\n", t, t);
//    t = cycleh_ops::read();
//    printf("cycleh_ops: %i | %x\n", t, t);
//
//    t = instret_ops::read();
//    printf("instret_ops: %i | %x\n", t, t);
//    t = instreth_ops::read();
//    printf("instreth_ops: %i | %x\n", t, t);
//
//    t = mhpmcounter3::read();
//    printf("mhpmcounter3: %i | %x\n", t, t);
//    t = mhpmcounter4::read();
//    printf("mhpmcounter4: %i | %x\n", t, t);
//    t = mhpmcounter5::read();
//    printf("mhpmcounter5: %i | %x\n", t, t);
//    t = mhpmcounter6::read();
//    printf("mhpmcounter6: %i | %x\n", t, t);
//    t = mhpmcounter7::read();
//    printf("mhpmcounter7: %i | %x\n", t, t);
//    t = mhpmcounter8::read();
//    printf("mhpmcounter8: %i | %x\n", t, t);
//    t = mhpmcounter9::read();
//    printf("mhpmcounter9: %i | %x\n", t, t);
//    t = mhpmcounter10::read();
//    printf("mhpmcounter10: %i | %x\n", t, t);
//    t = mhpmcounter11::read();
//    printf("mhpmcounter11: %i | %x\n", t, t);
//    t = mhpmcounter12::read();
//    printf("mhpmcounter12: %i | %x\n", t, t);
//    t = mhpmcounter13::read();
//    printf("mhpmcounter13: %i | %x\n", t, t);
//    t = mhpmcounter14::read();
//    printf("mhpmcounter14: %i | %x\n", t, t);
//    t = mhpmcounter15::read();
//    printf("mhpmcounter15: %i | %x\n", t, t);
//    t = mhpmcounter16::read();
//    printf("mhpmcounter16: %i | %x\n", t, t);
//    t = mhpmcounter17::read();
//    printf("mhpmcounter17: %i | %x\n", t, t);
//    t = mhpmcounter18::read();
//    printf("mhpmcounter18: %i | %x\n", t, t);
//    t = mhpmcounter19::read();
//    printf("mhpmcounter19: %i | %x\n", t, t);
//    t = mhpmcounter20::read();
//    printf("mhpmcounter20: %i | %x\n", t, t);
//    t = mhpmcounter21::read();
//    printf("mhpmcounter21: %i | %x\n", t, t);
//    t = mhpmcounter22::read();
//    printf("mhpmcounter22: %i | %x\n", t, t);
//    t = mhpmcounter23::read();
//    printf("mhpmcounter23: %i | %x\n", t, t);
//    t = mhpmcounter24::read();
//    printf("mhpmcounter24: %i | %x\n", t, t);
//    t = mhpmcounter25::read();
//    printf("mhpmcounter25: %i | %x\n", t, t);
//    t = mhpmcounter26::read();
//    printf("mhpmcounter26: %i | %x\n", t, t);
//    t = mhpmcounter27::read();
//    printf("mhpmcounter27: %i | %x\n", t, t);
//    t = mhpmcounter28::read();
//    printf("mhpmcounter28: %i | %x\n", t, t);
//    t = mhpmcounter29::read();
//    printf("mhpmcounter29: %i | %x\n", t, t);
//    t = mhpmcounter30::read();
//    printf("mhpmcounter30: %i | %x\n", t, t);
//    t = mhpmcounter31::read();
//    printf("mhpmcounter31: %i | %x\n", t, t);
//
////    t = mhpmevent3_ops::read();
////    printf("mhpmevent3_ops: %i | %x\n", t, t);
//
////    t = time_ops::read();
////    printf("time_ops: %i | %x\n", t, t);
////    t = timeh_ops::read();
////    printf("timeh_ops: %i | %x\n", t, t);
}

inline void __attribute__((always_inline)) sim_stat_reset() {
    aux_reset_all_stats();
    aux_clr_sys_time();
//    aux_clr_cycle_cnt();
}

inline void prefetch_dcache(uint32_t addr){
    DCACHE_PREFETCH_TRIGER = addr;
}

#else   // SIMULATION

inline void aux_clr_icache_miss_cnt(){}
inline void aux_clr_icache_hit_cnt(){}
inline void aux_clr_dcache_miss_cnt(){}
inline void aux_clr_dcache_hit_cnt(){}

inline uint32_t aux_icache_miss_cnt(){
    return 0;
}
inline uint32_t aux_icache_hit_cnt(){
    return 0;
}
inline uint32_t aux_dcache_miss_cnt(){
    return 0;
}
inline uint32_t aux_dcache_hit_cnt(){
    return 0;
}

inline void prefetch_dcache(uint32_t addr){
    // nothing to do in ISS
}

#endif

#endif // eisv_aux_h
