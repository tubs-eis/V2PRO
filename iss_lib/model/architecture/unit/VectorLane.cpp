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
 * @file VectorLane.cpp
 * @author Alexander Köhne, Sven Gesper
 * @date Created by Gesper, 06.03.19
 *
 * Architecture vector lane
 */

#include "VectorLane.h"
#include <stdlib.h>
#include <bitset>
#include <iostream>
#include "../../../core_wrapper.h"
#include "../../../simulator/ISS.h"
#include "../../../simulator/helper/debugHelper.h"
#include "../../../simulator/helper/typeConversion.h"
#include "../../../simulator/setting.h"
#include "../Cluster.h"
#include "VectorUnit.h"

#include <tuple>

#include <string>
using std::to_string;

namespace Unit {

bool VectorLane::additional_check() {
    return true;
}

VectorLane::VectorLane(int id,
    VectorUnit* vector_unit,
    std::shared_ptr<ArchitectureState> architecture_state)
    : vector_lane_id(id),
      vector_unit(vector_unit),
      architecture_state(architecture_state),
      clock_cycle(0),
      blocking(false),
      blocking_nxt(false),
      src_lane_stall(false),
      dst_lane_stall(false),
      dst_lane_ready(false),
      updated(false),
      consecutive_DST_stall_counter(0),
      consecutive_SRC_stall_counter(0),
      regFile(vector_unit->cluster_id,
          vector_unit->vector_unit_id,
          vector_lane_id,
          VPRO_CFG::RF_SIZE,
          (id == 1 << VPRO_CFG::LANES)) {
    current_cmd = std::make_shared<CommandVPRO>();
    new_cmd = std::make_shared<CommandVPRO>();
    noneCmd = std::make_shared<CommandVPRO>();
    pipeObj = std::make_unique<PipeObject>(5 + CommandVPRO::MAX_ALU_DEPTH, 3);
}

/**
 * checks if the current command is processed completely
 * @return bool
 */
bool VectorLane::isBusy() const {
    return isBlocking() || pipeObj->isBusy();
}
bool VectorLane::isBlocking() const {
    return blocking;
}

std::shared_ptr<CommandVPRO> VectorLane::getCmd() {
    return current_cmd;
}

void VectorLane::check_src_lane_stall() {       // chain data to stage 4
    constexpr int pipeline_src_read_stage = 3;  // in next tick this will be 4 (RD stage
    src_lane_stall = false;

    check_stall_conditions_msg(pipeline_src_read_stage);

    if (isLSLane()) {
        if ((*pipeObj)[pipeline_src_read_stage].cmd->isWriteLM()) {  // store command
            auto lane_src =
                vector_unit->getLanes()
                    [(*pipeObj)[pipeline_src_read_stage]
                            .cmd->dst.gamma];  // ALWAYS lane chain source -> TODO: immediate?!g
            src_lane_stall = lane_src->fifo._is_empty();
        }
    } else {
        const addr_field_t& src1 = (*pipeObj)[pipeline_src_read_stage].cmd->src1;
        if (is_src_chaining(src1)) {
            src_lane_stall = get_lane_from_src(src1).fifo._is_empty();
        }

        const addr_field_t& src2 = (*pipeObj)[pipeline_src_read_stage].cmd->src2;
        if (is_src_chaining(src2)) {
            src_lane_stall = get_lane_from_src(src2).fifo._is_empty();
        }
    }
}

void VectorLane::check_dst_lane_stall() {
    dst_lane_stall = false;
    if (pipeObj->isChaining() && (!fifo._is_fillable())) {
        dst_lane_stall = true;
    }
}

void VectorLane::check_adr_lane_stall() {
    constexpr int pipeline_adr_read_stage = 2;  // in next tick this will be 3
    adr_lane_stall = false;

    addr_field_t src1 = (*pipeObj)[pipeline_adr_read_stage].cmd->src1;
    if (is_indirect_addr(src1)) {
        auto& lane_src1 = get_lane_from_src(src1);
        if (lane_src1.fifo._is_empty()) adr_lane_stall = true;
    }

    addr_field_t src2 = (*pipeObj)[pipeline_adr_read_stage].cmd->src2;
    if (is_indirect_addr(src2)) {
        auto& lane_src2 = get_lane_from_src(src2);
        if (lane_src2.fifo._is_empty()) adr_lane_stall = true;
    }

    addr_field_t dst = (*pipeObj)[pipeline_adr_read_stage].cmd->dst;
    if (is_indirect_addr(dst)) {
        auto& lane_dst = get_lane_from_src(dst);
        if (lane_dst.fifo._is_empty()) adr_lane_stall = true;
    }
}

void VectorLane::check_stall_conditions() {  //kombinatorische

    check_src_lane_stall();
    check_dst_lane_stall();
    check_adr_lane_stall();
}

/**
     * Checks if current CMD is done, fetch next one, adjust the ALUDepth
     */
void VectorLane::fetchCMD() {
    if (current_cmd->is_done()) {  // Get a new command if the "old" is done
        if (new_cmd->is_done()) {
            new_cmd = vector_unit->getNextCommandForLane(vector_lane_id);
        }  // if there is already a new cmd fetched...
        if (new_cmd->pipelineALUDepth <
            pipeObj->pipelineALUDepth) {  // check whether to delay the start?
            ifx_debug(DEBUG_PIPELINELENGTH_CHANGES, [&]() {
                printf_info(
                    "Got a New Command (buffer) for Lane %i cause old one is DONE! [WAIT cause "
                    "pipeline Length has changed!]\n",
                    vector_lane_id);
            });
            pipeObj->pipelineALUDepth--;  // maybe next tick
            current_cmd = std::make_shared<CommandVPRO>();
            current_cmd->id_mask = 1u << uint(vector_lane_id);
        } else {
            pipeObj->pipelineALUDepth = new_cmd->pipelineALUDepth;
            current_cmd = new_cmd;
        }
    }
}

/**
 * process X, Y and Z SEQ
 */
void VectorLane::processCMD() {
    current_cmd->x++;
    if (current_cmd->x > current_cmd->x_end) {
        current_cmd->x = 0;
        current_cmd->y++;
        if (current_cmd->y > current_cmd->y_end) {
            current_cmd->y = 0;
            current_cmd->z++;
            if (current_cmd->z > current_cmd->z_end) {
                Statistics::get().getVPROStat()->addExecutedCmdQueue(
                    current_cmd.get(), vector_lane_id);
                current_cmd->done = true;
            }
        }
    }
}

/**
     * clock tick to process lanes command/pipeline
     */
void VectorLane::tick() {
    check_stall_conditions();

    //*********************************************
    // Execute Pipeline
    //*********************************************

    if (!adr_lane_stall && !src_lane_stall && !dst_lane_stall) {  // run all
        Statistics::get().getVPROStat()->addExecutedCmdTick(current_cmd.get(), vector_lane_id);
        pipeObj->process(current_cmd);
        pipeObj->tick_pipeline(
            *this, 0, 5 + pipeObj->pipelineALUDepth + 1);  // execute ALL pipeline stages
    } else if (adr_lane_stall && !src_lane_stall && !dst_lane_stall) {  // run from stage 3
        Statistics::get().getVPROStat()->addExecutedCmdTick(noneCmd.get(), vector_lane_id);
        int from = chain_target_stage;
        pipeObj->processInStall(from);
        pipeObj->tick_pipeline(
            *this, from, 4 + pipeObj->pipelineALUDepth + 1);  // execute later pipeline stages
    } else if (src_lane_stall && !adr_lane_stall && !dst_lane_stall) {  // run from src if SRC wait
        Statistics::get().getVPROStat()->addExecutedCmdTick(noneCmd.get(), vector_lane_id);
        int from = chain_target_stage + 1;  // 3+1=4 the "fifth" pipeline stage
        pipeObj->processInStall(from);
        pipeObj->tick_pipeline(
            *this, from, 5 + pipeObj->pipelineALUDepth + 1);  // execute later pipeline stages
    } else if (src_lane_stall && dst_lane_stall) {            // run nothing if both stall
        Statistics::get().getVPROStat()->addExecutedCmdTick(noneCmd.get(), vector_lane_id);
    } else if (!src_lane_stall && dst_lane_stall) {  // run nothing if DST stall
        Statistics::get().getVPROStat()->addExecutedCmdTick(noneCmd.get(), vector_lane_id);
    }

    //*********************************************
    // Update Registers
    //*********************************************

    if (!dst_lane_stall) {  // run later part! (all)
        (*pipeObj)[5 + pipeObj->pipelineALUDepth + 1].cmd->blocking = false;
    }
}
/**
     * @brief Seq Update
     *
     */
void VectorLane::update() {
    if (!adr_lane_stall && !src_lane_stall && !dst_lane_stall) {
        processCMD();
    }

    clock_cycle++;
    if (dst_lane_stall) {
        consecutive_DST_stall_counter++;
    } else {
        consecutive_DST_stall_counter = 0;
    }

    if (src_lane_stall) {
        consecutive_SRC_stall_counter++;
    } else {
        consecutive_SRC_stall_counter = 0;
    }

    if (adr_lane_stall) {
        consecutive_ADR_stall_counter++;
    } else {
        consecutive_ADR_stall_counter = 0;
    }

    fetchCMD();
    updateDebugMsg();
    print_fifo();
    regFile.update();
    fifo.update();

    //blocks new CMD
    blocking = current_cmd->blocking || pipeObj->isBlocking(chain_target_stage);
    //processPipe
    pipeObj->update();
    lane_chaining = pipeObj->isChaining();

    // reset register will be set from other lane
    blocking_nxt = false;
    lane_chaining_nxt = false;
    dst_lane_ready = false;
}

bool VectorLane::is_src_chaining(addr_field_t src) const {
    switch (src.sel) {
        case SRC_SEL_LEFT:
        case SRC_SEL_RIGHT:
        case SRC_SEL_LS:
            return true;
        default:
            return false;
    }
}

bool VectorLane::is_indirect_addr(addr_field_t adr) const {
    return adr.sel == SRC_SEL_INDIRECT_LEFT || adr.sel == SRC_SEL_INDIRECT_RIGHT ||
           adr.sel == SRC_SEL_INDIRECT_LS;
}

VectorLane& VectorLane::get_lane_from_src(addr_field_t src) {
    if (isLSLane()) {
        switch (src.sel) {
            case SRC_SEL_LEFT:
            case SRC_SEL_INDIRECT_LEFT:
                return *left_lane;
            case SRC_SEL_RIGHT:
            case SRC_SEL_INDIRECT_RIGHT:
                return *right_lane;
            case SRC_SEL_LS:
            case SRC_SEL_INDIRECT_LS:
                if (isLSLane()) {  // for chaining between LS Lanes
                    if (src.chain_right) return vector_unit->getRightNeighbor()->getLSLane();
                    if (src.chain_left) return vector_unit->getLeftNeighbor()->getLSLane();
                }
                return getLSLane();
            default:
                return *this;
        }
    } else {
        switch (src.sel) {
            case SRC_SEL_LEFT:
            case SRC_SEL_INDIRECT_LEFT:
                return getLeftNeighbor();
            case SRC_SEL_RIGHT:
            case SRC_SEL_INDIRECT_RIGHT:
                return getRightNeighbor();
            case SRC_SEL_LS:
            case SRC_SEL_INDIRECT_LS:
                return getLSLane();
            default:
                return *this;
        }
    }
}

/**
     * to get SRC1 and SRC2 data this function resolves the source (Immediate, Left Lane Chain data, Right ...
     * or simple address to the register file)
     * @param src contains selection and immediate/alpha/beta for adress calc
     * @param x currents commands 'iterations'
     * @param y
     * @param x_end
     * @return uint8_t[3] array with data for specified operand
     */
std::tuple<word_t, bool, bool> VectorLane::get_operand(
    addr_field_t src, word_t x, word_t y, word_t z) {
    word_t op_data = 0;
    bool z_flag = false;
    bool n_flag = false;

    switch (src.sel) {
        case SRC_SEL_INDIRECT_LS:
        case SRC_SEL_INDIRECT_LEFT:
        case SRC_SEL_INDIRECT_RIGHT:
        case SRC_SEL_ADDR:
            op_data =
                regFile.get_rf_data(src.offset + src.alpha * x + src.beta * y + src.gamma * z);
            z_flag =
                regFile.get_rf_flag(src.offset + src.alpha * x + src.beta * y + src.gamma * z, 0);
            n_flag =
                regFile.get_rf_flag(src.offset + src.alpha * x + src.beta * y + src.gamma * z, 1);
            break;
        case SRC_SEL_IMM:
            op_data = src.getImm();
            if (uint8_t(op_data >> ISA_COMPLEX_LENGTH_3D) >
                0)  // cannot happen. immediate is cut to ISA_COMPLEX_LENGTH_2D bit...
                printf_warning(
                    "using immediate as operand which is more than 24 bit! upper 8 bit got "
                    "ignored!\n");
            op_data = *__24to32signed(op_data);
            break;
        case SRC_SEL_LEFT:
        case SRC_SEL_RIGHT:
        case SRC_SEL_LS:
            if (isLSLane()) {  // LS store data
                get_operand_debug_chaining_msg(*vector_unit->getLanes()[src.gamma]);
                std::tie(op_data, z_flag, n_flag) = vector_unit->getLanes()[src.gamma]->fifo.pop();
            } else {  // processing lane
                get_operand_debug_chaining_msg(get_lane_from_src(src));
                std::tie(op_data, z_flag, n_flag) = get_lane_from_src(src).fifo.pop();
            }
            break;
        default:
            printf_error("VPRO_SIM ERROR: Invalid source selection! (src_sel=%d)\n", src.sel);
            exit(1);
    }
    return std::tie(op_data, z_flag, n_flag);
}

void VectorLane::print_pipeline(const QString prefix) {
    if (!additional_check()) return;

    printf(ORANGE);
    if (vector_unit->cluster_id == 0 && vector_unit->vector_unit_id == 0 && vector_lane_id == 0)
        printf("PRINT PIPELINE AFTER_CYCLE: %5li   DEPTH: %i \n",
            clock_cycle,
            5 + pipeObj->pipelineALUDepth + 1);
    printf("C%02iU%02i", vector_unit->cluster_id, vector_unit->vector_unit_id);
    if (isLSLane())
        printf("LS: ");
    else
        printf("L%i: ", vector_lane_id);

    int num_stages = 5 + pipeObj->pipelineALUDepth + 1;
    for (int stage = 0; stage < num_stages; stage++) {
        printf("|");
        if ((*pipeObj)[stage].cmd->type == CommandVPRO::NONE) {
            printf(INVERTED);
            if (src_lane_stall || dst_lane_stall)
                printf(RESET_COLOR
                    "\e[30m"
                    "\e[100m");
            printf("    ");
            (*pipeObj)[stage].cmd->print_type();
            printf(RESET_COLOR "\e[49m");
            printf(ORANGE);
        } else {
            int cmd_vector_pos =
                (*pipeObj)[stage].cmd->z *
                    (((*pipeObj)[stage].cmd->x_end + 1) * ((*pipeObj)[stage].cmd->y_end + 1)) +
                (*pipeObj)[stage].cmd->y * ((*pipeObj)[stage].cmd->x_end + 1) +
                (*pipeObj)[stage].cmd->x;
            if (src_lane_stall || dst_lane_stall) printf("\e[100m");
            if (cmd_vector_pos == 0) printf("\e[44m");
            printf("%3i ", cmd_vector_pos);
            (*pipeObj)[stage].cmd->print_type();
            printf(ORANGE "\e[49m");
        }
    }
    printf("|");
    printf("  %s", (lane_chaining) ? "DATA READY" : "\e[90mDATA READY\e[93m");
    printf((dst_lane_ready) ? " CHAIN TO " : "\e[90m CHAIN TO \e[93m");
    printf("%s", (isBlocking() ? "\tBLOCKING" : ""));
    if (dst_lane_stall) printf("  STALL_DST");
    if (src_lane_stall) printf("  STALL_SRC ");
    printf("\n");
    printf(RESET_COLOR);
    if (vector_unit->cluster_id == VPRO_CFG::CLUSTERS - 1 &&
        vector_unit->vector_unit_id == VPRO_CFG::UNITS - 1 && vector_lane_id == VPRO_CFG::LANES)
        printf("\n");
}

}  // namespace Unit
