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
// ###############################################################
// # vpro_aux.h - VPRO system helper functions                   #
// ###############################################################

#ifndef vpro_aux_h
#define vpro_aux_h

// IO map
#include "vpro_defs.h"
#include "vpro_cmd_defs.h"
#include "vpro_globals.h"
#include "vpro_special_register_enums.h"

#include <cstdio>

#ifndef SIMULATION

#include "cstdlib"

#pragma GCC diagnostic ignored "-Warray-bounds"

#define REDBG    "\033[41m"
#define RED    "\033[91m" // 31
#define ORANGE "\033[93m"
#define BLACK  "\033[22;30m"
#define GREEN  "\033[32m"
#define LGREEN "\033[92m"
#define LYELLOW  "\033[93m"
#define YELLOW  "\033[33m"
#define MAGENTA "\033[95m" // or 35?
#define BLUE   "\033[36m" // 34 hard to read on thin clients
#define LBLUE   "\033[96m" // 94 hard to read on thin clients

#define INVERTED "\033[7m"
#define UNDERLINED "\033[4m"
#define BOLD "\033[1m"
#define RESET_COLOR "\033[m"

#define LIGHT "\033[2m"
//FIXME crash because of global define NORMAL and usage of same identifier in opencv/core.hpp
#define NORMAL_ "\033[22m"

inline void __attribute__((always_inline)) sim_printf(const char *format){
    printf(format);
}
template<typename... Args> void sim_printf(const char *format, Args... args){
    printf(format, args...);
}

inline void __attribute__((always_inline)) printf_error(const char *format){
    printf(REDBG);
    printf(format);
    printf(RESET_COLOR);
}
template<typename... Args> void printf_error(const char *format, Args... args){
    printf(REDBG);
    printf(format, args...);
    printf(RESET_COLOR);
}

inline void __attribute__((always_inline)) printf_warning(const char *format){
    printf(ORANGE);
    printf(format);
    printf(RESET_COLOR);
}
template<typename... Args> void printf_warning(const char *format, Args... args){
    printf(ORANGE);
    printf(format, args...);
    printf(RESET_COLOR);
}

inline void __attribute__((always_inline)) printf_info(const char *format){
    printf(LBLUE);
    printf(format);
    printf(RESET_COLOR);
}
template<typename... Args> void printf_info(const char *format, Args... args){
    printf(LBLUE);
    printf(format, args...);
    printf(RESET_COLOR);
}

inline void __attribute__((always_inline)) printf_success(const char *format){
    printf(GREEN);
    printf(format);
    printf(RESET_COLOR);
}
template<typename... Args> void printf_success(const char *format, Args... args){
    printf(GREEN);
    printf(format, args...);
    printf(RESET_COLOR);
}

static bool default_flags[4] = {0,0,0,0};

inline void __attribute__((always_inline))  vpro_sync(){
//    volatile auto tmp = VPRO_SYNC;
    asm("lw x0, 0(%0) # VPRO_SYNC manual inline asm \n" : : "r" (VPRO_SYNC_ADDR));
}

// ***************************************************************************************************************
// DMA elementary function prototypes
// ***************************************************************************************************************
inline void __attribute__((always_inline)) dma_set_pad_widths(uint32_t top, uint32_t right, uint32_t bottom, uint32_t left) {
    IDMA_PAD_TOP_SIZE = top;
    IDMA_PAD_BOTTOM_SIZE = bottom;
    IDMA_PAD_RIGHT_SIZE = right;
    IDMA_PAD_LEFT_SIZE = left;
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
}

inline void __attribute__((always_inline)) dma_set_pad_value(uint32_t value) {
    IDMA_PAD_VALUE = value;
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
}

// ---------------------------------------------------------------------------------
// Blocks until all DMA transactions in cluster are done
// ---------------------------------------------------------------------------------
inline void __attribute__((always_inline)) dma_wait_to_finish(uint32_t cluster_mask = 0xffffffff) {
    VPRO_BUSY_MASK_CL = cluster_mask;
    // wait additional cycles for CDC of busy signal
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");

    while (VPRO_BUSY_MASKED_DMA != 0) {
        continue;
    }
}
inline void __attribute__((always_inline))  vpro_dma_sync(){
//    volatile auto tmp = VPRO_DMA_SYNC;
    asm("lw x0, 0(%0) # VPRO_DMA_SYNC manual inline asm \n" : : "r" (VPRO_DMA_SYNC_ADDR));
}

inline void __attribute__((always_inline)) dma_dcache_short_command(const void *ptr) {
    while (IDMA_COMMAND_FSM != 0){} // wait to finish current block loop
    IDMA_COMMAND_DCACHE = uint32_t(intptr_t(ptr));
// #include "dma_cmd_struct.h"
}

inline void __attribute__((always_inline)) dma_block_size(const uint32_t &size) {
    while (IDMA_COMMAND_FSM != 0){} // wait to finish current block loop
    IDMA_COMMAND_BLOCK_SIZE = size;
}

inline void __attribute__((always_inline)) dma_block_addr_trigger(const void *ptr) {
    while (IDMA_COMMAND_FSM != 0){} // wait to finish current block loop
    IDMA_COMMAND_BLOCK_ADDR_TRIGGER = uint32_t(intptr_t(ptr));

    // to enable sync to receive dma fsm busy signal...
//    asm volatile ("nop");
//    asm volatile ("nop");
}

inline void __attribute__((always_inline)) rv_dcache_flush() {
    printf("deprecated: rv_dcache_flush(), USE EIS-V Lib: aux_flush_dcache()\n");
    RV_DCACHE_FLUSH = 1;
}
inline void __attribute__((always_inline)) rv_dcache_clear() {
    printf("deprecated: rv_dcache_clear(), USE EIS-V Lib: aux_clr_dcache()\n");
    RV_DCACHE_CLEAR = 1;
}
inline void __attribute__((always_inline)) rv_icache_clear() {
    printf("deprecated: rv_icache_clear(), USE EIS-V Lib: aux_clr_icache()\n");
    RV_ICACHE_CLEAR = 1;
}


// ---------------------------------------------------------------------------------
// DMA transfers
// ---------------------------------------------------------------------------------

// cnn converter also defines the dma command struct (same name).
// fix to define the dma command struct.
// in this file, the dma command struct is only accessible in the LIMITED_VISIBILITY namespace

#ifndef DMA_CMD_STRUCT_H_TMP    // avoid multiple definitions inside LIMITED_VISIBILITY
#if not defined(DMA_CMD_STRUCT_H)   // undefine header load after include
#define DMA_CMD_STRUCT_H_TMP
#else
#undef DMA_CMD_STRUCT_H // undef to enable reload into LIMITED_VISIBILITY
#endif
namespace LIMITED_VISIBILITY{
    #include "dma_cmd_struct.h"
}
#if defined(DMA_CMD_STRUCT_H_TMP)
#undef DMA_CMD_STRUCT_H
#endif
#endif

inline void __attribute__((always_inline)) dma_e2l_1d(uint32_t cluster_mask, uint32_t unit_mask, intptr_t ext_base, uint32_t loc_base, uint32_t x_size){
    volatile LIMITED_VISIBILITY::COMMAND_DMA::COMMAND_DMA __attribute__ ((aligned (32))) dmas{
        LIMITED_VISIBILITY::COMMAND_DMA::DMA_DIRECTION::e2l1D,
        (uint32_t)cluster_mask,
        (uint32_t)unit_mask,
        (uint32_t)ext_base,
        (uint32_t)loc_base,
        (uint16_t)x_size,
        1,
        1,
        0
    };
    dma_block_size(1);
    dma_block_addr_trigger((void *)(&dmas));
}

inline void __attribute__((always_inline)) dma_e2l_2d(uint32_t cluster_mask, uint32_t unit_mask, intptr_t ext_base, uint32_t loc_base, uint32_t x_size,
                uint32_t y_size, uint32_t y_leap, const bool pad_flags[4] = default_flags){
    uint8_t pad = pad_flags[LIMITED_VISIBILITY::COMMAND_DMA::PAD::TOP];
    pad |= (pad_flags[LIMITED_VISIBILITY::COMMAND_DMA::PAD::RIGHT] << 1);
    pad |= (pad_flags[LIMITED_VISIBILITY::COMMAND_DMA::PAD::BOTTOM] << 2);
    pad |= (pad_flags[LIMITED_VISIBILITY::COMMAND_DMA::PAD::LEFT] << 3);
    volatile LIMITED_VISIBILITY::COMMAND_DMA::COMMAND_DMA __attribute__ ((aligned (32))) dmas{
        LIMITED_VISIBILITY::COMMAND_DMA::DMA_DIRECTION::e2l2D,
        (uint32_t)cluster_mask,
        (uint32_t)unit_mask,
        (uint32_t)ext_base,
        (uint32_t)loc_base,
        (uint16_t)x_size,
        (uint16_t)y_size,
        (uint16_t)y_leap,
        pad
    };
    dma_block_size(1);
    dma_block_addr_trigger((void *)(&dmas));
}

inline void __attribute__((always_inline)) dma_l2e_1d(uint32_t cluster_mask, uint32_t unit_mask, intptr_t ext_base, uint32_t loc_base, uint32_t x_size){
    volatile LIMITED_VISIBILITY::COMMAND_DMA::COMMAND_DMA __attribute__ ((aligned (32))) dmas{
        LIMITED_VISIBILITY::COMMAND_DMA::DMA_DIRECTION::l2e1D,
        (uint32_t)cluster_mask,
        (uint32_t)unit_mask,
        (uint32_t)ext_base,
        (uint32_t)loc_base,
        (uint16_t)x_size,
        1,
        1,
        0
    };
    dma_block_size(1);
    dma_block_addr_trigger((void *)(&dmas));
}

inline void __attribute__((always_inline)) dma_l2e_2d(uint32_t cluster_mask, uint32_t unit_mask, intptr_t ext_base, uint32_t loc_base, uint32_t x_size,
                uint32_t y_size, uint32_t y_leap){
    volatile LIMITED_VISIBILITY::COMMAND_DMA::COMMAND_DMA __attribute__ ((aligned (32))) dmas{
        LIMITED_VISIBILITY::COMMAND_DMA::DMA_DIRECTION::l2e2D,
        (uint32_t)cluster_mask,
        (uint32_t)unit_mask,
        (uint32_t)ext_base,
        (uint32_t)loc_base,
        (uint16_t)x_size,
        (uint16_t)y_size,
        (uint16_t)y_leap,
        0
    };
    dma_block_size(1);
    dma_block_addr_trigger((void *)(&dmas));
}

inline uint32_t __attribute__((always_inline)) get_gpr_clusters();

inline void __attribute__((always_inline)) dma_print_access_counters(){
    for(unsigned int cluster = 0; cluster < get_gpr_clusters(); cluster++){
         printf("  [DMA_ACCESS_COUNTER] DMA %i, [Cycles] Read Hit %i Miss %i, Write Hit %i Miss %i\n", cluster, IDMA_READ_HIT_CYCLES, IDMA_READ_MISS_CYCLES, IDMA_WRITE_HIT_CYCLES, IDMA_WRITE_MISS_CYCLES);
//         printf("  [DMA_ACCESS_COUNTER]     READ_HIT_CYCLES:   %i\n", IDMA_READ_HIT_CYCLES);
//         printf("  [DMA_ACCESS_COUNTER]     READ_MISS_CYCLES:  %i\n", IDMA_READ_MISS_CYCLES);
//         printf("  [DMA_ACCESS_COUNTER]     WRITE_HIT_CYCLES:  %i\n", IDMA_WRITE_HIT_CYCLES);
//         printf("  [DMA_ACCESS_COUNTER]     WRITE_MISS_CYCLES: %i\n", IDMA_WRITE_MISS_CYCLES);
    }
     printf("\n");
}

inline void __attribute__((always_inline)) dma_reset_access_counters(){
    IDMA_READ_HIT_CYCLES = 1;
}


// ***************************************************************************************************************
// DCMA elementary function prototypes
// ***************************************************************************************************************

inline void __attribute__((always_inline)) dcma_flush(){
    while (*((volatile uint32_t *) ((uint32_t)(intptr_t)(DCMA_WAIT_BUSY_ADDR)))){}
    *((volatile uint32_t *) ((uint32_t)(intptr_t)(DCMA_FLUSH_ADDR))) = 0;
    while (*((volatile uint32_t *) ((uint32_t)(intptr_t)(DCMA_WAIT_BUSY_ADDR)))){}
}

inline void __attribute__((always_inline)) dcma_reset(){
    while (*((volatile uint32_t *) ((uint32_t)(intptr_t)(DCMA_WAIT_BUSY_ADDR)))){}
    *((volatile uint32_t *) ((uint32_t)(intptr_t)(DCMA_RESET_ADDR))) = 0;
}

// ***************************************************************************************************************
// VPRO Status/Control Register
// ***************************************************************************************************************
inline void __attribute__((always_inline)) vpro_wait_busy(uint32_t cluster_mask = 0xffffffff, uint32_t unit_mask = 0xffffffff) {
    VPRO_BUSY_MASK_CL = cluster_mask;
    // wait additional cycles for CDC of busy signal
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");

    assert(unit_mask == 0xffffffff);

    while (VPRO_BUSY_MASKED_VPRO != 0) {
        continue;
    }
}
inline void __attribute__((always_inline))  vpro_wait_busy(uint32_t cluster_mask, uint32_t unit_mask, int cycle){
    printf("vpro_wait_busy in HW not available with cycle paramter!\n");
}
inline void __attribute__((always_inline))  vpro_lane_sync(){
//    volatile auto tmp = VPRO_LANE_SYNC;
    asm("lw x0, 0(%0) # VPRO_LANE_SYNC manual inline asm \n" : : "r" (VPRO_LANE_SYNC_ADDR));
}
inline void __attribute__((always_inline))  vpro_pipeline_wait(){
    printf("vpro_pipeline_wait in HW not available!\n");
}

inline void __attribute__((always_inline)) vpro_set_cluster_mask(uint32_t idmask = 0xffffffff) {
    VPRO_CLUSTER_MASK = idmask;
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    // required since IO transfer (the idmask) is sent in the processors MEM stage while
    // vector instructions are issued in the ID stage (use NOPS to make sure idmask is set
    // when the vector instruction is issued)
}

inline void __attribute__((always_inline)) vpro_set_unit_mask(uint32_t idmask = 0xffffffff) {
    VPRO_UNIT_MASK = idmask;
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    // required since IO transfer (the idmask) is sent in the processors MEM stage while
    // vector instructions are issued in the ID stage (use NOPS to make sure idmask is set
    // when the vector instruction is issued)
}


inline void __attribute__((always_inline)) vpro_set_idmask(uint32_t idmask = 0xffffffff) {  // backward compatibility
    vpro_set_cluster_mask(idmask);
}

inline uint32_t __attribute__((always_inline)) vpro_get_unit_mask(){
    return VPRO_UNIT_MASK;
}
inline uint32_t __attribute__((always_inline)) vpro_get_cluster_mask(){
    return VPRO_CLUSTER_MASK;
}

// //  LOOPER
// inline void __attribute__((always_inline)) vpro_loop_start(uint32_t start, uint32_t end, uint32_t increment_step){
//     VPRO_LOOP_START_REG_1 = (end << 16u) | increment_step;
// }
//
// inline void __attribute__((always_inline)) vpro_loop_end(){
//     VPRO_LOOP_END = 1;
//     asm volatile ( "nop");   // wait for looper busy command to pass through to mips
//     asm volatile ( "nop");
//     asm volatile ( "nop");
//     asm volatile ( "nop");
//     asm volatile ( "nop");
//     asm volatile ( "nop");
//     asm volatile ( "nop");
//     asm volatile ( "nop");
// //    vpro_wait_busy(0xffffffff, 0xffffffff);
// }
//
// inline void __attribute__((always_inline)) vpro_loop_mask(uint32_t src1_mask, uint32_t src2_mask, uint32_t dst_mask){
//     VPRO_LOOP_MASK = (src1_mask << 20u) | (src2_mask << 10u) | dst_mask;
// }


// VPRO Command 4 parameters -> 3D
inline void __attribute__((always_inline)) __vpro(uint32_t id, uint32_t blocking, uint32_t is_chain,
                                                  uint32_t func, uint32_t flag_update,
                                                  uint32_t dst, uint32_t src1, uint32_t src2,
                                                  uint32_t x_end, uint32_t y_end, uint32_t z_end) {
    // printf("(vpro_aux)VPRO: %u %u %u %u %u %u %u %u %u %u %u\n", id, blocking, is_chain, func, flag_update, dst, src1, src2, x_end, y_end, z_end);
    // src1/src2/dst use 10-bit offset, 6-bit alpha, 6-bit beta, 6-bit gamma
    // if not -> fail due to concat IO Write from RISC!
    assert(ISA_X_END_LENGTH == 6 && ISA_Y_END_LENGTH == 6 && ISA_Z_END_LENGTH == 10);
    assert(ISA_BETA_LENGTH == 6 && ISA_ALPHA_LENGTH == 6 && ISA_OFFSET_LENGTH == 10 && ISA_GAMMA_LENGTH == 6);

    assert (id <= 0xf);
    assert (x_end <= 0x3f);
    assert (y_end <= 0x3f);
    assert (z_end <= 0x3ff);

    // src1, src2, dst without gamma
    VPRO_CMD_REGISTER_4_0 = ((src1 & 0xffffffc0) << (7-6)) | (x_end << 1) | is_chain;
    VPRO_CMD_REGISTER_4_1 = ((src2 & 0xffffffc0) << (7-6)) | (y_end << 1) | blocking;
    VPRO_CMD_REGISTER_4_2 = ((dst & 0xffffffc0) << (10-6)) | (func << 4) | (id << 1) | flag_update;

    uint32_t dst_sel = (dst >> ISA_COMPLEX_LENGTH_3D) & 0x7;
    // remaining 3d parameters (gamma, zend)
    VPRO_CMD_REGISTER_4_3 = ((dst & 0x3f) << 26) | ((src1 & 0x3f) << 20) | ((src2 & 0x3f) << 14) | (dst_sel << 10) | z_end;
}

/**
 * 3 IO Write
*/
inline void __attribute__((always_inline)) __vpro(uint32_t id, uint32_t blocking, uint32_t is_chain,
                                                  uint32_t func, uint32_t flag_update,
                                                  uint32_t dst, uint32_t src1, uint32_t src2,
                                                  uint32_t x_end, uint32_t y_end) {
    // src1/src2/dst use 10-bit offset, 6-bit alpha, 6-bit beta, 6-bit gamma
    // if not -> fail due to concat IO Write from RISC!
    assert(ISA_X_END_LENGTH == 6 && ISA_Y_END_LENGTH == 6 && ISA_Z_END_LENGTH == 10);
    assert(ISA_BETA_LENGTH == 6 && ISA_ALPHA_LENGTH == 6 && ISA_OFFSET_LENGTH == 10 && ISA_GAMMA_LENGTH == 6);

    assert (id <= 0x7);
    assert (x_end <= 0x3f);
    assert (y_end <= 0x3f);

    VPRO_CMD_REGISTER_3_0 = (src1 << 7) | (x_end << 1) | is_chain;
    VPRO_CMD_REGISTER_3_1 = (src2 << 7) | (y_end << 1) | blocking;
    VPRO_CMD_REGISTER_3_2 = (dst << 10) | (func << 4) | (id << 1) | flag_update;
}


inline void __attribute__((always_inline)) vpro_mac_h_bit_shift(uint32_t shift = 24) {
    VPRO_MAC_SHIFT_REG = shift;
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    // required since IO transfer (the idmask) is sent in the processors MEM stage while
    // vector instructions are issued in the ID stage (use NOPS to make sure idmask is set
//    // when the vector instruction is issued)
}

inline void __attribute__((always_inline)) vpro_mul_h_bit_shift(uint32_t shift = 24) {
    VPRO_MUL_SHIFT_REG = shift;
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    // required since IO transfer (the idmask) is sent in the processors MEM stage while
    // vector instructions are issued in the ID stage (use NOPS to make sure idmask is set
    // when the vector instruction is issued)
}

inline VPRO::MAC_INIT_SOURCE __attribute__((always_inline)) vpro_get_mac_init_source(){
     return static_cast<VPRO::MAC_INIT_SOURCE>(VPRO_MAC_INIT_SOURCE);
}

inline void __attribute__((always_inline)) vpro_set_mac_init_source(VPRO::MAC_INIT_SOURCE source = VPRO::MAC_INIT_SOURCE::ZERO) {
    VPRO_MAC_INIT_SOURCE = source;
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    // required since IO transfer (the idmask) is sent in the processors MEM stage while
    // vector instructions are issued in the ID stage (use NOPS to make sure idmask is set
    // when the vector instruction is issued)
}


inline VPRO::MAC_RESET_MODE __attribute__((always_inline)) vpro_get_mac_reset_mode(){
     return static_cast<VPRO::MAC_RESET_MODE>(VPRO_MAC_RESET_MODE);
}

inline void __attribute__((always_inline)) vpro_set_mac_reset_mode(VPRO::MAC_RESET_MODE mode = VPRO::MAC_RESET_MODE::Z_INCREMENT) {
    VPRO_MAC_RESET_MODE = mode;
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    // required since IO transfer (the idmask) is sent in the processors MEM stage while
    // vector instructions are issued in the ID stage (use NOPS to make sure idmask is set
    // when the vector instruction is issued)
}

// ***************************************************************************************************************
// Simulator dummies
// ***************************************************************************************************************
inline void __attribute__((always_inline)) sim_finish_and_wait_step(int clusters){}
inline void __attribute__((always_inline)) sim_wait_step(void){}
inline void __attribute__((always_inline)) sim_wait_step(bool finish = false, const char* msg = ""){}

inline int __attribute__((always_inline)) sim_init(int (*main_fkt)(int, char**), int &argc, char* argv[], int main_memory_size = 1024*1024*1024, int local_memory_size = 8192, int register_file_size = 1024, int cluster_count = 1, int vector_unit_count = 1, int vector_lane_count = 2, int pipeline_depth = 10){return 0;}
inline void __attribute__((always_inline)) sim_start(void){}
inline void __attribute__((always_inline)) sim_stop(void){}
inline void __attribute__((always_inline)) sim_stop(bool){}

inline void __attribute__((always_inline)) sim_dump_local_memory(uint32_t cluster, uint32_t unit){}
inline void __attribute__((always_inline)) sim_dump_register_file(uint32_t cluster, uint32_t unit, uint32_t lane){}
inline void __attribute__((always_inline)) sim_dump_queue(uint32_t cluster, uint32_t unit){}

#include "../riscv/eisv_defs.h"
inline uint32_t __attribute__((always_inline)) get_gpr_vpro_freq() { return *((volatile uint32_t *)(GP_REGISTERS_ADDR + 6 * 4)); }
inline uint32_t __attribute__((always_inline)) get_gpr_risc_freq() { return *((volatile uint32_t *)(GP_REGISTERS_ADDR + 7 * 4)); }
inline uint32_t __attribute__((always_inline)) get_gpr_dma_freq() { return *((volatile uint32_t *)(GP_REGISTERS_ADDR + 8 * 4)); }
inline uint32_t __attribute__((always_inline)) get_gpr_axi_freq() { return *((volatile uint32_t *)(GP_REGISTERS_ADDR + 9 * 4)); }
inline uint32_t __attribute__((always_inline)) get_gpr_git_sha() { return *((volatile uint32_t *)(GP_REGISTERS_ADDR + 14 * 4)); }
inline uint32_t __attribute__((always_inline)) get_gpr_hw_build_date() { return *((volatile uint32_t *)(GP_REGISTERS_ADDR + 2 * 4)); }
inline uint32_t __attribute__((always_inline)) get_gpr_clusters() { return *((volatile uint32_t *)(GP_REGISTERS_ADDR + 3 * 4)); }
inline uint32_t __attribute__((always_inline)) get_gpr_units() { return *((volatile uint32_t *)(GP_REGISTERS_ADDR + 4 * 4)); }
inline uint32_t __attribute__((always_inline)) get_gpr_laness() { return *((volatile uint32_t *)(GP_REGISTERS_ADDR + 5 * 4)); }
inline uint32_t __attribute__((always_inline)) get_gpr_DCMA_off() { return *((volatile uint32_t *)(GP_REGISTERS_ADDR + 16 * 4)); }
inline uint32_t __attribute__((always_inline)) get_gpr_DCMA_nr_rams() { return *((volatile uint32_t *)(GP_REGISTERS_ADDR + 17 * 4)); }
inline uint32_t __attribute__((always_inline)) get_gpr_DCMA_line_size() { return *((volatile uint32_t *)(GP_REGISTERS_ADDR + 18 * 4)); }
inline uint32_t __attribute__((always_inline)) get_gpr_DCMA_associativity() { return *((volatile uint32_t *)(GP_REGISTERS_ADDR + 19 * 4)); }

inline bool __attribute__((always_inline)) sim_min_req(int cluster_cnt, int unit_cnt, int lane_cnt){
     int clusters_available = get_gpr_clusters();
     int units_available = get_gpr_units();
     int lanes_available = get_gpr_laness();
     bool enaugh = true;
     if (cluster_cnt > clusters_available){
         enaugh = false;
         printf_error("[Min Req Check Failed] App needs %i Clusters. HW has %i Clusters only!\n", cluster_cnt, clusters_available);
     }
     if (unit_cnt > units_available){
         enaugh = false;
         printf_error("[Min Req Check Failed] App needs %i Units. HW has %i Units only!\n", unit_cnt, units_available);
     }
     if (lane_cnt > lanes_available){
         enaugh = false;
         printf_error("[Min Req Check Failed] App needs %i Lanes. HW has %i Lanes only!\n", lane_cnt, lanes_available);
     }
     if (enaugh){
         printf("[Min Req Check Succeeds] %i Clusters, %i Units, %i Lanes\n", cluster_cnt, unit_cnt, lane_cnt);
     } else {
         exit(1);
     }
     return enaugh;
}

#endif // ndef simulation
#endif // vpro_aux_h
