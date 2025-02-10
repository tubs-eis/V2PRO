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
#include <tuple>
#include "../../../simulator/helper/debugHelper.h"
#include "../../../simulator/helper/typeConversion.h"
#include "../Cluster.h"
#include "VectorLane.h"
#include "VectorUnit.h"

namespace Unit {

void VectorLane::updateDebugMsg() {
    //*********************************************
    // Debug Print messages
    //*********************************************
    if (if_debug(DEBUG_INSTRUCTIONS)) {
        printf("[Pipeline 0] (new?) Processed Instr.: \tUnit(%2i) Lane(%2i):",
            vector_unit->vector_unit_id,
            vector_lane_id);
        if ((*pipeObj)[0].cmd->type == CommandVPRO::NONE) {
            printf("\tCMD: NONE \n");
        } else {
            printf("\tCMD: ");
            print_cmd((*pipeObj)[0].cmd.get());
        }
    }
    if (if_debug(DEBUG_PIPELINE) && additional_check()) {
        print_pipeline();
    }
    if (if_debug(DEBUG_PIPELINE_9)) {
        printf(BLUE);
        printf("\t\t\tResult processed from pipeline[9]: %i\n", (*pipeObj)[9].data);
        if (!__is_zero((*pipeObj)[9].data)) dumpRegisterFile("\t\t");
        printf(RESET_COLOR);
    }

    //Debug Limitation
    if (if_debug(DEBUG_INSTRUCTION_DATA) &&
        additional_check()) {  // PRINT operations DATA and Addresses
        auto pipe = (*pipeObj)[pipeObj->pipelineALUDepth + 5];  //???
        if (this->isLSLane()) {
            if (pipe.cmd->type != CommandVPRO::NONE) {
                printf("\e[48;5;232m");
                printf(BLUE);
                printf("C%2i U%2i LS", vector_unit->cluster_id, vector_unit->vector_unit_id);
                printf("\t");
                pipe.cmd->print_type();
                int cmd_vector_pos = pipe.cmd->z * (pipe.cmd->x_end + 1) * (pipe.cmd->y_end + 1) +
                                     pipe.cmd->y * (pipe.cmd->x_end + 1) + pipe.cmd->x + 1;
                int cmd_vector_length =
                    (pipe.cmd->x_end + 1) * (pipe.cmd->y_end + 1) * (pipe.cmd->z_end + 1);
                printf("%2i/%2i", cmd_vector_pos, cmd_vector_length);
                if (is_src_chaining(pipe.cmd->src1))
                    printf((get_lane_from_src(pipe.cmd->src1).isLSLane()) ? "\t CH[  LS]="
                                                                          : "\t CH[  L%i]=",
                        get_lane_from_src(pipe.cmd->src1).vector_lane_id);
                else if (is_src_chaining(pipe.cmd->src2))
                    printf((get_lane_from_src(pipe.cmd->src2).isLSLane()) ? "\t CH[  LS]="
                                                                          : "\t CH[  L%i]=",
                        get_lane_from_src(pipe.cmd->src2).vector_lane_id);
                else
                    printf_info("\t LM[%4i]=", pipe.lm_addr);
                printf("\033[0m");
                if (pipe.data != 0) printf(INVERTED);
                if (pipe.cmd->isWriteLM())
                    printf_info("%13i (0x%04x)", int(pipe.data), pipe.data);
                else
                    printf_info("%13i (0x%04x)", int(pipe.pre_data), pipe.pre_data);

                printf(RESET_COLOR "\n");
            }
        } else {
            if (pipe.cmd->type != CommandVPRO::NONE) {
                if (vector_lane_id == 0) printf("\e[48;5;235m");
                if (vector_lane_id == 1) printf("\e[48;5;238m");
                printf(BLUE);
                printf("C%2i U%2i L%i",
                    vector_unit->cluster_id,
                    vector_unit->vector_unit_id,
                    vector_lane_id);
                printf("\t");
                pipe.cmd->print_type();
                int cmd_vector_pos = pipe.cmd->z * (pipe.cmd->x_end + 1) * (pipe.cmd->y_end + 1) +
                                     pipe.cmd->y * (pipe.cmd->x_end + 1) + pipe.cmd->x + 1;
                int cmd_vector_length =
                    (pipe.cmd->x_end + 1) * (pipe.cmd->y_end + 1) * (pipe.cmd->z_end + 1);
                printf("%2i/%2i", cmd_vector_pos, cmd_vector_length);

                if (is_src_chaining(pipe.cmd->src1))
                    printf((get_lane_from_src(pipe.cmd->src1).isLSLane()) ? "\t CH[  LS]="
                                                                          : "\t CH[  L%i]=",
                        get_lane_from_src(pipe.cmd->src1).vector_lane_id);
                else
                    printf_info("\tOPA[%4i]=",
                        (pipe.cmd->src1.offset + pipe.cmd->src1.alpha * pipe.cmd->x +
                            pipe.cmd->src1.beta * pipe.cmd->y +
                            pipe.cmd->src1.gamma * pipe.cmd->z));
                if (vector_lane_id == 0) printf("\e[48;5;235m");
                if (vector_lane_id == 1) printf("\e[48;5;238m");
                printf_info("%9i (0x%08x)\t", int(pipe.opa), pipe.opa);
                if (vector_lane_id == 0) printf("\e[48;5;235m");
                if (vector_lane_id == 1) printf("\e[48;5;238m");

                if (is_src_chaining(pipe.cmd->src2))
                    printf((get_lane_from_src(pipe.cmd->src2).isLSLane()) ? " CH[  LS]="
                                                                          : " CH[  L%i]=",
                        get_lane_from_src(pipe.cmd->src2).vector_lane_id);
                else
                    printf_info("OPB[%4i]=",
                        (pipe.cmd->src2.offset + pipe.cmd->src2.alpha * pipe.cmd->x +
                            pipe.cmd->src2.beta * pipe.cmd->y +
                            pipe.cmd->src2.gamma * pipe.cmd->z));
                if (vector_lane_id == 0) printf("\e[48;5;235m");
                if (vector_lane_id == 1) printf("\e[48;5;238m");
                printf_info("%9i (0x%08x)\t", int(pipe.opb), pipe.opb);
                if (vector_lane_id == 0) printf("\e[48;5;235m");
                if (vector_lane_id == 1) printf("\e[48;5;238m");

                if (pipe.cmd->type == CommandVPRO::MACL || pipe.cmd->type == CommandVPRO::MACH) {
                    printf_info("OPC[%4i]=",
                        (pipe.cmd->src1.offset + pipe.cmd->src1.alpha * pipe.cmd->x +
                            pipe.cmd->src1.beta * pipe.cmd->y +
                            pipe.cmd->src1.gamma * pipe.cmd->z));
                    if (vector_lane_id == 0) printf("\e[48;5;235m");
                    if (vector_lane_id == 1) printf("\e[48;5;238m");
                    printf_info("%9i (0x%08x)\t", int(pipe.opc), pipe.opc);
                    if (vector_lane_id == 0) printf("\e[48;5;235m");
                    if (vector_lane_id == 1) printf("\e[48;5;238m");
                }

                printf_info(
                    "Result=%9i, Data=%9i (0x%06x)", int(pipe.pre_data), int(pipe.data), pipe.data);
                printf("\033[0m");
                if (pipe.move) printf(INVERTED);
                printf_info("\t=>RF%i[%4i] (move: %s)",
                    vector_lane_id,
                    pipe.rf_addr,
                    (pipe.move ? "true" : "false"));
                printf(RESET_COLOR "\n");
            }
        }
    }
    if (if_debug(DEBUG_LANE_STALLS) && false) {
        if (src_lane_stall || dst_lane_stall) {
            printf("Lane %2i Stall @Clock: %li -> %li: ",
                vector_lane_id,
                clock_cycle - 1,
                clock_cycle);
            if (src_lane_stall) printf_info("Chain-SRC[not rdy] ");
            if (dst_lane_stall) printf_info("Chain-DST[not read]");
            printf("\n");
            for (auto lane : vector_unit->getLanes()) {
                lane->print_pipeline(" >> ");
            }
            printf_warning("\n[press Enter to continue...]\n");
            getchar();
        }
    }

    // 30 due to possible mips overheads, etc. maybe another chain is active before, this can have maximal length of x_end*y_end
    constexpr int MAX_STALL_CYCLES_IN_ROW =
        30 + std::min(MAX_X_END * MAX_Y_END * MAX_Z_END * 2, 8192u);
    if (consecutive_DST_stall_counter >= MAX_STALL_CYCLES_IN_ROW ||
        consecutive_SRC_stall_counter >= MAX_STALL_CYCLES_IN_ROW ||
        consecutive_ADR_stall_counter >= MAX_STALL_CYCLES_IN_ROW) {
        printf_info(
            "\n\tToo Many Stall Cycles (> %i consecutive ones)!! \n", MAX_STALL_CYCLES_IN_ROW);
        printf_info(
            "\tCheck your CODE!\n\t-> Isolate this command and check if chaining works (correct "
            "src/dst). \n");
        printf_info(
            "\tIf it works and still error, this could be caused by scheduled old cmds (e.g. "
            "previous commands still chaining due wrong vector length)\n\n");
        if (isLSLane())
            printf_info("This is Lane[LS]\n");
        else
            printf_info("This is Lane[%i]\n", vector_lane_id);

        if (consecutive_DST_stall_counter >= MAX_STALL_CYCLES_IN_ROW) {
            printf_info("Current Pipeline: \n");
            print_pipeline();
            printf_info("Chaining Command: \n");
            (*pipeObj)[5 + pipeObj->pipelineALUDepth].cmd->print();
            printf("\n");
            printf_error(
                "Destination does not get read and causes stall! (Command %s in Pipeline Stage "
                "%i)\n",
                (*pipeObj)[5 + pipeObj->pipelineALUDepth].cmd->get_type().toStdString().c_str(),
                5 + pipeObj->pipelineALUDepth);
            getchar();
        }
        if (consecutive_SRC_stall_counter >= MAX_STALL_CYCLES_IN_ROW) {
            printf_info("Current Pipeline: \n\t");
            print_pipeline();
            printf_info("Chaining Command: \n\t");
            (*pipeObj)[chain_target_stage].cmd->print();
            printf("\n");
            printf_error(
                "Stall is caused by no Data fed on Source Read! (Command %s in Pipeline Stage "
                "%i)\n",
                (*pipeObj)[chain_target_stage].cmd->get_type().toStdString().c_str(),
                chain_target_stage);
            getchar();
        }
        if (consecutive_ADR_stall_counter >= MAX_STALL_CYCLES_IN_ROW) {
            printf_info("Current Pipeline: \n\t");
            print_pipeline();
            printf_info("Chaining Command: \n\t");
            (*pipeObj)[chain_target_stage].cmd->print();
            printf("\n");
            printf_error(
                "Stall is caused by no Data fed on Indirect Address Read! (Command %s in Pipeline "
                "Stage %i)\n",
                (*pipeObj)[chain_target_stage - 1].cmd->get_type().toStdString().c_str(),
                chain_target_stage - 1);
            getchar();
        }
    }
    if (checkForWARConflict) {  // analyse pipeline for WAR conflict
        const int i = chain_target_stage;
        if ((*pipeObj)[i].cmd->type != CommandVPRO::NONE) {
            QMap<int, QString> writtenRFs;
            for (int j = chain_target_stage + 1; j <= 5 + pipeObj->pipelineALUDepth; ++j) {
                if ((*pipeObj)[j].cmd->isWriteRF())
                    writtenRFs[(*pipeObj)[j].cmd->dst.offset +
                               (*pipeObj)[j].cmd->dst.alpha * (*pipeObj)[j].cmd->x +
                               (*pipeObj)[j].cmd->dst.beta * (*pipeObj)[j].cmd->y +
                               (*pipeObj)[i].cmd->dst.gamma * (*pipeObj)[i].cmd->z] =
                        (*pipeObj)[j].cmd->get_string();
            }
            if ((*pipeObj)[i].cmd->src1.sel == SRC_SEL_ADDR) {
                int rf_addr1 = (*pipeObj)[i].cmd->src1.offset +
                               (*pipeObj)[i].cmd->src1.alpha * (*pipeObj)[i].cmd->x +
                               (*pipeObj)[i].cmd->src1.beta * (*pipeObj)[i].cmd->y +
                               (*pipeObj)[i].cmd->src1.gamma * (*pipeObj)[i].cmd->z;
                if (writtenRFs.count(rf_addr1) > 0) {
                    printf_warning("Write to RF after Read (WAR conflict!?)\n");
                    printf_warning(
                        "Previous instruction \n\t(%s) \nwill write into RF[%i] after \n\t%s SRC1 "
                        "Reads it, but is scheduled before!\n",
                        writtenRFs[rf_addr1].toStdString().c_str(),
                        rf_addr1,
                        (*pipeObj)[i].cmd->get_string().toStdString().c_str());
                }
            }
            if ((*pipeObj)[i].cmd->src2.sel == SRC_SEL_ADDR) {
                int rf_addr2 = (*pipeObj)[i].cmd->src2.offset +
                               (*pipeObj)[i].cmd->src2.alpha * (*pipeObj)[i].cmd->x +
                               (*pipeObj)[i].cmd->src2.beta * (*pipeObj)[i].cmd->y +
                               (*pipeObj)[i].cmd->src2.gamma * (*pipeObj)[i].cmd->z;
                if (writtenRFs.count(rf_addr2) > 0) {
                    printf_warning("Write to RF after Read (WAR conflict!?)\n");
                    printf_warning(
                        "Previous instruction \n\t(%s) \nwill write into RF[%i] after \n\t%s SRC2 "
                        "Reads it, but is scheduled before!\n",
                        writtenRFs[rf_addr2].toStdString().c_str(),
                        rf_addr2,
                        (*pipeObj)[i].cmd->get_string().toStdString().c_str());
                }
            }
        }
    }
}

void VectorLane::check_stall_conditions_msg1() const {
    if (if_debug(DEBUG_LANE_STALLS)) {
        printf(ORANGE);
        printf("C%02iU%02i", vector_unit->cluster_id, vector_unit->vector_unit_id);
        printf((isLSLane()) ? "LS: " : "L%i: ", vector_lane_id);
    }
}

void VectorLane::check_stall_conditions_msg2(addr_field_t const& src) {
    if (if_debug(DEBUG_LANE_STALLS)) {
        printf(ORANGE);
        printf("C%02iU%02i", vector_unit->cluster_id, vector_unit->vector_unit_id);
        printf((isLSLane()) ? "LS: " : "L%i: ", vector_lane_id);
        printf("SET DST LANE READY IN ");
        printf("C%02iU%02i",
            get_lane_from_src(src).vector_unit->cluster_id,
            get_lane_from_src(src).vector_unit->vector_unit_id);
        printf((get_lane_from_src(src).isLSLane()) ? "LS: " : "L%i: ",
            get_lane_from_src(src).vector_lane_id);
    }
}

void VectorLane::get_operand_debug_chaining_msg(VectorLane const& src) const {
    if (if_debug(DEBUG_CHAINING) ||
        //        !get_lane_from_src(src)->pipeObj->isChaining() ||
        !src.is_lane_chain_fifo_ready()) {
        printf(ORANGE "C%02iU%02i", vector_unit->cluster_id, vector_unit->vector_unit_id);
        printf((isLSLane()) ? "LS " : "L%i ", vector_lane_id);
        printf("ACCESSES C%02iU%02i", src.vector_unit->cluster_id, src.vector_unit->vector_unit_id);
        printf((src.isLSLane()) ? "LS" : "L%i", src.vector_lane_id);

        //Dif Data
        //if(if_debug(DEBUG_CHAINING)) printf(" - DATA: %i (0x%X)\n", get_lane_from_src(src)->chain_data, get_lane_from_src(src)->chain_data);
        if (!src.pipeObj->isChaining()) printf_warning(" - NO CHAINING CMD IN OUT STAGE\n");
        if (!src.is_lane_chain_fifo_ready()) printf_warning(" - DST LANE NOT READY\n");
    }
}

VectorLane* VectorLane::who() {
    printf((isLSLane()) ? "LS" : "L%i", vector_lane_id);
    return this;
}

void VectorLane::whoami() const {
    printf((isLSLane()) ? "LS" : "L%i", vector_lane_id);
}

void VectorLane::whoami(VectorLane const* v) const {
    printf((v->isLSLane()) ? "LS" : "L%i", v->vector_lane_id);
}

void VectorLane::whoami(addr_field_t const& src) {
    printf(
        (get_lane_from_src(src).isLSLane()) ? "LS" : "L%i", get_lane_from_src(src).vector_lane_id);
}

void VectorLane::print_fifo() const {
    if (!if_debug(DEBUG_FIFO_MSG)) {
        return;
    }
    whoami();
    if (fifo.if_in()) {
        auto a = fifo.in();
        printf(" FIFO IN <(%d %d %d)>", std::get<0>(a), std::get<1>(a), std::get<2>(a));
    } else
        printf(" FIFO IN <NONE>");

    printf(" OUT ");
    if (fifo.if_out()) {
        auto b = fifo.out();
        printf("<(%d %d %d)>", std::get<0>(b), std::get<1>(b), std::get<2>(b));
    } else
        printf("<NONE>");

    printf(" D_OUT ");
    if (fifo.wasRead()) {
        auto c = fifo.wasData();
        printf("<(%d %d %d)>", std::get<0>(c), std::get<1>(c), std::get<2>(c));
    } else
        printf("<NONE>");

    printf("\n--CONTENT: ");
    auto print_elem = [](auto& d) {
        printf("<(%d %d %d)> ", std::get<0>(d), std::get<1>(d), std::get<2>(d));
    };
    fifo.list(print_elem);
    printf("\n");
}

void VectorLane::check_stall_conditions_msg(const int pipeline_src_read_stage) {  //kombinatorische

    check_stall_conditions_msg1();
    if (!if_debug(DEBUG_LANE_STALLS)) {
        return;
    }
    addr_field_t src1 = (*pipeObj)[pipeline_src_read_stage].cmd->src1;
    printf("SRC1: ");

    if (is_src_chaining(src1)) {
        auto& lane_src1 = get_lane_from_src(src1);

        if (lane_src1.fifo._is_empty()) {
            //src_lane_stall = true;
            printf("<FIFO>IS STALL ");
        } else {
            printf("NO STALL ");
        }
    } else
        printf("NO CHAIN ");

    addr_field_t src2 = (*pipeObj)[pipeline_src_read_stage].cmd->src2;
    printf("SRC2: ");
    if (is_src_chaining(src2)) {
        auto& lane_src2 = get_lane_from_src(src2);
        if (lane_src2.fifo._is_empty()) {
            //src_lane_stall = true;
            printf("<FIFO>IS STALL ");
        } else {
            printf("NO STALL ");
        }
    } else
        printf("NO CHAIN ");

    if (pipeObj->isChaining()) {
        if (!fifo._is_fillable()) {
            //dst_lane_stall = true;
            printf("DST_STALL: DST LANE READY WAS NOT SET");
        } else
            printf("DST_LANE_READY: CHAINING DATA");
    } else
        printf("NO CHAINING OUT");

    printf("\n");
}

void print_hex(uint32_t value, int char_cnt) {
    printf("0x");
    for (int i = 0; i < char_cnt; ++i) {
        printf("%1x", (value >> (4 * (char_cnt - 1 - i))) & 0xf);
    }
}

/**
 * Sim function to dump content of this lanes whole register file
 */
void VectorLane::dumpRegisterFile(std::string prefix) {
    auto rf_data = regFile.getregister();
    auto rf_flag_0 = regFile.getzeroflag();
    auto rf_flag_1 = regFile.getnegativeflag();
    printf("%s", prefix.c_str());
    printf("DUMP Register File (Vector Lane %i):\n", vector_lane_id);
    printf(LGREEN);
    int printLast = 0;
    for (int r = 0; r < VPRO_CFG::RF_SIZE * 3; r += 16 * 3) {  // all elements in 16 element blocks
        bool hasData = false;
        for (int s = 0; s <= 15 * 3; s += 3) {  // each of the 16 blocks
            auto data = ((uint32_t)(rf_data[r + s]) + (uint32_t)(rf_data[r + s + 1] << 8) +
                         (uint32_t)(rf_data[r + s + 2] << 16));
            hasData |= (data != 0);
        }
        if (hasData) {
            printLast = std::min(printLast + 1, 1);
            printf(LGREEN);
            printf("%s", prefix.c_str());
            uint address = (r / 3) & (~(16 - 1));
            printf("$%03x:  ", address);
            for (int s = 0; s <= 15 * 3; s += 3) {  // each of the 16 blocks
                auto data = ((uint32_t)(rf_data[r + s]) + (uint32_t)(rf_data[r + s + 1] << 8) +
                             (uint32_t)(rf_data[r + s + 2] << 16));
                if ((uint32_t(data >> 23u) & 1) == 1) data |= 0xFF000000;

                if (data == 0) {
                    printf(LIGHT);
                    print_hex(data, 6);
                    printf(NORMAL_);
                } else {
                    printf(LGREEN);
                    print_hex(data, 6);
                }
                if (if_debug(DEBUG_DUMP_FLAGS)) {
                    auto neg = rf_flag_1[(r + s) / 3];
                    auto zero = rf_flag_0[(r + s) / 3];
                    printf("[%s%s]", neg ? "-" : "+", zero ? "z" : "d");
                }
                printf(" ");
            }
            printf("\n");
        } else {
            if (printLast == 0) {
                printf("%s", prefix.c_str());
                printf("...\n");
            }
            printLast = std::max(printLast - 1, -1);
        }
    }
    printf(RESET_COLOR);
}

}  // namespace Unit
