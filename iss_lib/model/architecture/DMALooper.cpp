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
// Created by gesper on 26.05.23.
//

#include "DMALooper.h"

#include "../../simulator/ISS.h"
#include "../commands/CommandDMA.h"

DMALooper::DMALooper(ISS* core) : core(core) {}

void DMALooper::tick() {
    if (state == LOOPING) {
        busy = true;

        if (cluster <= dma_loop.cluster_loop_len) {
            if (unit < dma_loop.unit_loop_len) {
                if (inter_unit < dma_loop.inter_unit_loop_len) {
                    inter_unit++;
                    lm_addr = lm_addr + signext_13to16(dma_loop.lm_incr);
                } else {
                    unit++;
                    inter_unit = 0;
                    lm_addr = base_lm_addr;
                    if (dma_loop.unit_loop_shift_incr >= 0)
                        unit_mask = unit_mask << dma_loop.unit_loop_shift_incr;
                    else
                        unit_mask = unit_mask >> -dma_loop.unit_loop_shift_incr;
                }
            } else {
                if (inter_unit < dma_loop.inter_unit_loop_len) {
                    inter_unit++;
                    lm_addr = lm_addr + signext_13to16(dma_loop.lm_incr);
                } else {
                    cluster++;
                    unit = 0;
                    inter_unit = 0;
                    lm_addr = base_lm_addr;
                    unit_mask = base_unit_mask;
                    if (dma_loop.cluster_loop_shift_incr >= 0)
                        cluster_mask = cluster_mask << dma_loop.cluster_loop_shift_incr;
                    else
                        cluster_mask = cluster_mask >> -dma_loop.cluster_loop_shift_incr;
                }
            }
        } else {
            if (generated_commands != dma_loop.dma_cmd_count) {
                printf_error(
                    "[DMA Loop] Error! loop iterations done but not yet enaugh -> fsm error!\n");
                total_generated_commands += generated_commands;
                state = IDLE;
                busy = false;
                return;
            }
        }
        mm_addr = mm_addr + dma_loop.mm_incr;
        base_dma_cmd.mm_addr = mm_addr;
        base_dma_cmd.lm_addr = lm_addr;
        base_dma_cmd.unit_mask = unit_mask;
        base_dma_cmd.cluster = cluster_mask;

        generated_commands++;

        if ((generated_commands == dma_loop.dma_cmd_count) ||
            (cluster == dma_loop.cluster_loop_len && unit == dma_loop.unit_loop_len &&
                inter_unit == dma_loop.inter_unit_loop_len)) {
            // this was the last iteration
            total_generated_commands += generated_commands;
            state = IDLE;
            busy = false;
        }

        // TODO: check if fifo full
        core->run_dma_instruction(bif_to_dma(&base_dma_cmd), true);
    } else {  // not looping
        if (input_register.is_filled) {
            busy = true;
            auto* dma = (COMMAND_DMA::COMMAND_DMA*)input_register.dcache_data_struct;

            if (state == WAIT_FOR_BASE) {
                memcpy(&base_dma_cmd, dma, 32);
                state = LOOPING;

                base_lm_addr = dma->lm_addr;
                base_unit_mask = dma->unit_mask;
                base_cluster_mask = dma->cluster;
                base_mm_addr = dma->mm_addr;

                cluster_mask = dma->cluster;
                unit_mask = dma->unit_mask;
                lm_addr = dma->lm_addr;
                mm_addr = dma->mm_addr;

                cluster = 0;
                unit = 0;
                inter_unit = 0;

                generated_commands = 0;

                generated_commands++;
                // TODO: check if fifo full
                core->run_dma_instruction(bif_to_dma(&base_dma_cmd), true);
            } else {
                busy = false;
                if (dma->direction == COMMAND_DMA::DMA_DIRECTION::loop) {  // loop command
                    memcpy(&dma_loop, dma, 32);
                    state = WAIT_FOR_BASE;
                } else {  // direct dma
                    core->run_dma_instruction(bif_to_dma(dma), true);
                }
            }
            input_register.is_filled = false;
        }  // not looping
    }
}

void DMALooper::new_dcache_input(const uint8_t dcache_data_struct[32]) {
    if (input_register.is_filled) {
        printf_error(
            "\n[DMA Looper] Looping is active. new_dcache_input. Got a new input - RISC-V should "
            "wait! \n");

        auto dma = ((COMMAND_DMA::COMMAND_DMA*)input_register.dcache_data_struct);
        if (dma->direction == COMMAND_DMA::DMA_DIRECTION::loop) {
            auto loop = ((COMMAND_DMA::COMMAND_DMA_LOOP*)input_register.dcache_data_struct);
            printf_info("\tNew Command is DCACHE DMA Loop: %s\n", loop->to_char());
        } else {
            printf_info("\tNew Command is DCACHE DMA: %s\n", dma->to_char());
        }
    }

    memcpy(input_register.dcache_data_struct, dcache_data_struct, 32);
    input_register.is_filled = true;

    if (if_debug(DEBUG_DMA)) {
        auto dma = ((COMMAND_DMA::COMMAND_DMA*)dcache_data_struct);
        if (dma->direction == COMMAND_DMA::DMA_DIRECTION::loop) {
            auto loop = ((COMMAND_DMA::COMMAND_DMA_LOOP*)dcache_data_struct);
            printf_info("[DCache DMA | Looper] New LOOP Command: %s\n", loop->to_char());
        } else {
            printf_info("[DCache DMA] New DMA, direct Command: %s\n", dma->to_char());
        }
    }
}

std::shared_ptr<CommandDMA> DMALooper::bif_to_dma(COMMAND_DMA::COMMAND_DMA* dma) {
    CommandDMA cmdi;
    std::shared_ptr<CommandDMA> cmdp = std::make_shared<CommandDMA>(cmdi);
    cmdp->ext_base = (uint64_t(dma->mm_addr_64) << 32) + uint64_t(dma->mm_addr);
    cmdp->loc_base = dma->lm_addr;
    cmdp->y_leap = dma->y_leap;
    cmdp->x_size = dma->x_size;
    cmdp->y_size = dma->y_size;
    cmdp->pad[0] = dma->padding & 0b0001;
    cmdp->pad[1] = dma->padding & 0b0010;
    cmdp->pad[2] = dma->padding & 0b0100;
    cmdp->pad[3] = dma->padding & 0b1000;
    cmdp->cluster_mask = dma->cluster;
    switch (dma->direction) {
        case COMMAND_DMA::e2l1D:
            cmdp->type = CommandDMA::TYPE::EXT_1D_TO_LOC_1D;
            break;
        case COMMAND_DMA::e2l2D:
            cmdp->type = CommandDMA::TYPE::EXT_2D_TO_LOC_1D;
            break;
        case COMMAND_DMA::l2e1D:
            cmdp->type = CommandDMA::TYPE::LOC_1D_TO_EXT_1D;
            break;
        case COMMAND_DMA::l2e2D:
            cmdp->type = CommandDMA::TYPE::LOC_1D_TO_EXT_2D;
            break;
        default:
            break;
    }

    assert(dma->unit_mask != 0);
    for (size_t i = 0; i < VPRO_CFG::UNITS; i++) {
        if ((dma->unit_mask >> i) & 0x1) {
            cmdp->unit.append(i);
        }
    }
    if (cmdp->y_size == 0)
        printf_error("[Error: DMA Direct] got a y size of 0! Infinite Transfer!\n");

    return cmdp;
}
