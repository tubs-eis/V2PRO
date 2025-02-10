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
 * @file ISS.cpp
 * @author Alexander Köhne, Sven Gesper
 * @date Created by Gesper, 2019
 *
 * VPRO instruction & system simulation library
 * Main entry for simulation.
 * Here are the Core related functions
 * -> call commands for vpro
 * -> stop
 * here are memory/DMA related functions
 */

#include <math.h>
#include <bitset>
#include <cstring>
#include <iostream>
#include <vector>

#include <riscv/eisv_defs.h>
#include <vpro/dma_cmd_struct.h>
#include <vpro/vpro_defs.h>
#include "../model/architecture/DMALooper.h"
#include "../model/architecture/stats/Statistics.h"
#include "ISS.h"
#include "helper/debugHelper.h"

struct Dmacmd {
    uint32_t data[11];
};

/*
dma_e2l_2d(cluster_mask, unit_mask, ext_base, loc_base, x_size, y_size, x_stride, pad_flags);
*/
Dmacmd gen_dma_struct(const std::shared_ptr<CommandDMA>& command) {
    Dmacmd cmd;
    uint64_t unit_mask = 0lu;
    for (auto u : command->unit)
        unit_mask |= 1u << u;
    uint32_t pad = 0;
    pad |= (command->pad[0] << 0);
    pad |= (command->pad[1] << 1);
    pad |= (command->pad[2] << 2);
    pad |= (command->pad[3] << 3);
    cmd.data[0] = 515;

    cmd.data[1] = 1 >> command->cluster_mask;  // obtain cluster, used for outdated wrapper
    cmd.data[2] = unit_mask;

    cmd.data[3] = static_cast<uint32_t>(command->ext_base >> 32);
    cmd.data[4] = static_cast<uint32_t>(command->ext_base & 0xFFFFFFFF);
    cmd.data[5] = command->loc_base;
    cmd.data[6] = command->x_size;
    cmd.data[7] = command->y_size;
    cmd.data[8] = command->y_leap;
    cmd.data[9] = pad;

    cmd.data[10] = command->get_direction();  // e2l=1, l2e=0, other=-1
    return cmd;
}

struct Vprocmd {
    uint32_t data[12];
};

Vprocmd gen_vpro_struct(const std::shared_ptr<CommandVPRO>& command) {
    Vprocmd cmd;
    cmd.data[0] = 514;

    cmd.data[1] = command->id_mask;
    cmd.data[2] = command->blocking;
    cmd.data[3] = command->is_chain;
    cmd.data[4] = command->get_func_type();
    cmd.data[5] = command->flag_update;

    cmd.data[6] = command->dst.create_IMM();
    cmd.data[7] = command->src1.create_IMM();
    cmd.data[8] = command->src2.create_IMM(command->get_src2_off());

    cmd.data[9] = command->x_end;
    cmd.data[10] = command->y_end;
    cmd.data[11] = command->z_end;

    printf("(ISS)     VPRO: %u %u %u %u %u %u %u %u %u %u %u\n",
        cmd.data[1],
        cmd.data[2],
        cmd.data[3],
        cmd.data[4],
        cmd.data[5],
        cmd.data[6],
        cmd.data[7],
        cmd.data[8],
        cmd.data[9],
        cmd.data[10],
        cmd.data[11]);
    return cmd;
}

void ISS::run_vpro_instruction(const std::shared_ptr<CommandVPRO>& command) {
#ifdef ISS_STANDALONE
    runUntilReadyForCmd();
#endif
    check_vpro_instruction_length(command);
    for (auto cluster : clusters) {
        if (((1u << cluster->cluster_id) & architecture_state->cluster_mask_global) > 0) {
            if (!cluster->sendCMD(std::dynamic_pointer_cast<CommandBase>(command))) {
                printf_warning("Cluster did not receive this command... Queue full?");
                command->print();
            }
        }
    }

    if (CREATE_CMD_PRE_GEN) {
        Vprocmd instr_data = gen_vpro_struct(command);
        for (uint32_t num : instr_data.data) {
            *PRE_GEN_FILE_STREAM << num;
        }
    }

    if (CREATE_CMD_HISTORY_FILE) {
        QString cmd_description =
            "VPRO " + command->get_type().leftJustified(17, ' ') + ", id " +
            QString::number(command->id_mask, 2).leftJustified(3, ' ') + ", bl " +
            QString::number(command->fu_sel) + ", " + QString::number(command->func) + ", " +
            QString::number(command->blocking) + ", ch " + QString::number(command->is_chain) +
            ", flag " + QString::number(command->flag_update) + ", SRC1 " +
            command->src1.__toString() + ", SRC2 " + command->src2.__toString() + ", DST " +
            command->dst.__toString() + ", xE " +
            QString::number(command->x_end).leftJustified(3, ' ') + ", yE " +
            QString::number(command->y_end).leftJustified(3, ' ') + ", zE " +
            QString::number(command->z_end).leftJustified(3, ' ');

        *CMD_HISTORY_FILE_STREAM << cmd_description << "\n";
    }
}

//## Integration
void ISS::run_dma_instruction(const std::shared_ptr<CommandDMA>& command, bool skip_tick) {
#ifdef ISS_STANDALONE
    if (!skip_tick) runUntilReadyForCmd();
#endif
    for (auto cluster : clusters) {
        if (((command->cluster_mask >> cluster->cluster_id) & 0b1) == 1) {
            // create a copy for each cluster (dma)
            auto cmd = std::make_shared<CommandDMA>(command.get());
            cluster->sendCMD(std::dynamic_pointer_cast<CommandBase>(cmd));
        }
    }

    if (CREATE_CMD_PRE_GEN) {
        Dmacmd instr_data = gen_dma_struct(command);
        for (uint32_t num : instr_data.data) {
            *PRE_GEN_FILE_STREAM << num;
        }
    }

    if (CREATE_CMD_HISTORY_FILE) {
        uint64_t unit_mask = 0lu;
        for (auto u : command->unit)
            unit_mask |= 1u << u;

        QString cmd_description =
            "DMA  " + command->get_type().leftJustified(17, ' ') + ", cluster_mask " +
            QString::number(command->cluster_mask).leftJustified(2, ' ') + ", unit_mask " +
            QString::number(unit_mask) + ", Ext " +
            QString::number(command->ext_base).leftJustified(10, ' ') + ", Loc " +
            QString::number(command->loc_base).leftJustified(4, ' ') + ", pad " +
            QString::number(command->pad[0]) + ", " + QString::number(command->pad[1]) + ", " +
            QString::number(command->pad[2]) + ", " + QString::number(command->pad[3]) + ", xS " +
            QString::number(command->y_leap).leftJustified(3, ' ') + ", xE " +
            QString::number(command->x_size).leftJustified(3, ' ') + ", yE " +
            QString::number(command->y_size).leftJustified(3, ' ');

        *CMD_HISTORY_FILE_STREAM << cmd_description << "\n";
    }
}

void ISS::gen_direct_dma_instruction(const uint8_t dcache_data_struct[32]) {
    dmalooper->new_dcache_input(dcache_data_struct);
}

uint32_t ISS::io_read(uint32_t addr) {
#ifdef ISS_STANDALONE
    runUntilRiscReadyForCmd();
#endif
    uint32_t value = 0;

    // handle vpro requests (16-bit addr passed to vpro top)
    auto io_cl_addr = addr & 0xffff;

    // handle special registers (using thos bits to differntiate)
    auto io_cl_register_masked = io_cl_addr & 0b0000000011111100;

    auto dcma_stat = Statistics::get().getDCMAStat();

    switch (addr) {
        /**
         * EISV Registers
         */
        case DEBUG_FIFO_ADDR:  // debug fifo read = 0
            break;

        case CNT_VPRO_LANE_ACT_ADDR:
            value = aux_cnt_lane_act;
            break;
        case CNT_VPRO_DMA_ACT_ADDR:
            value = aux_cnt_dma_act;
            break;
        case CNT_VPRO_BOTH_ACT_ADDR:
            value = aux_cnt_both_act;
            break;
        case CNT_VPRO_TOTAL_ADDR:
            value = aux_cnt_vpro_total;
            break;
        case CNT_RISC_TOTAL_ADDR:
            value = aux_cnt_riscv_total;
            break;
        case CNT_RISC_ENABLED_ADDR:
            value = aux_cnt_riscv_enabled;
            break;
        case SYS_TIME_CNT_LO_ADDR:
            value = aux_cycle_counter;
            break;
        case SYS_TIME_CNT_HI_ADDR:
            value = aux_cycle_counter >> 32;
            break;
        case DEV_NULL_ADDR:  // dev null read 0
            break;
            /**
             * VPRO Registers
             */
        case VPRO_CLUSTER_MASK_ADDR:
            value = architecture_state->cluster_mask_global;
            break;
        case VPRO_BUSY_MASK_CL_ADDR:
            value = architecture_state->sync_cluster_mask_global;
            break;
        case VPRO_BUSY_MASKED_DMA_ADDR:
            // masked cluster any dma busy?
            value = (dmalooper->isBusy() | dmablock->isBusy()) << 31;
            for (auto cluster : clusters) {
                if (((architecture_state->sync_cluster_mask_global >> cluster->cluster_id) & 0x1) !=
                    0) {  // masked cluster
                    if (cluster->dma->isBusy()) {
                        value |= 1 << cluster->cluster_id;
                    }
                }
            }
            break;
        case DCMA_WAIT_BUSY_ADDR:
            value = dcma->isBusy();
            break;
        case IDMA_READ_HIT_CYCLES_ADDR:
            value =
                dcma_stat->dma_access_counter.read_hit_cycles[dma_access_counter_cluster_pointer];
            break;
        case IDMA_READ_MISS_CYCLES_ADDR:
            value =
                dcma_stat->dma_access_counter.read_miss_cycles[dma_access_counter_cluster_pointer];
            break;
        case IDMA_WRITE_HIT_CYCLES_ADDR:
            value =
                dcma_stat->dma_access_counter.write_hit_cycles[dma_access_counter_cluster_pointer];
            break;
        case IDMA_WRITE_MISS_CYCLES_ADDR:
            value =
                dcma_stat->dma_access_counter.write_miss_cycles[dma_access_counter_cluster_pointer];
            dma_access_counter_cluster_pointer++;
            if (dma_access_counter_cluster_pointer == VPRO_CFG::CLUSTERS)
                dma_access_counter_cluster_pointer = 0;
            break;
        case VPRO_BUSY_MASKED_VPRO_ADDR:
            // masked cluster any busy?
            value = 0;
            for (auto cluster : clusters) {
                if (((architecture_state->sync_cluster_mask_global >> cluster->cluster_id) & 0x1) !=
                    0) {  // masked cluster
                    for (auto unit : cluster->getUnits()) {
                        if (unit->isBusy()) {
                            value |= 1 << unit->id();
                        }
                    }
                }
            }
            break;
        case VPRO_LANE_SYNC_ADDR:
            value = 0;
            for (auto cl : clusters) {
                for (auto u : cl->getUnits()) {
                    value |= (u->isBusy() << cl->cluster_id);
                }
            }
            break;
        case VPRO_DMA_SYNC_ADDR:
            value = (dmalooper->isBusy() | dmablock->isBusy()) << 31;
            for (auto cl : clusters) {
                value |= cl->dma->isBusy() << cl->cluster_id;
                ;
            }
            break;
        case VPRO_SYNC_ADDR:
            value = (dmalooper->isBusy() | dmablock->isBusy()) << 31;
            for (auto cl : clusters) {
                value |= cl->dma->isBusy() << cl->cluster_id;
                ;
                for (auto u : cl->getUnits()) {
                    value |= (u->isBusy() << cl->cluster_id);
                }
            }
            break;
        case (IDMA_COMMAND_FSM_ADDR):
#ifdef ISS_STANDALONE
            value = dmalooper->isBusy() | dmablock->isBusy();
#else
            printf_error(
                "[IO Write] ERROR: IDMA_COMMAND_FSM_ADDR should be handled by SystemC FSM to "
                "access the simulated FSM in the DCache!\n");
#endif
            break;
        case VPRO_UNIT_MASK_ADDR:
            value = architecture_state->unit_mask_global;
            break;
        case VPRO_MUL_SHIFT_REG_ADDR:
            value = architecture_state->ACCU_MUL_HIGH_BIT_SHIFT;
            break;
        case VPRO_MAC_SHIFT_REG_ADDR:
            value = architecture_state->ACCU_MAC_HIGH_BIT_SHIFT;
            break;
        case VPRO_MAC_INIT_SOURCE_ADDR:
            value = architecture_state->MAC_ACCU_INIT_SOURCE;
            break;
        case VPRO_MAC_RESET_MODE_ADDR:
            value = architecture_state->MAC_ACCU_RESET_MODE;
            break;
        case GP_REGISTERS_ADDR + 2 * 4: {
            std::tm t = {};
            std::stringstream ss;
            ss << __DATE__ << " " << __TIME__;
            ss >> std::get_time(&t, "%b %d %Y %H:%M:%S");
            std::time_t l = std::mktime(&t);
            value = l;
            break;
        }
        case GP_REGISTERS_ADDR + 3 * 4: {
            value = VPRO_CFG::CLUSTERS;
            break;
        }
        case GP_REGISTERS_ADDR + 4 * 4: {
            value = VPRO_CFG::UNITS;
            break;
        }
        case GP_REGISTERS_ADDR + 5 * 4: {
            value = VPRO_CFG::LANES;
            break;
        }
        case GP_REGISTERS_ADDR + 6 * 4: {
            value = uint32_t(1000000000. / vpro_clock_period);
            break;
        }
        case GP_REGISTERS_ADDR + 7 * 4: {
            value = uint32_t(1000000000. / risc_clock_period);
            break;
        }
        case GP_REGISTERS_ADDR + 8 * 4: {
            value = uint32_t(1000000000. / dma_clock_period);
            break;
        }
        case GP_REGISTERS_ADDR + 9 * 4: {
            value = uint32_t(1000000000. / axi_clock_period);
            break;
        }
        case GP_REGISTERS_ADDR + 14 * 4: {
            value = 0xcafecafe;
            break;
        }
        case GP_REGISTERS_ADDR + 16 * 4: {
            value = 0;
            if (dcma->getDCMAOff()) value = 1;
            break;
        }
        case GP_REGISTERS_ADDR + 17 * 4: {
            value = dcma->getNrRams();
            break;
        }
        case GP_REGISTERS_ADDR + 18 * 4: {
            value = dcma->getLineSize();
            break;
        }
        case GP_REGISTERS_ADDR + 19 * 4: {
            value = std::log2(dcma->getAssociativity());
            break;
        }

        default: {
            if ((addr & 0xFFFFFF00) == GP_REGISTERS_ADDR) {  // 00 - ff (byte addr)
                // remaining GP Register
                value = general_purpose_register[(addr & 0xff) / 4];
            } else if ((addr & 0xffff00ff) == VPRO_BUSY_BASE_ADDR) {
                // addressed unit busy?
                value = 0;
                for (auto cluster : clusters) {
                    if (cluster->cluster_id == ((addr & 0xFF00) >> 8)) {
                        for (auto unit : cluster->getUnits()) {
                            if (unit->isBusy()) {
                                value |= 1 << unit->id();
                            }
                        }
                    }
                }
            } else if ((addr & 0xffff0000) == 0xfffe0000) {  //VPRO io access not yet handled
                // handle vpro requests (16-bit addr passed to vpro top)
                // handle special registers (using thos bits to differntiate)
                if ((io_cl_register_masked & 0b0000000010000000) != 0) {
                    // => dma_access
                    for (auto c : clusters) {
                        if (c->cluster_id == ((io_cl_addr & 0xff00) >> 8)) {
                            value = c->dma->io_read(addr);
                        }
                    }
                } else {
                    printf_warning(
                        "IO Read from ISS (inside VPRO Addr range) but not from existing register! "
                        "Addr: 0x%08x\n",
                        addr);
                }
            } else {
                printf_warning(
                    "IO Read from ISS but not from existing register! Addr: 0x%08x\n", addr);
            }
        }  // vpro io access
    }      // io access

    return value;
}

void ISS::io_write(uint32_t addr, uint32_t value) {
#ifdef ISS_STANDALONE
    if ((addr & 0b1) == 0)  // skip special addresses (sim on host -> ext address 64-bit split)
        runUntilRiscReadyForCmd();
#endif

    auto dcma_stat = Statistics::get().getDCMAStat();

    // handle vpro requests (16-bit addr passed to vpro top)
    auto io_cl_addr = addr & 0xffff;

    // handle special registers (using those bits to differentiate)
    auto io_cl_register_masked = io_cl_addr & 0b0000000011111100;

    switch (addr) {
        case DEBUG_FIFO_ADDR:
            printf("#DEBUG_FIFO: 0x%08x\n", value);
            break;

        case CNT_VPRO_LANE_ACT_ADDR:
            aux_cnt_lane_act = 0;
            break;
        case CNT_VPRO_DMA_ACT_ADDR:
            aux_cnt_dma_act = 0;
            break;
        case CNT_VPRO_BOTH_ACT_ADDR:
            aux_cnt_both_act = 0;
            break;
        case CNT_VPRO_TOTAL_ADDR:
            aux_cnt_vpro_total = 0;
            break;
        case CNT_RISC_TOTAL_ADDR:
            aux_cnt_riscv_total = 0;
            break;
        case CNT_RISC_ENABLED_ADDR:
            aux_cnt_riscv_enabled = 0;
            break;
        case SYS_TIME_CNT_LO_ADDR:
        case SYS_TIME_CNT_HI_ADDR:
            aux_cycle_counter = 0;
            break;

        // DCMA
        case (DCMA_FLUSH_ADDR): {
            dcma->flush();
            break;
        }
        case (DCMA_RESET_ADDR): {
            dcma->reset();
            break;
        }
        case IDMA_READ_HIT_CYCLES_ADDR:
            dcma_stat->reset();
            dma_access_counter_cluster_pointer = 0;
            break;
        case IDMA_READ_MISS_CYCLES_ADDR:
            dcma_stat->reset();
            dma_access_counter_cluster_pointer = 0;
            break;
        case IDMA_WRITE_HIT_CYCLES_ADDR:
            dcma_stat->reset();
            dma_access_counter_cluster_pointer = 0;
            break;
        case IDMA_WRITE_MISS_CYCLES_ADDR:
            dcma_stat->reset();
            dma_access_counter_cluster_pointer = 0;
            break;
            /**
             * 2D VPRO Registers
             */
        case (VPRO_CMD_REGISTER_3_0_ADDR): {  // = VPRO_CMD_REGISTER_4_0_ADDR
            io_vpro_cmd_register.src1_IMM = value;

            io_vpro_cmd_register.src1_sel = value >> 29;
            io_vpro_cmd_register.src1_offset = (value >> 19) & 0x3FF;
            io_vpro_cmd_register.src1_alpha = (value >> 13) & 0x3F;
            io_vpro_cmd_register.src1_beta = (value >> 7) & 0x3F;

            io_vpro_cmd_register.x_end = (value >> 1) & 0x3F;
            io_vpro_cmd_register.is_chain = (value & 0x1);
            break;
        }
        case (VPRO_CMD_REGISTER_3_1_ADDR): {  // = VPRO_CMD_REGISTER_4_1_ADDR
            io_vpro_cmd_register.src2_IMM = value;

            io_vpro_cmd_register.src2_sel = value >> 29;
            io_vpro_cmd_register.src2_offset = (value >> 19) & 0x3FF;
            io_vpro_cmd_register.src2_alpha = (value >> 13) & 0x3F;
            io_vpro_cmd_register.src2_beta = (value >> 7) & 0x3F;

            io_vpro_cmd_register.y_end = (value >> 1) & 0x3F;
            io_vpro_cmd_register.blocking = (value & 0x1);
            break;
        }
        case (VPRO_CMD_REGISTER_3_2_ADDR): {  // Trigger, 2D
            io_vpro_cmd_register.dst_offset = (value >> 22) & 0x3FF;
            io_vpro_cmd_register.dst_alpha = (value >> 16) & 0x3F;
            io_vpro_cmd_register.dst_beta = (value >> 10) & 0x3F;
            io_vpro_cmd_register.func = (value >> 4) & 0x3F;
            io_vpro_cmd_register.id = (value >> 1) & 0x7;
            io_vpro_cmd_register.f_update = (value & 0x1);

            io_vpro_cmd_register.src1_gamma = 0;
            io_vpro_cmd_register.src2_gamma = 0;
            io_vpro_cmd_register.dst_gamma = 0;
            io_vpro_cmd_register.z_end = 0;
            io_vpro_cmd_register.dst_sel = SRC_SEL_LEFT;

            if ((io_vpro_cmd_register.func & 0b111000) ==
                0b001000) {  // is store (encode source chain id in gamma, not beta)
                io_vpro_cmd_register.dst_gamma = io_vpro_cmd_register.dst_beta;
                io_vpro_cmd_register.dst_beta = 0;
            }

            if (io_vpro_cmd_register.src1_sel !=
                SRC_SEL_ADDR) {  // immediate requires usage of gamma to be decoded in VPRO -> all shifted down to 3D mode
                io_vpro_cmd_register.src1_gamma = io_vpro_cmd_register.src1_beta;
                io_vpro_cmd_register.src1_beta = io_vpro_cmd_register.src1_alpha;
                io_vpro_cmd_register.src1_alpha = io_vpro_cmd_register.src1_offset & 0x3F;
                io_vpro_cmd_register.src1_offset = io_vpro_cmd_register.src1_offset >> 6;
                if (io_vpro_cmd_register.src1_offset & 0x8)  // sign extension for 22-bit imm
                    io_vpro_cmd_register.src1_offset |= 0x30;
            }
            if (io_vpro_cmd_register.src2_sel !=
                SRC_SEL_ADDR) {  // immediate requires usage of gamma to be decoded in VPRO -> all shifted down to 3D mode
                io_vpro_cmd_register.src2_gamma = io_vpro_cmd_register.src2_beta;
                io_vpro_cmd_register.src2_beta = io_vpro_cmd_register.src2_alpha;
                io_vpro_cmd_register.src2_alpha = io_vpro_cmd_register.src2_offset & 0x3F;
                io_vpro_cmd_register.src2_offset = io_vpro_cmd_register.src2_offset >> 6;
                if (io_vpro_cmd_register.src2_offset & 0x8)  // sign extension for 22-bit imm
                    io_vpro_cmd_register.src2_offset |= 0x30;
            }
            run_vpro_instruction(gen_io_vpro_command());
            break;
        }
        case (VPRO_CMD_REGISTER_4_2_ADDR): {
            io_vpro_cmd_register.dst_IMM = value;
            io_vpro_cmd_register.dst_offset = (value >> 22) & 0x3FF;
            io_vpro_cmd_register.dst_alpha = (value >> 16) & 0x3F;
            io_vpro_cmd_register.dst_beta = (value >> 10) & 0x3F;
            io_vpro_cmd_register.func = (value >> 4) & 0x3F;
            io_vpro_cmd_register.id = (value >> 1) & 0x7;
            io_vpro_cmd_register.f_update = (value & 0x1);
            break;
        }
        case (VPRO_CMD_REGISTER_4_3_ADDR): {  // Trigger
            io_vpro_cmd_register.dst_gamma = (value >> 26) & 0x3F;
            io_vpro_cmd_register.src1_gamma = (value >> 20) & 0x3F;
            io_vpro_cmd_register.src2_gamma = (value >> 14) & 0x3F;
            io_vpro_cmd_register.dst_sel = (value >> 10) & 0x7;
            io_vpro_cmd_register.z_end = (value >> 0) & 0x3FF;
            io_vpro_cmd_register.dst_sel = (value >> 10) & 0b111;
            run_vpro_instruction(gen_io_vpro_command());
            break;
        }

        /**
         * Deprecated LOOP Registers
         */
        case (0xFFFFFE10): {
            // REPORT NOT IMPLEMENTED
            printf_error(
                "[LOOP START] Command Issue (io_write in ISS) with start value not implemented! "
                "LOOP no longer supported!\n");
            break;
        }
            /**
             * Special Command Issue VPRO Registers
             */
        case (0xFFFFFE20):
        case (0xFFFFFE24):
        case (0xFFFFFE28):
        case (0xFFFFFE2C): {
            // TODO: IMPLEMENT special vpro command registers to modify multiple params
            // special registers: modify multiple paramters (used in yolo lite HW version ...)
            printf_error("[Special VPRO CMD Registers] not implemented in ISS. \n");
            break;
        }
        /**
         * 3D VPRO Registers
         */
        case (VPRO_MUL_SHIFT_REG_ADDR): {
            if ((value & ~0b11111) != 0) {
                printf_error(
                    "[MULH SHIFT REG io-write] only positive 5-bit shift value allowed! given: "
                    "%i\n",
                    value);
            }
            architecture_state->ACCU_MUL_HIGH_BIT_SHIFT = value;
            break;
        }
        case (VPRO_MAC_SHIFT_REG_ADDR): {
            if ((value & ~0b11111) != 0) {
                printf_error(
                    "[MACH SHIFT REG io-write] only positive 5-bit shift value allowed! given: "
                    "%i\n",
                    value);
            }
            architecture_state->ACCU_MAC_HIGH_BIT_SHIFT = value;
            break;
        }
        case VPRO_MAC_INIT_SOURCE_ADDR: {
            architecture_state->MAC_ACCU_INIT_SOURCE = static_cast<VPRO::MAC_INIT_SOURCE>(value);
            break;
        }
        case VPRO_MAC_RESET_MODE_ADDR: {
            architecture_state->MAC_ACCU_RESET_MODE = static_cast<VPRO::MAC_RESET_MODE>(value);
            break;
        }
        case (VPRO_CLUSTER_MASK_ADDR): {
            architecture_state->cluster_mask_global = value;
            break;
        }
        case (VPRO_UNIT_MASK_ADDR): {
            architecture_state->unit_mask_global = value;
            break;
        }
        case (VPRO_BUSY_MASK_CL_ADDR): {
            architecture_state->sync_cluster_mask_global = value;
            break;
        }

        case (IDMA_UNIT_MASK_ADDR):
            io_dma_cmd_register.unit_mask = value;
            break;
        case (IDMA_CLUSTER_MASK_ADDR):
            //            io_dma_cmd_register = io_dma_cmd_register_t();
            io_dma_cmd_register.x_stride = 1;
            io_dma_cmd_register.y_size = 1;
            io_dma_cmd_register.pad_flags[0] = false;
            io_dma_cmd_register.pad_flags[1] = false;
            io_dma_cmd_register.pad_flags[2] = false;
            io_dma_cmd_register.pad_flags[3] = false;

            io_dma_cmd_register.cluster_mask = value;
            break;
        case (IDMA_LOC_ADDR_ADDR):
            io_dma_cmd_register.loc_addr = value;
            break;
        case (IDMA_X_BLOCK_SIZE_ADDR):
            io_dma_cmd_register.x_size = value;
            break;
        case (IDMA_Y_BLOCK_SIZE_ADDR):
            io_dma_cmd_register.y_size = value;
            break;
        case (IDMA_X_STRIDE_ADDR):
            io_dma_cmd_register.x_stride = int32_t(value);
            break;
        case (IDMA_PAD_ACTIVATION_ADDR):
            io_dma_cmd_register.pad_flags[CommandDMA::PAD::TOP] =
                value & (1 << CommandDMA::PAD::TOP);
            io_dma_cmd_register.pad_flags[CommandDMA::PAD::RIGHT] =
                value & (1 << CommandDMA::PAD::RIGHT);
            io_dma_cmd_register.pad_flags[CommandDMA::PAD::BOTTOM] =
                value & (1 << CommandDMA::PAD::BOTTOM);
            io_dma_cmd_register.pad_flags[CommandDMA::PAD::LEFT] =
                value & (1 << CommandDMA::PAD::LEFT);
            break;
        case (IDMA_EXT_BASE_ADDR_E2L_ADDR +
              1):  // store upper 32-bit for host mm access upon trigger lateron
        case (IDMA_EXT_BASE_ADDR_L2E_ADDR +
              1):  // store upper 32-bit for host mm access upon trigger lateron
            io_dma_cmd_register.ext_addr = value;
            break;

        case (IDMA_COMMAND_BLOCK_SIZE_ADDR):
#ifdef ISS_STANDALONE
            dmablock->newSize(value);
#else
            printf_error(
                "[IO Write] ERROR: IDMA_COMMAND_BLOCK_SIZE_ADDR should be handled by SystemC FSM "
                "to access the simulated DCache!\n");
#endif
            break;
        case (IDMA_COMMAND_BLOCK_ADDR_TRIGGER_ADDR + 1):
        case (IDMA_COMMAND_BLOCK_ADDR_TRIGGER_ADDR):
#ifdef ISS_STANDALONE
            if ((addr & 0b1) !=
                0) {  //64-bit host case to transfer 64-bit addr (2x io write, this is the higher part; ADDR+1)
                assert(!dmablock->isBusy());
                dmablock->newAddrTrigger(value, 32);
            } else {
                dmablock->newAddrTrigger(value, 0);
            }
#else
            printf_error(
                "[IO Write] ERROR: IDMA_COMMAND_BLOCK_ADDR_TRIGGER_ADDR should be handled by "
                "SystemC FSM to access the simulated DCache!\n");
#endif
            break;

        case (IDMA_EXT_BASE_ADDR_E2L_ADDR):
        case (IDMA_EXT_BASE_ADDR_L2E_ADDR): {
            if (io_dma_cmd_register.ext_addr != 0) {
                io_dma_cmd_register.ext =
                    intptr_t((uint64_t(io_dma_cmd_register.ext_addr) << 32) + value);
            } else {
                io_dma_cmd_register.ext = intptr_t(value);
            }
            std::shared_ptr<CommandDMA> cmdp = std::make_shared<CommandDMA>();
            cmdp->ext_base = io_dma_cmd_register.ext;
            cmdp->loc_base = io_dma_cmd_register.loc_addr;
            cmdp->x_size = io_dma_cmd_register.x_size;
            cmdp->y_size = io_dma_cmd_register.y_size;
            cmdp->y_leap = io_dma_cmd_register.x_stride;
            cmdp->pad[0] = io_dma_cmd_register.pad_flags[0];
            cmdp->pad[1] = io_dma_cmd_register.pad_flags[1];
            cmdp->pad[2] = io_dma_cmd_register.pad_flags[2];
            cmdp->pad[3] = io_dma_cmd_register.pad_flags[3];
            cmdp->cluster_mask = io_dma_cmd_register.cluster_mask;
            assert(io_dma_cmd_register.unit_mask != 0);
            for (size_t i = 0; i < VPRO_CFG::UNITS; i++) {
                if ((io_dma_cmd_register.unit_mask >> i) & 0x1) {
                    cmdp->unit.append(i);
                }
            }
            if (addr == IDMA_EXT_BASE_ADDR_L2E_ADDR) {
                if (io_dma_cmd_register.y_size == 1)
                    cmdp->type = CommandDMA::TYPE::LOC_1D_TO_EXT_1D;
                else
                    cmdp->type = CommandDMA::TYPE::LOC_1D_TO_EXT_2D;
            } else {  // E2L
                if (io_dma_cmd_register.y_size == 1)
                    cmdp->type = CommandDMA::TYPE::EXT_1D_TO_LOC_1D;
                else
                    cmdp->type = CommandDMA::TYPE::EXT_2D_TO_LOC_1D;
            }
            run_dma_instruction(cmdp);
            io_dma_cmd_register.ext_addr = 0;
            break;
        }

        case (IDMA_COMMAND_DCACHE_ADDR + 1):
        case (IDMA_COMMAND_DCACHE_ADDR): {
#ifdef ISS_STANDALONE
            if ((addr & 0b1) !=
                0) {  //64-bit host case to transfer 64-bit addr (2x io write, this is the higher part; ADDR+1)
                assert(!dmablock->isBusy());
                dmablock->newSize(1);
                dmablock->newAddrTrigger(value, 32);
            } else {
                dmablock->newAddrTrigger(value, 0);
            }
#else
            printf_error(
                "[IO Write] ERROR: IDMA_COMMAND_BLOCK_ADDR_TRIGGER_ADDR should be handled by "
                "SystemC FSM to access the simulated DCache!\n");
#endif
            break;
        }
        default: {
            if ((addr & 0xFFFFFF00) == GP_REGISTERS_ADDR) {  // 00 - ff (byte addr)
                // GP Register
                general_purpose_register[(addr & 0xff) / 4] = value;
            } else if ((addr & 0xffff0000) == 0xfffe0000) {  //VPRO io access not yet handled
                /**
                 * Cascade to DMA IO Bus
                 */
                if ((io_cl_register_masked & 0b0000000010000000) != 0) {
                    // => dma_access
                    for (auto c : clusters) {
                        c->dma->io_write(addr, value);
                    }
                } else {
#ifndef ISS_STANDALONE
                    printf("%c", char(value));
#else
                    printf_warning(
                        "IO Write to ISS (inside VPRO Addr range) but not to existing register! "
                        "Addr: 0x%08x, Value: 0x%08x\n",
                        addr,
                        value);
#endif
                }
            } else {
#ifndef ISS_STANDALONE
                printf("%c", char(value));
#else
                printf_warning(
                    "IO Write to ISS but not to existing register! Addr: 0x%08x, Value: 0x%08x\n",
                    addr,
                    value);
#endif
            }
            break;
        }
    }
}

std::shared_ptr<CommandVPRO> ISS::gen_io_vpro_command() const {
    auto command = std::make_shared<CommandVPRO>();

    command->load_offset = io_vpro_cmd_register.src2_IMM;

    command->is_chain = io_vpro_cmd_register.is_chain;
    command->blocking = io_vpro_cmd_register.blocking;
    command->fu_sel = (io_vpro_cmd_register.func >> 4) & 0b11;
    command->func = io_vpro_cmd_register.func & 0b1111;
    command->flag_update = io_vpro_cmd_register.f_update;
    command->x_end = io_vpro_cmd_register.x_end;
    command->y_end = io_vpro_cmd_register.y_end;
    command->z_end = io_vpro_cmd_register.z_end;
    command->id_mask = io_vpro_cmd_register.id;

    command->src1.sel = io_vpro_cmd_register.src1_sel;
    command->src1.offset = io_vpro_cmd_register.src1_offset;
    command->src1.alpha = io_vpro_cmd_register.src1_alpha;
    command->src1.beta = io_vpro_cmd_register.src1_beta;
    command->src1.gamma = io_vpro_cmd_register.src1_gamma;
    command->src1.chain_left = (io_vpro_cmd_register.src1_sel == SRC_SEL_LEFT) ||
                               (io_vpro_cmd_register.src1_sel == SRC_SEL_INDIRECT_LEFT);
    command->src1.chain_right = (io_vpro_cmd_register.src1_sel == SRC_SEL_RIGHT) ||
                                (io_vpro_cmd_register.src1_sel == SRC_SEL_INDIRECT_RIGHT);
    command->src1.chain_ls = (io_vpro_cmd_register.src1_sel == SRC_SEL_LS) ||
                             (io_vpro_cmd_register.src1_sel == SRC_SEL_INDIRECT_LS);

    command->src2.sel = io_vpro_cmd_register.src2_sel;
    command->src2.offset = io_vpro_cmd_register.src2_offset;
    command->src2.alpha = io_vpro_cmd_register.src2_alpha;
    command->src2.beta = io_vpro_cmd_register.src2_beta;
    command->src2.gamma = io_vpro_cmd_register.src2_gamma;
    command->src2.chain_left = (io_vpro_cmd_register.src2_sel == SRC_SEL_LEFT) ||
                               (io_vpro_cmd_register.src2_sel == SRC_SEL_INDIRECT_LEFT);
    command->src2.chain_right = (io_vpro_cmd_register.src2_sel == SRC_SEL_RIGHT) ||
                                (io_vpro_cmd_register.src2_sel == SRC_SEL_INDIRECT_RIGHT);
    command->src2.chain_ls = (io_vpro_cmd_register.src2_sel == SRC_SEL_LS) ||
                             (io_vpro_cmd_register.src2_sel == SRC_SEL_INDIRECT_LS);

    command->dst.sel = io_vpro_cmd_register.dst_sel;
    command->dst.offset = io_vpro_cmd_register.dst_offset;
    command->dst.alpha = io_vpro_cmd_register.dst_alpha;
    command->dst.beta = io_vpro_cmd_register.dst_beta;
    command->dst.gamma = io_vpro_cmd_register.dst_gamma;
    command->dst.chain_left = (io_vpro_cmd_register.dst_sel == SRC_SEL_LEFT) ||
                              (io_vpro_cmd_register.dst_sel == SRC_SEL_INDIRECT_LEFT);
    command->dst.chain_right = (io_vpro_cmd_register.dst_sel == SRC_SEL_RIGHT) ||
                               (io_vpro_cmd_register.dst_sel == SRC_SEL_INDIRECT_RIGHT);
    command->dst.chain_ls = (io_vpro_cmd_register.dst_sel == SRC_SEL_LS) ||
                            (io_vpro_cmd_register.dst_sel == SRC_SEL_INDIRECT_LS);

    command->updateType();
    return command;
}

void ISS::check_vpro_instruction_length(const std::shared_ptr<CommandVPRO>& command) {
    if constexpr (VPRO_CFG::SIM::STRICT) {
        bool invalid_x_y_end = false;
        if (command->x_end > MAX_X_END) {
            invalid_x_y_end = true;
            printf_error(
                "ERROR! x_end out of range (is %d, max is %i)!\n", command->x_end, MAX_X_END);
            print_cmd(command.get());
        }
        if (command->y_end > MAX_Y_END) {
            invalid_x_y_end = true;
            printf_error(
                "ERROR! y_end out of range (is %d, max is %i)!\n", command->y_end, MAX_Y_END);
            print_cmd(command.get());
        }
        if (invalid_x_y_end) {
            printf_error("Exiting due to STRICT simulation mode.");
            std::exit(EXIT_FAILURE);
        }
    }
}

// ###############################################################################################################################
// Debugging Functions
// ###############################################################################################################################

void ISS::sim_dump_local_memory(uint32_t cluster, uint32_t unit) {
    if (if_debug(DEBUG_USER_DUMP)) {
        if (unit > VPRO_CFG::UNITS) {
            printf_warning(
                "#SIM_DUMP: Invalid unit selection (%d)!. Maximum is %d.", unit, VPRO_CFG::UNITS);
        } else if (cluster > VPRO_CFG::CLUSTERS) {
            printf_warning("#SIM_DUMP: Invalid cluster selection (%d)!. Maximum is %d.",
                cluster,
                VPRO_CFG::CLUSTERS);
        } else {
            printf("#SIM_DUMP: Local memory, cluster=%d unit=%d\n", cluster, unit);
            this->clusters[cluster]->dumpLocalMemory(unit);
        }
    }
}

void ISS::sim_dump_queue(uint32_t cluster, uint32_t unit) {
    if (if_debug(DEBUG_USER_DUMP)) {
        if (unit > VPRO_CFG::UNITS) {
            printf_warning(
                "#SIM_DUMP: Invalid unit selection (%d)!. Maximum is %d.", unit, VPRO_CFG::UNITS);
        } else if (cluster > VPRO_CFG::CLUSTERS) {
            printf_warning("#SIM_DUMP: Invalid cluster selection (%d)!. Maximum is %d.",
                cluster,
                VPRO_CFG::CLUSTERS);
        } else {
            printf("#SIM_DUMP: Local memory, cluster=%d unit=%d\n", cluster, unit);
            this->clusters[cluster]->dumpQueue(unit);
        }
    }
}

void ISS::sim_dump_register_file(uint32_t cluster, uint32_t unit, uint32_t lane) {
    if (if_debug(DEBUG_USER_DUMP)) {
        if (unit > VPRO_CFG::UNITS) {
            printf_warning(
                "#SIM_DUMP: Invalid unit selection (%d)!. Maximum is %d.", unit, VPRO_CFG::UNITS);
        } else if (cluster > VPRO_CFG::CLUSTERS) {
            printf_warning("#SIM_DUMP: Invalid cluster selection (%d)!. Maximum is %d.",
                cluster,
                VPRO_CFG::CLUSTERS);
        } else if (lane > VPRO_CFG::LANES) {
            printf_warning(
                "#SIM_DUMP: Invalid lane selection (%d)!. Maximum is %d.", lane, VPRO_CFG::LANES);
        } else {
            printf("#SIM_DUMP: Register file, cluster=%d unit=%d lane=%d\n", cluster, unit, lane);
            this->clusters[cluster]->dumpRegisterFile(unit, lane);
        }
    }
}

void ISS::sim_wait_step(bool finish, char const* msg) {
    if (finish) {
        io_write(VPRO_BUSY_MASK_CL_ADDR, 0xffffffff);
        while ((io_read(VPRO_BUSY_MASKED_DMA_ADDR) & 0xffffffff) != 0) {}
        io_write(VPRO_BUSY_MASK_CL_ADDR, 0xffffffff);
        while ((io_read(VPRO_BUSY_MASKED_VPRO_ADDR) & 0xffffffff) != 0) {}
    }

    sendSimUpdate();
    printf(ORANGE);
    printf("\n\t%s\n", msg);
    pauseSim();

    printf(RESET_COLOR);
}

// copies data in simulated main memory
int ISS::bin_file_send(uint32_t addr, int num_bytes, char const* file_name) {
    if (debug & DEBUG_MODE) {
        printf("\n#SIM: bin_file_send\nBase: 0x%08x\nSize: %d bytes\nSrc:  %s\n\n",
            addr,
            num_bytes,
            file_name);
    }
    while ((num_bytes & 7) != 0) {
        num_bytes++;
    }  // must be a multiple of 8

    auto pfh = fopen(file_name, "rb");
    if (pfh == nullptr) {
        printf_error("Unable to open file %s!\n", file_name);
        return -1;
    }

    auto buf = (uint8_t*)malloc(num_bytes * sizeof(uint8_t));
    auto read = fread(buf, 1, num_bytes, pfh);
    fclose(pfh);

    // copy to simulated main memory
    for (int i = 0; i < (uint32_t)num_bytes; i++) {
        bus->dbgWrite(addr + i, buf + i);
        // main memory access in range?
        if ((addr + i) > (VPRO_CFG::MM_SIZE - 1)) {
            printf_error("\n#SIM: bin_file_send\n");
            printf_error("#SIM: Main memory access out of range!\n");
            printf_error("Access address: 0x%08x, max address: 0x%08x\nAborting.\n",
                addr + i,
                VPRO_CFG::MM_SIZE - 1);
            exit(1);
        }
    }
    free(buf);
    return 0;
}

// returns data from simulated main memory
uint8_t* ISS::bin_file_return(uint32_t addr, int num_bytes) {
    if (debug & DEBUG_MODE) {
        printf("\n#SIM: bin_file_return\n");
        printf("Base: 0x%08x\n", addr);
        printf("Size: %d bytes\n", num_bytes);
    }
    while ((num_bytes & 7) != 0) {
        num_bytes++;
    }  // must be a multiple of 8

    auto* buf = (uint8_t*)malloc(num_bytes);
    for (int i = 0; i < (uint32_t)num_bytes; i++) {  // copy from simulated main memory
        bus->dbgRead(addr + i, buf + i);
        if ((addr + i) > (VPRO_CFG::MM_SIZE - 1)) {  // main memory access in range?
            printf_error("\n#SIM: bin_file_return\n");
            printf_error("#SIM: Main memory access out of range!\n");
            printf_error("Access address: 0x%08x, max address: 0x%08x\nAborting.\n",
                addr + i,
                VPRO_CFG::MM_SIZE - 1);
            exit(1);
        }
    }
    return (buf);
}

// returns data from simulated main memory
uint16_t* ISS::get_main_memory(uint32_t addr, unsigned int num_elements_16bit) {
    if (num_elements_16bit == 0)
        printf("get_main_memory: num_elements_16bit <=0 -> no memory was allocated \n");

    unsigned int num_elements_8bit;
    num_elements_8bit = num_elements_16bit * 2;
    auto* buffer_8bit = (uint8_t*)malloc(num_elements_8bit * sizeof(uint8_t));
    if ((addr + num_elements_8bit) > (VPRO_CFG::MM_SIZE - 1)) {  // main memory access in range?
        printf_error("\n#SIM: bin_file_return\n");
        printf_error("#SIM: Main memory access out of range!\n");
        printf_error("Access address: 0x%08x, max address: 0x%08x\nAborting.\n",
            addr + num_elements_8bit,
            VPRO_CFG::MM_SIZE - 1);
        exit(1);
    }
    for (int i = 0; i < num_elements_8bit; i++) {
        bus->dbgRead(addr + i, buffer_8bit + i);
    }
    auto buffer_16bit = (uint16_t*)buffer_8bit;
    return buffer_16bit;
}

uint8_t* ISS::bin_lm_return(uint32_t cluster, uint32_t unit) {
    auto u = this->clusters[cluster]->getUnits().at(unit);

    auto lmdata =
        new uint8_t[VPRO_CFG::LM_SIZE * Unit::VectorUnit::LOCAL_MEMORY_DATA_WIDTH]();
    for (int lm = 0; lm < VPRO_CFG::LM_SIZE * Unit::VectorUnit::LOCAL_MEMORY_DATA_WIDTH; lm++)
        lmdata[lm] = u->getLocalMemoryPtr()[lm];

    return lmdata;
}

uint8_t* ISS::bin_rf_return(uint32_t cluster, uint32_t unit, uint32_t lane) {
    auto u = this->clusters[cluster]->getUnits().at(unit);
    auto l = u->getLanes().at(lane);

    auto rfdata = new uint8_t[VPRO_CFG::RF_SIZE * 3]();
    for (int lm = 0; lm < VPRO_CFG::RF_SIZE * 3; lm++) {
        rfdata[lm] = l->getregister()[lm];
    }
    return rfdata;
}

// writes data from simulated main memory to file
int ISS::bin_file_dump(uint32_t addr, int num_bytes, char const* file_name) {
    if (debug & DEBUG_MODE) {
        printf("\n#SIM: bin_file_dump\n");
        printf("Base: 0x%08x\n", addr);
        printf("Size: %d bytes\n", num_bytes);
        printf("Dst:  %s\n\n", file_name);
    }
    while ((num_bytes & 7) != 0) {
        num_bytes++;
    }  // must be a multiple of 8

    auto buf = (uint8_t*)malloc(num_bytes * sizeof(uint8_t));

    for (int i = 0; i < (uint32_t)num_bytes; i++) {  // copy from simulated main memory
        bus->dbgRead(addr + i, buf + i);
        if ((addr + i) > (VPRO_CFG::MM_SIZE - 1)) {  // main memory access in range?
            printf_error("\n#SIM: bin_file_dump\n");
            printf_error("#SIM: Main memory access out of range!\n");
            printf_error("Access address: 0x%08x, max address: 0x%08x\nAborting.\n",
                addr + i,
                VPRO_CFG::MM_SIZE - 1);
            exit(1);
        }
    }

    auto pfh = fopen(file_name, "wb+");
    if (pfh == nullptr) {
        printf_error("Unable to open file %s!\n", file_name);
        return -1;
    }
    fwrite(buf, 1, num_bytes, pfh);
    fclose(pfh);
    free(buf);
    return 0;
}

void ISS::aux_memset(uint32_t base_addr, uint8_t value, uint32_t num_bytes) {
    auto cmd = new CommandSim();
    cmd->type = CommandSim::AUX_MEMSET;
    cmd->num_bytes = num_bytes;
    cmd->value = value;
    cmd->addr = base_addr;

    if (debug & DEBUG_MODE) {
        printf("\n#SIM: aux_memset\n");
        printf("Base: 0x%08x\n", cmd->addr);
        printf("Data: 0x%02x\n", cmd->value);
        printf("Size: %d bytes\n", cmd->num_bytes);
    }
    for (uint i = cmd->addr; i < (cmd->addr + cmd->num_bytes); i++) {
        // main memory access in range?
        if (i > (VPRO_CFG::MM_SIZE - 1)) {
            printf_error("\n#SIM: aux_memset\n");
            printf_error("#SIM: Main memory access out of range!\n");
            printf_error("Access address: 0x%08x, max address: 0x%08x\nAborting.\n",
                cmd->addr + i,
                VPRO_CFG::MM_SIZE - 1);
            exit(1);
        }
        bus->dbgWrite(i, &(cmd->value));
    }
}
