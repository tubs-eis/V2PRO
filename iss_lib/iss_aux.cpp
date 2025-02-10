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

#include "iss_aux.h"

#include <riscv/eisv_defs.h>
#include <vpro/vpro_defs.h>
#include <cstdint>
#include <cstdlib>
#include "simulator/helper/debugHelper.h"

struct sync_function {
    uint32_t data[1];
};

void vpro_sync() {
    vpro_lane_sync();
    vpro_dma_sync();
}

void dma_set_pad_widths(uint32_t top, uint32_t right, uint32_t bottom, uint32_t left) {
    assert(top < 8191);
    assert(right < 8191);
    assert(bottom < 8191);
    assert(left < 8191);
    core_->io_write(uint32_t(intptr_t(&IDMA_PAD_TOP_SIZE)), top);
    core_->io_write(uint32_t(intptr_t(&IDMA_PAD_BOTTOM_SIZE)), bottom);
    core_->io_write(uint32_t(intptr_t(&IDMA_PAD_RIGHT_SIZE)), right);
    core_->io_write(uint32_t(intptr_t(&IDMA_PAD_LEFT_SIZE)), left);
}

void dma_set_pad_value(int16_t value) {
    assert(value < 8191);
    core_->io_write(uint32_t(intptr_t(&IDMA_PAD_VALUE)), value);
}

void dma_wait_to_finish(uint32_t cluster_mask) {
    core_->setWaitingToFinish(true);

    if (core_->DMA_gen_trace) {
        uint32_t cycle_stamp = core_->getTime() / core_->getDMAClockPeriod();
        core_->dma_sync_trace << cycle_stamp << "\n";
        core_->dma_sync_trace.flush();
    }

    core_->io_write(VPRO_BUSY_MASK_CL_ADDR, cluster_mask);
    while (core_->io_read(VPRO_BUSY_MASKED_DMA_ADDR) != 0) {}
    core_->setWaitingToFinish(false);

    if (CREATE_CMD_HISTORY_FILE) {
        QString cmd_description = "DMA Sync";
        *CMD_HISTORY_FILE_STREAM << cmd_description << "\n";
    }
    if (CREATE_CMD_PRE_GEN) {
        sync_function pre_gen_sync;
        pre_gen_sync.data[0] = 513;
        *PRE_GEN_FILE_STREAM << pre_gen_sync.data[0];
    }
}

void dma_wait_to_finish(uint32_t cluster_mask, int cycle) {
    printf("cycle:%i dma_wait_to_finish %u \n", cycle, cluster_mask);
    dma_wait_to_finish(cluster_mask);
}

void vpro_dma_sync() {
    dma_wait_to_finish(0xffffffff);
}

// *** VPRO DMA *** //

void dma_dcache_short_command(const void* ptr) {
#ifdef ISS_STANDALONE
    while (core_->io_read(IDMA_COMMAND_FSM_ADDR) != 0) {
        core_->runUntilRiscReadyForCmd();  // "Risc" takes 2 cycles until this check is parsed
    }                                      // wait to finish current block loop
#endif

    core_->io_write(IDMA_COMMAND_DCACHE_ADDR + 1, uint32_t(intptr_t(ptr) >> 32));
    core_->io_write(IDMA_COMMAND_DCACHE_ADDR, uint32_t(intptr_t(ptr)));
}

void dma_block_size(const uint32_t& size) {
    core_->runUntilRiscReadyForCmd();  // "Risc" takes 2 cycles until this check is parsed
    while (core_->io_read(IDMA_COMMAND_FSM_ADDR) != 0) {
        core_->runUntilRiscReadyForCmd();  // "Risc" takes 2 cycles until this check is parsed
    }                                      // wait to finish current block loop

    core_->io_write(IDMA_COMMAND_BLOCK_SIZE_ADDR, size);
}

void dma_block_addr_trigger(const void* ptr) {
    core_->runUntilRiscReadyForCmd();  // "Risc" takes 2 cycles until this check is parsed
    while (core_->io_read(IDMA_COMMAND_FSM_ADDR) != 0) {
        core_->runUntilRiscReadyForCmd();  // "Risc" takes 2 cycles until this check is parsed
    }                                      // wait to finish current block loop

    core_->io_write(IDMA_COMMAND_BLOCK_ADDR_TRIGGER_ADDR + 1, uint32_t(intptr_t(ptr) >> 32));
    core_->io_write(IDMA_COMMAND_BLOCK_ADDR_TRIGGER_ADDR, uint32_t(intptr_t(ptr)));
}

void rv_dcache_flush() {
    printf_error("rv_dcache_flush not implemented!\n");
}

void rv_dcache_clear() {
    printf_error("rv_dcache_clear not implemented!\n");
}

void rv_icache_flush() {
    printf_error("rv_icache_flush not implemented!\n");
}

void rv_icache_clear() {
    printf_error("rv_icache_clear not implemented!\n");
}

void dma_e2l_1d(uint32_t cluster_mask,
    uint32_t unit_mask,
    intptr_t ext_base,
    uint32_t loc_base,
    uint32_t x_size) {
    dma_e2l_2d(cluster_mask, unit_mask, ext_base, loc_base, x_size, 1, 1);
}

void dma_e2l_2d(uint32_t cluster_mask,
    uint32_t unit_mask,
    intptr_t ext_base,
    uint32_t loc_base,
    uint32_t x_size,
    uint32_t y_size,
    uint32_t x_stride,
    const bool pad_flags[4]) {
    uint32_t pad = 0;
    pad |= (pad_flags[0] << 0);
    pad |= (pad_flags[1] << 1);
    pad |= (pad_flags[2] << 2);
    pad |= (pad_flags[3] << 3);
    core_->io_write(IDMA_CLUSTER_MASK_ADDR, cluster_mask);
    core_->io_write(IDMA_UNIT_MASK_ADDR, unit_mask);
    core_->io_write(IDMA_LOC_ADDR_ADDR, loc_base);
    core_->io_write(IDMA_X_BLOCK_SIZE_ADDR, x_size);

    // only for 2d
    if (y_size != 1) {
        core_->io_write(IDMA_Y_BLOCK_SIZE_ADDR, y_size);
        core_->io_write(IDMA_X_STRIDE_ADDR, x_stride);
        core_->io_write(IDMA_PAD_ACTIVATION_ADDR, pad);
        // TODO: if y size == 1, no padding possible!?!
    }

    if (ext_base <= VPRO_CFG::MM_SIZE - 1) {
        core_->io_write(IDMA_EXT_BASE_ADDR_E2L_ADDR, uint32_t(ext_base));
    } else {
        core_->io_write(IDMA_EXT_BASE_ADDR_E2L_ADDR + 1, uint32_t(ext_base >> 32));
        core_->io_write(IDMA_EXT_BASE_ADDR_E2L_ADDR, uint32_t(ext_base));
    }
}

void dma_l2e_1d(uint32_t cluster_mask,
    uint32_t unit_mask,
    intptr_t ext_base,
    uint32_t loc_base,
    uint32_t x_size) {
    dma_l2e_2d(cluster_mask, unit_mask, ext_base, loc_base, x_size, 1, 1);
}

void dma_l2e_2d(uint32_t cluster_mask,
    uint32_t unit_mask,
    intptr_t ext_base,
    uint32_t loc_base,
    uint32_t x_size,
    uint32_t y_size,
    uint32_t x_stride) {
    core_->io_write(IDMA_CLUSTER_MASK_ADDR, cluster_mask);
    core_->io_write(IDMA_UNIT_MASK_ADDR, unit_mask);
    core_->io_write(IDMA_LOC_ADDR_ADDR, loc_base);
    core_->io_write(IDMA_X_BLOCK_SIZE_ADDR, x_size);

    // only for 2d
    if (y_size != 1) {
        core_->io_write(IDMA_Y_BLOCK_SIZE_ADDR, y_size);
        core_->io_write(IDMA_X_STRIDE_ADDR, x_stride);
    }

    if (ext_base <= VPRO_CFG::MM_SIZE - 1) {
        core_->io_write(IDMA_EXT_BASE_ADDR_L2E_ADDR, uint32_t(ext_base));
    } else {
        core_->io_write(IDMA_EXT_BASE_ADDR_L2E_ADDR + 1, uint32_t(ext_base >> 32));
        core_->io_write(IDMA_EXT_BASE_ADDR_L2E_ADDR, uint32_t(ext_base));
    }
}

void dma_reset_access_counters() {
    core_->io_write(IDMA_READ_HIT_CYCLES_ADDR, 1);
}

void dma_print_access_counters() {
    printf("DMA_ACCESS_COUNTERs in DMA Domain %s(%04.2lf MHz)%s\n",
        MAGENTA,
        1000 / core_->getDMAClockPeriod(),
        RESET_COLOR);

    for (int cluster = 0; cluster < VPRO_CFG::CLUSTERS; cluster++) {
        uint32_t read_hit_cycles = core_->io_read(IDMA_READ_HIT_CYCLES_ADDR);
        uint32_t read_miss_cycles = core_->io_read(IDMA_READ_MISS_CYCLES_ADDR);
        uint32_t write_hit_cycles = core_->io_read(IDMA_WRITE_HIT_CYCLES_ADDR);
        uint32_t write_miss_cycles = core_->io_read(IDMA_WRITE_MISS_CYCLES_ADDR);

        printf("  DMA %i, [Cycles] Read Hit %i Miss %i, Write Hit %i Miss %i\n",
            cluster,
            read_hit_cycles,
            read_miss_cycles,
            write_hit_cycles,
            write_miss_cycles);
    }
    printf("\n");
}

// *** VPRO DCMA *** //

void dcma_flush() {
    while (core_->io_read(DCMA_WAIT_BUSY_ADDR)) {}
    core_->io_write(DCMA_FLUSH_ADDR, 0);
    while (core_->io_read(DCMA_WAIT_BUSY_ADDR)) {}
}

void dcma_reset() {
    while (core_->io_read(DCMA_WAIT_BUSY_ADDR)) {}
    core_->io_write(DCMA_RESET_ADDR, 0);
};

void vpro_wait_busy(uint32_t cluster_mask, uint32_t unit_mask, int cycle) {
    printf("cycle:%i vpro_wait_busy cluster_mask:%u unit_mask: unit_mask:%u \n",
        cycle,
        cluster_mask,
        unit_mask);
    vpro_wait_busy(cluster_mask, unit_mask);
}

void vpro_wait_busy(uint32_t cluster_mask, uint32_t unit_mask) {
    core_->setWaitingToFinish(true);
    core_->io_write(VPRO_BUSY_MASK_CL_ADDR, cluster_mask);
    while ((core_->io_read(VPRO_BUSY_MASKED_VPRO_ADDR) & unit_mask) != 0) {}
    core_->setWaitingToFinish(false);
    if (CREATE_CMD_HISTORY_FILE) {
        QString cmd_description = "VPRO Sync";
        *CMD_HISTORY_FILE_STREAM << cmd_description << "\n";
    }
    if (CREATE_CMD_PRE_GEN) {
        sync_function pre_gen_sync;
        pre_gen_sync.data[0] = 512;
        *PRE_GEN_FILE_STREAM << pre_gen_sync.data[0];
    }
}

void vpro_lane_sync() {
    vpro_wait_busy(0xffffffff, 0xffffffff);
}

void vpro_set_cluster_mask(uint32_t idmask) {
    core_->io_write(VPRO_CLUSTER_MASK_ADDR, idmask);
}

void vpro_set_unit_mask(uint32_t idmask) {
    core_->io_write(VPRO_UNIT_MASK_ADDR, idmask);
}

void vpro_set_idmask(uint32_t idmask) {
    vpro_set_cluster_mask(idmask);
}

uint32_t vpro_get_cluster_mask() {
    return core_->io_read(VPRO_CLUSTER_MASK_ADDR);
}

uint32_t vpro_get_unit_mask() {
    return core_->io_read(VPRO_UNIT_MASK_ADDR);
}

void vpro_mac_h_bit_shift(uint32_t shift) {
    core_->io_write(VPRO_MAC_SHIFT_REG_ADDR, shift);
}

uint32_t vpro_mac_h_bit_shift() {
    return core_->io_read(VPRO_MAC_SHIFT_REG_ADDR);
}

void vpro_mul_h_bit_shift(uint32_t shift) {
    core_->io_write(VPRO_MUL_SHIFT_REG_ADDR, shift);
}

uint32_t vpro_mul_h_bit_shift() {
    return core_->io_read(VPRO_MUL_SHIFT_REG_ADDR);
}

void vpro_set_mac_init_source(VPRO::MAC_INIT_SOURCE source) {
    core_->io_write(VPRO_MAC_INIT_SOURCE_ADDR, source);
}

VPRO::MAC_INIT_SOURCE vpro_get_mac_init_source() {
    return static_cast<VPRO::MAC_INIT_SOURCE>(core_->io_read(VPRO_MAC_INIT_SOURCE_ADDR));
}

void vpro_set_mac_reset_mode(VPRO::MAC_RESET_MODE mode) {
    core_->io_write(VPRO_MAC_RESET_MODE_ADDR, mode);
}

VPRO::MAC_RESET_MODE vpro_get_mac_reset_mode() {
    return static_cast<VPRO::MAC_RESET_MODE>(core_->io_read(VPRO_MAC_RESET_MODE_ADDR));
}

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
    uint32_t z_end) {
    assert(id <= 0xf);
    assert(x_end <= 0x3f);
    assert(y_end <= 0x3f);
    assert(z_end <= 0x3ff);

    // src1, src2, dst without gamma
    core_->io_write(
        VPRO_CMD_REGISTER_4_0_ADDR, ((src1 & 0xffffffc0) << (7 - 6)) | (x_end << 1) | is_chain);
    core_->io_write(
        VPRO_CMD_REGISTER_4_1_ADDR, ((src2 & 0xffffffc0) << (7 - 6)) | (y_end << 1) | blocking);
    core_->io_write(VPRO_CMD_REGISTER_4_2_ADDR,
        ((dst & 0xffffffc0) << (10 - 6)) | (func << 4) | (id << 1) | flag_update);

    uint32_t dst_sel = (dst >> ISA_COMPLEX_LENGTH_3D) & 0x7;
    // remaining 3d parameters (gamma, zend)
    core_->io_write(VPRO_CMD_REGISTER_4_3_ADDR,
        ((dst & 0x3f) << 26) | ((src1 & 0x3f) << 20) | ((src2 & 0x3f) << 14) | (dst_sel << 10) |
            z_end);
}

void __vpro(uint32_t id,
    uint32_t blocking,
    uint32_t is_chain,
    uint32_t func,
    uint32_t flag_update,
    uint32_t dst,
    uint32_t src1,
    uint32_t src2,
    uint32_t x_end,
    uint32_t y_end) {
    assert(id <= 0xf);
    assert(x_end <= 0x3f);
    assert(y_end <= 0x3f);

    core_->io_write(VPRO_CMD_REGISTER_3_0_ADDR, (src1 << 7) | (x_end << 1) | is_chain);
    core_->io_write(VPRO_CMD_REGISTER_3_1_ADDR, (src2 << 7) | (y_end << 1) | blocking);
    core_->io_write(
        VPRO_CMD_REGISTER_3_2_ADDR, (dst << 10) | (func << 4) | (id << 1) | flag_update);
}

void sim_wait_step(bool finish, const char* msg) {
    core_->sim_wait_step(finish, msg);
}

// *** General simulator environment control ***
int sim_init(int (*main_fkt)(int, char**), int& argc, char* argv[]) {
    return core_->sim_init(main_fkt,
        argc,
        argv);
}

void sim_stop(bool silent) {
    core_->sim_stop(silent);
}

/**
 * @brief Checks the Sim for mininmal Requirments
 * @param cluster_cnt Count of minimal Clusters
 * @param unit_cnt Count of minimal Vector Units
 * @param lane_cnt Count of minimal Lanes
 *
 * Returns True if requirments were fullfilled or false if not met and executes sim_stop()
 */
bool sim_min_req(int cluster_cnt, int unit_cnt, int lane_cnt) {
#ifndef ISS_STANDALONE
    printf("sim_min_req only tested with ISS_STANDALONE Mode\n");
#else
    uint32_t clusters_available = get_gpr_clusters();
    uint32_t units_available = get_gpr_units();
    uint32_t lanes_available = get_gpr_laness();
    bool enaugh = true;
    if (cluster_cnt > clusters_available) {
        enaugh = false;
        printf_error("[Min Req Check Failed] App needs %i Clusters. HW has %i Clusters only!\n",
            cluster_cnt,
            clusters_available);
    }
    if (unit_cnt > units_available) {
        enaugh = false;
        printf_error("[Min Req Check Failed] App needs %i Units. HW has %i Units only!\n",
            unit_cnt,
            units_available);
    }
    if (lane_cnt > lanes_available) {
        enaugh = false;
        printf_error("[Min Req Check Failed] App needs %i Lanes. HW has %i Lanes only!\n",
            lane_cnt,
            lanes_available);
    }
    if (enaugh) {
        printf("[Min Req Check Succeeds] %i Clusters, %i Units, %i Lanes\n",
            cluster_cnt,
            unit_cnt,
            lane_cnt);
    } else {
        fflush(stdout);
        exit(1);
    }
    return enaugh;
#endif
    return false;
}

// *** Simulator debugging funtions ***
void sim_dump_local_memory(uint32_t cluster, uint32_t unit) {
    core_->sim_dump_local_memory(cluster, unit);
}

void sim_dump_register_file(uint32_t cluster, uint32_t unit, uint32_t lane) {
    core_->sim_dump_register_file(cluster, unit, lane);
}

void sim_dump_queue(uint32_t cluster, uint32_t unit) {
    core_->sim_dump_queue(cluster, unit);
}

void sim_stat_reset() {
    aux_reset_all_stats();
    core_->sim_stats_reset();
}

void sim_printf(const char* format) {
    printf("#SIM_PRINTF: ");
    printf("%s", format);
}

uint32_t get_gpr_vpro_freq() {
    return core_->io_read(GP_REGISTERS_ADDR + 6 * 4);
}
uint32_t get_gpr_risc_freq() {
    return core_->io_read(GP_REGISTERS_ADDR + 7 * 4);
}
uint32_t get_gpr_dma_freq() {
    return core_->io_read(GP_REGISTERS_ADDR + 8 * 4);
}
uint32_t get_gpr_axi_freq() {
    return core_->io_read(GP_REGISTERS_ADDR + 9 * 4);
}
uint32_t get_gpr_git_sha() {
    return core_->io_read(GP_REGISTERS_ADDR + 14 * 4);
}
uint32_t get_gpr_hw_build_date() {
    return core_->io_read(GP_REGISTERS_ADDR + 2 * 4);
}
uint32_t get_gpr_clusters() {
    return core_->io_read(GP_REGISTERS_ADDR + 3 * 4);
}
uint32_t get_gpr_units() {
    return core_->io_read(GP_REGISTERS_ADDR + 4 * 4);
}
uint32_t get_gpr_laness() {
    return core_->io_read(GP_REGISTERS_ADDR + 5 * 4);
}
uint32_t get_gpr_DCMA_off() {
    return core_->io_read(GP_REGISTERS_ADDR + 16 * 4);
}
uint32_t get_gpr_DCMA_nr_rams() {
    return core_->io_read(GP_REGISTERS_ADDR + 17 * 4);
}
uint32_t get_gpr_DCMA_line_size() {
    return core_->io_read(GP_REGISTERS_ADDR + 18 * 4);
}
uint32_t get_gpr_DCMA_associativity() {
    return core_->io_read(GP_REGISTERS_ADDR + 19 * 4);
}
