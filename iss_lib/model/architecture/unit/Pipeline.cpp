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
 * @file Pipeline.cpp
 * @author Alexander Köhne, Sven Gesper
 * @date 2021
 *
 * Outsourced pipeline for vector lanes
 */

#include "Pipeline.h"
#include "../Cluster.h"
#include "VectorUnit.h"

#include <vector>
using std::vector;

#include <memory>
#include <tuple>
using std::get;

#include "../../../core_wrapper.h"
#include "../../../simulator/ISS.h"
#include "../../../simulator/helper/debugHelper.h"
#include "../../../simulator/helper/typeConversion.h"

namespace Unit {

bool chains_from_src(CommandVPRO cmd, uint32_t src_sel) {
    return (cmd.src1.sel == src_sel || cmd.src2.sel == src_sel || cmd.dst.sel == src_sel);
}

void update_offsets(
    VectorLane& vl, std::shared_ptr<CommandVPRO> cmd, uint32_t src_sel, VectorLane& chaining_src) {
    if (src_sel != SRC_SEL_INDIRECT_LEFT && src_sel != SRC_SEL_INDIRECT_RIGHT &&
        src_sel != SRC_SEL_INDIRECT_LS) {
        return;  // no indirect addressing, don't update offsets
    }
    if (chains_from_src(*cmd, src_sel)) {
        uint32_t offset =
            get<0>(chaining_src.fifo.pop());  // read from FIFO of left neighboring lane
        offset = *__32to10(offset);
        cmd->src1.offset = (cmd->src1.sel == src_sel) ? offset : cmd->src1.offset;
        cmd->src2.offset = (cmd->src2.sel == src_sel) ? offset : cmd->src2.offset;
        cmd->dst.offset = (cmd->dst.sel == src_sel) ? offset : cmd->dst.offset;
    }
}

PipeObject::PipeObject(int size, int pipelineALUDepth)
    : accu(0),
      data(vector<PipelineDate>(size)),
      pipelineALUDepth(pipelineALUDepth)

{}

const PipelineDate& PipeObject::operator[](int index) const {
    return data[index];
}

PipelineDate& PipeObject::operator[](int index) {
    return data[index];
}

void PipeObject::resetAccu(int64_t value) {
    //ALU
    accu = value;
}
void PipeObject::resetMinMax(uint32_t value) {
    minmax_value = value;
}

void PipeObject::tick_pipeline(VectorLane& vl, int from, int until) {
    for (int stage = from; stage <= until; stage++) {
        // Addressing Units (Stage 0..3) for address calc
        // Local Memory Address Computation, base address (stage 3),
        // Chaining Write Enable (stage 4)
        // ALU Input Operands (stage 4) e.g. chain, imm
        // ALU Input Operands (stage 5) (assign to alu) e.g., src1_rdata
        // Local Memory Interface (stage 5) e.g. is_load/store, addr, wdata
        // ALU (stages 5..8)
        // Condition Check (stage 6) e.g., z_v, n_v
        // Data Write-Back Buffer (stage 8) e.g., rf_write_data <= lm_data/alu_result  chain_data_o <= alu_result
        // Chaining Output (stage 9)
        //        chain_sync_o <= enable(4) and (not enable(5)) and cmd_reg_pipe(4)(cmd_is_chain_c); -- 3 cycles latency between sync and actual data!!
        // Register File (read: stage 3..5; write: stage 9)

        PipelineDate& pipe = data[stage];
        if (pipe.cmd->type == CommandVPRO::NONE) continue;

        if (stage == 0) {  // calc addresses
            // result address in rf for regular instructions
            pipe.rf_addr = pipe.cmd->dst.alpha * pipe.cmd->x + pipe.cmd->dst.beta * pipe.cmd->y +
                           pipe.cmd->dst.gamma * pipe.cmd->z;
        } else if (stage == 3) {  // indirect addressing (chained offsets)

            check_indirect_addressing(pipe.cmd);
            update_offsets(vl, pipe.cmd, SRC_SEL_INDIRECT_LS, vl.getLSLane());
            update_offsets(vl, pipe.cmd, SRC_SEL_INDIRECT_LEFT, vl.getLeftNeighbor());
            update_offsets(vl, pipe.cmd, SRC_SEL_INDIRECT_RIGHT, vl.getRightNeighbor());

            // add offset to destination address
            pipe.rf_addr += pipe.cmd->dst.offset;
        } else if (stage == 4) {  // read chain input, read rf <?>
            auto data_a = vl.get_operand(pipe.cmd->src1, pipe.cmd->x, pipe.cmd->y, pipe.cmd->z);
            auto data_b = vl.get_operand(pipe.cmd->src2, pipe.cmd->x, pipe.cmd->y, pipe.cmd->z);
            if (pipe.cmd->type == CommandVPRO::MACL || pipe.cmd->type == CommandVPRO::MACH) {
                // for MAC initialization
                addr_field_t src1_addr = pipe.cmd->src1;
                if (vl.architecture_state->MAC_ACCU_INIT_SOURCE == VPRO::MAC_INIT_SOURCE::ADDR) {
                    src1_addr.sel = SRC_SEL_ADDR;
                    pipe.opc =
                        get<0>(vl.get_operand(src1_addr, pipe.cmd->x, pipe.cmd->y, pipe.cmd->z));
                } else if (vl.architecture_state->MAC_ACCU_INIT_SOURCE ==
                           VPRO::MAC_INIT_SOURCE::IMM) {
                    src1_addr.sel = SRC_SEL_IMM;
                    pipe.opc =
                        get<0>(vl.get_operand(src1_addr, pipe.cmd->x, pipe.cmd->y, pipe.cmd->z));
                }
                // else ZERO (default of opc)
                pipe.opc = *__24to32signed(pipe.opc);
            }
            pipe.opa = get<0>(data_a);
            pipe.opa = *__24to32signed(pipe.opa);
            pipe.opb = get<0>(data_b);

            // only 18 bit for SRC2 if mac operation
            if (pipe.cmd->type == CommandVPRO::MULL || pipe.cmd->type == CommandVPRO::MULH ||
                pipe.cmd->type == CommandVPRO::MACL || pipe.cmd->type == CommandVPRO::MACH ||
                pipe.cmd->type == CommandVPRO::MACL_PRE ||
                pipe.cmd->type == CommandVPRO::MACH_PRE ||
                pipe.cmd->type == CommandVPRO::MULL_NEG ||
                pipe.cmd->type == CommandVPRO::MULH_NEG ||
                pipe.cmd->type == CommandVPRO::MULL_POS ||
                pipe.cmd->type == CommandVPRO::MULH_POS) {
                auto tmp = *__24to32signed(pipe.opb);
                pipe.opb = *__18to32signed(pipe.opb);
                if (tmp != pipe.opb) {
                    pipe.cmd->print();
                    printf_warning(
                        "MUL* only takes 18-bit for second operand (SRC2)/OPB. Loaded Data got "
                        "cut... %i (soll) != %i (18-bit cut)\n",
                        tmp,
                        pipe.opb);
                }

            } else {
                pipe.opb = *__24to32signed(pipe.opb);
            }

            // update move flag to print correctly in execute stage // differs from HW!
            switch (pipe.cmd->type) {
                case CommandVPRO::MV_ZE:
                    pipe.move = get<1>(data_a);  // z ==1
                    break;
                case CommandVPRO::MV_NZ:
                    pipe.move = !(get<1>(data_a));  // z == 0
                    break;
                case CommandVPRO::MV_MI:
                case CommandVPRO::MULL_NEG:
                case CommandVPRO::MULH_NEG:
                case CommandVPRO::SHIFT_AR_NEG:
                    pipe.move = get<2>(data_a);  // n == 1
                    break;
                case CommandVPRO::MV_PL:
                case CommandVPRO::MULL_POS:
                case CommandVPRO::MULH_POS:
                case CommandVPRO::SHIFT_AR_POS:
                    pipe.move = !(get<2>(data_a));  // n == 0
                    break;
                default:
                    pipe.move = true;
                    break;
            }
        } else if (stage == 5) {  // execute alu, write lm
            if ((vl.architecture_state->MAC_ACCU_RESET_MODE == VPRO::MAC_RESET_MODE::ONCE &&
                    pipe.cmd->x == 0 && pipe.cmd->y == 0 && pipe.cmd->z == 0) ||
                (vl.architecture_state->MAC_ACCU_RESET_MODE == VPRO::MAC_RESET_MODE::Z_INCREMENT &&
                    pipe.cmd->x == 0 && pipe.cmd->y == 0) ||
                (vl.architecture_state->MAC_ACCU_RESET_MODE == VPRO::MAC_RESET_MODE::Y_INCREMENT &&
                    pipe.cmd->x == 0) ||
                (vl.architecture_state->MAC_ACCU_RESET_MODE == VPRO::MAC_RESET_MODE::X_INCREMENT)) {
                if (pipe.cmd->type == CommandVPRO::MACL_PRE ||
                    pipe.cmd->type == CommandVPRO::MACH_PRE) {
                    resetAccu();  // accu = 0
                } else if (pipe.cmd->type == CommandVPRO::MACL ||
                           pipe.cmd->type == CommandVPRO::MACH) {
                    if (vl.architecture_state->MAC_ACCU_INIT_SOURCE ==
                            VPRO::MAC_INIT_SOURCE::ADDR ||
                        vl.architecture_state->MAC_ACCU_INIT_SOURCE == VPRO::MAC_INIT_SOURCE::IMM ||
                        vl.architecture_state->MAC_ACCU_INIT_SOURCE ==
                            VPRO::MAC_INIT_SOURCE::ZERO) {
                        if (pipe.cmd->type == CommandVPRO::MACH) {
                            //resetAccu(pipe.opc << vl.architecture_state->ACCU_MAC_HIGH_BIT_SHIFT);
                            resetAccu(int64_t(pipe.opc)
                                      << vl.architecture_state->ACCU_MAC_HIGH_BIT_SHIFT);
                        } else {
                            resetAccu(pipe.opc);
                        }
                    }
                }
                // TODO: ..._PRE instruction needed? -> yes for non ls source (reset not possible)
            }
            if (pipe.cmd->x == 0 && pipe.cmd->y == 0 &&
                (pipe.cmd->type == CommandVPRO::MIN_VECTOR ||
                    pipe.cmd->type == CommandVPRO::MAX_VECTOR)) {
                minmax_value = pipe.opa;
                minmax_index =
                    pipe.cmd->src1.alpha * pipe.cmd->x + pipe.cmd->src1.beta * pipe.cmd->y;
            }
            execute_cmd(vl, pipe);
            if (pipe.cmd->type == CommandVPRO::MULL_NEG ||
                pipe.cmd->type == CommandVPRO::MULH_NEG ||
                pipe.cmd->type == CommandVPRO::SHIFT_AR_NEG ||
                pipe.cmd->type == CommandVPRO::MULL_POS ||
                pipe.cmd->type == CommandVPRO::MULH_POS ||
                pipe.cmd->type == CommandVPRO::SHIFT_AR_POS)
                if (!pipe.move) {
                    pipe.pre_data = pipe.opa;
                    pipe.move =
                        true;  // only mv_x will not write RF, others do always but conditional take Operand a if condition fails
                }

            pipe.flag[0] = __is_zero(pipe.pre_data);
            pipe.flag[1] = __is_negative(pipe.pre_data);
        }

        if (stage == 5 + pipelineALUDepth + 1) {  //usually stage: 9
            if (pipe.cmd->isWriteRF()) {
                if (pipe.move) {
                    vl.regFile.set_rf_data_nxt(pipe.rf_addr, pipe.data);
                    // Update FLAGs (two steps... to allow chaining)
                    if (pipe.cmd->flag_update) {
                        vl.regFile.set_rf_flag_nxt(pipe.rf_addr, 0, pipe.flag[0]);
                        vl.regFile.set_rf_flag_nxt(pipe.rf_addr, 1, pipe.flag[1]);
                    }
                }
            }

            if (pipe.cmd->is_chain) {
                //printf("FIFO PUSH Stage: %d", stage);
                vl.fifo.push(std::tie(pipe.data, pipe.flag[0], pipe.flag[1]));
            }
        }  // stage wb
    }      // stages
}

/**
     * this is the ALU
     * the execution of the command depends on function-selection
     * @param Pipeline item to be processed
     */
void PipeObject::execute_cmd(VectorLane& vl, PipelineDate& pipe) {
    uint32_t& res = pipe.res;
    pipe.pre_data = 0;
    uint64_t res_mul;
    float res_div;  //0..22|23..30|31 == Mantisse|Exponent|Vorzeichen
    uint32_t rf_addr_src1;

    switch (pipe.cmd->type) {
        case CommandVPRO::ADD:
            res = pipe.opa + pipe.opb;
            pipe.pre_data = res;
            break;
        case CommandVPRO::SUB:
            res = pipe.opb - pipe.opa;
            pipe.pre_data = res;
            break;
        case CommandVPRO::MULL_NEG:
        case CommandVPRO::MULL_POS:
        case CommandVPRO::
            MULL:  // (!) OPB is 18-bit long. (A + B inside uint32 with sign extension/int32)
            res_mul = (uint64_t)(((int64_t)(int32_t)pipe.opa) * ((int64_t)(int32_t)pipe.opb));
            accu = res_mul;
            res = (uint32_t)res_mul;
            pipe.pre_data = res;
            break;
        case CommandVPRO::MULH_NEG:
        case CommandVPRO::MULH_POS:
        case CommandVPRO::
            MULH:  // (!) OPB is 18-bit long. (A + B inside uint32 with sign extension/int32)
            res_mul = (uint64_t)(((int64_t)(int32_t)pipe.opa) * ((int64_t)(int32_t)pipe.opb));
            accu = res_mul;
            res = (uint32_t)(res_mul >> (vl.architecture_state->ACCU_MUL_HIGH_BIT_SHIFT));
            pipe.pre_data = res;
            break;
        case CommandVPRO::DIVL:  // only simulator!
            res_div = ((float)pipe.opa) / ((float)pipe.opb);
            for (int i = 0; i < 16; i++) {
                res_div *= 2;
            }
            res = (uint32_t)res_div;
            res = (((1 << 16) - 1) & res);
            pipe.pre_data = *__32to24(res);
            break;
        case CommandVPRO::DIVH:  // only simulator!
            res = (uint32_t)((float)pipe.opa) / ((float)pipe.opb);
            pipe.pre_data = *__32to24(res);
            break;
        case CommandVPRO::
            MACL:  // (!) OPB is 18-bit long. (A + B inside uint32 with sign extension/int32)
            res_mul = (uint64_t)(((int64_t)(int32_t)pipe.opa) * ((int64_t)(int32_t)pipe.opb));
            accu += (res_mul & 0xffffffffffffLL);
            res = (uint32_t)(accu);
            pipe.pre_data = res;
            break;
        case CommandVPRO::
            MACH:  // (!) OPB is 18-bit long. (A + B inside uint32 with sign extension/int32)
            res_mul = (uint64_t)(((int64_t)(int32_t)pipe.opa) * ((int64_t)(int32_t)pipe.opb));
            accu += (res_mul & 0xffffffffffffLL);  // 48-bit
            res = (uint32_t)(accu >> (vl.architecture_state->ACCU_MAC_HIGH_BIT_SHIFT));
            pipe.pre_data = res;
            break;
        case CommandVPRO::
            MACL_PRE:  // (!) OPB is 18-bit long. (A + B inside uint32 with sign extension/int32)
            res_mul = (uint64_t)(((int64_t)(int32_t)pipe.opa) * ((int64_t)(int32_t)pipe.opb));
            accu += (res_mul & 0xffffffffffffLL);  // 48-bit
            res = (uint32_t)(accu);
            pipe.pre_data = res;
            break;
        case CommandVPRO::
            MACH_PRE:  // (!) OPB is 18-bit long. (A + B inside uint32 with sign extension/int32)
            res_mul = (uint64_t)(((int64_t)(int32_t)pipe.opa) * ((int64_t)(int32_t)pipe.opb));
            accu += (res_mul & 0xffffffffffffLL);  // 48-bit
            res = (uint32_t)(accu >> (vl.architecture_state->ACCU_MAC_HIGH_BIT_SHIFT));
            pipe.pre_data = res;
            break;
        case CommandVPRO::XOR:
            res = pipe.opa ^ pipe.opb;
            pipe.pre_data = res;
            break;
        case CommandVPRO::XNOR:
            res = ~(pipe.opa ^ pipe.opb);
            pipe.pre_data = res;
            break;
        case CommandVPRO::AND:
            res = pipe.opa & pipe.opb;
            pipe.pre_data = res;
            break;
        case CommandVPRO::NAND:
            res = ~(pipe.opa & pipe.opb);
            pipe.pre_data = res;
            break;
        case CommandVPRO::OR:
            res = pipe.opa | pipe.opb;
            pipe.pre_data = res;
            break;
        case CommandVPRO::NOR:
            res = ~(pipe.opa | pipe.opb);
            pipe.pre_data = res;
            break;
        case CommandVPRO::SHIFT_LL:
            if constexpr (VPRO_CFG::SIM::STRICT) {
                printf_error(
                    "SHIFT_LL unsupported by hardware, exiting due to STRICT simulation mode.");
                std::exit(EXIT_FAILURE);
            }
            res = (*__32to24(pipe.opa) << uint(pipe.opb & uint(0x1f)));
            pipe.pre_data = res;
            break;
        case CommandVPRO::SHIFT_LR:
            res = (*__32to24(pipe.opa) >> uint(pipe.opb & uint(0x1f)));
            pipe.pre_data = res;
            break;
        case CommandVPRO::SHIFT_AR:
        case CommandVPRO::SHIFT_AR_NEG:
        case CommandVPRO::SHIFT_AR_POS:
            res = (uint32_t)(((int32_t)pipe.opa) >> uint(pipe.opb & uint(0x1f)));
            pipe.pre_data = res;
            break;
        case CommandVPRO::ABS:
            res = (pipe.opa & 0x80000000) ? -pipe.opa : pipe.opa;
            pipe.pre_data = *__32to24(res);
            break;
        case CommandVPRO::MIN:
            res = ((int32_t)pipe.opa < (int32_t)pipe.opb) ? pipe.opa : pipe.opb;
            pipe.pre_data = res;
            break;
        case CommandVPRO::MIN_VECTOR:
            // DST(komplex-addr / 1 register) SRC1(vector - komplex addr) SRC2(imm; @bit0: index?)
            // SRC2(1, -, -) == return index
            // SRC2(0, -, -) == return value
            if ((int32_t)pipe.opa < (int32_t)minmax_value) {
                minmax_value = pipe.opa;
                rf_addr_src1 = pipe.cmd->src2.alpha * pipe.cmd->x +
                               pipe.cmd->src2.beta * pipe.cmd->y +
                               pipe.cmd->src2.gamma * pipe.cmd->z;
                minmax_index = rf_addr_src1;
            }
            if ((pipe.cmd->src2.offset & 0b1u) == 1u)
                res = minmax_index;
            else
                res = minmax_value;
            pipe.pre_data = res;
            break;
        case CommandVPRO::MAX:
            res = ((int32_t)pipe.opa > (int32_t)pipe.opb) ? pipe.opa : pipe.opb;
            pipe.pre_data = res;
            break;
        case CommandVPRO::MAX_VECTOR:
            // DST(komplex-addr / 1 register) SRC1(vector - komplex addr) SRC2(imm; @bit0: index?)
            // SRC2(1, -, -) == return index
            // SRC2(0, -, -) == return value
            if ((int32_t)pipe.opa > (int32_t)minmax_value) {
                minmax_value = pipe.opa;
                rf_addr_src1 = pipe.cmd->src1.alpha * pipe.cmd->x +
                               pipe.cmd->src1.beta * pipe.cmd->y +
                               pipe.cmd->src1.gamma * pipe.cmd->z;
                minmax_index = rf_addr_src1;
            }
            if ((pipe.cmd->src2.offset & 0b1u) == 1u)
                res = minmax_index;
            else
                res = minmax_value;
            pipe.pre_data = res;
            break;
        case CommandVPRO::BIT_REVERSAL:
            res = 0;
            if (pipe.opb == 0)  // default if 0 bits should be reversed
                pipe.opb = 24;
            for (uint i = 0u; i < pipe.opb; i++)
                res |= ((pipe.opa >> i) & 0b1u) << (pipe.opb - 1 - i);
            pipe.pre_data = res;
            break;
        case CommandVPRO::MV_ZE:
        case CommandVPRO::MV_NZ:
        case CommandVPRO::MV_MI:
        case CommandVPRO::MV_PL:
            pipe.pre_data = *__32to24(pipe.opb);
            break;
        case CommandVPRO::NONE:
        case CommandVPRO::WAIT_BUSY:
        case CommandVPRO::PIPELINE_WAIT:
            break;
        case CommandVPRO::LOAD:
        case CommandVPRO::LOADS:
        case CommandVPRO::LOADB:
        case CommandVPRO::LOADBS:
        case CommandVPRO::STORE:
            printf_error(
                "Memory instruction in Lanes ALU detected! <should not happen because Pipeline "
                "handles this earlier>!\n");
            break;
        case CommandVPRO::LOOP_START:
        case CommandVPRO::LOOP_END:
        case CommandVPRO::LOOP_MASK:
            printf_error(
                "LOOP instruction in Lane detected! <should not happen because simulator handles "
                "this earlier; in the vector unit>!\n");
            break;
        default:
            printf_error("Lane Command Execution: Error in Command TYPE: ");
            print_cmd(pipe.cmd.get());
            printf("\n");
            break;
    }

    // convert to 24 bit (fill 1 if signed...)
    pipe.pre_data = *__24to32signed(pipe.pre_data);
}

/**
 * clock tick to process lanes command/pipeline
 */
void LSPipeObject::tick_pipeline(VectorLane& vl, int from, int until) {
    for (int stage = from; stage <= until; stage++) {
        // Addressing Units (Stage 0..3) for address calc
        // Local Memory Address Computation, base address (stage 3),
        // Chaining Write Enable (stage 4)
        // ALU Input Operands (stage 4) e.g. chain, imm
        // ALU Input Operands (stage 5) (assign to alu) e.g., src1_rdata
        // Local Memory Interface (stage 5) e.g. is_load/store, addr, wdata
        // ALU (stages 5..8)
        // Condition Check (stage 6) e.g., z_v, n_v
        // Data Write-Back Buffer (stage 8) e.g., rf_write_data <= lm_data/alu_result  chain_data_o <= alu_result
        // Chaining Output (stage 9)
        //        chain_sync_o <= enable(4) and (not enable(5)) and cmd_reg_pipe(4)(cmd_is_chain_c); -- 3 cycles latency between sync and actual data!!
        // Register File (read: stage 3..5; write: stage 9)

        PipelineDate& pipe = data[stage];
        if (pipe.cmd->type == CommandVPRO::NONE) continue;

        if (pipe.cmd->type != CommandVPRO::NONE && !pipe.cmd->isLS())
            printf_warning("LS Pipeline Tick got a NON-LS Cmd!");

        if (stage == 0) {  // calc addresses
            // result address in rf for regular instructions
            // for LM
            /**
             * SRC1 is complex addr
             */
            pipe.lm_addr = pipe.cmd->src1.alpha * pipe.cmd->x + pipe.cmd->src1.beta * pipe.cmd->y +
                           pipe.cmd->src1.gamma * pipe.cmd->z;

            if (pipe.cmd->type == CommandVPRO::STORE_SHIFT_LEFT ||
                pipe.cmd->type == CommandVPRO::STORE_SHIFT_RIGHT) {
                if (pipe.cmd->src2.sel != SRC_SEL_IMM)
                    printf_error("Shift in Store; only by immediate!\n");
                pipe.opb =
                    get<0>(vl.get_operand(pipe.cmd->src2, pipe.cmd->x, pipe.cmd->y, pipe.cmd->z));
                pipe.opb = *__24to32signed(pipe.opb);
            }
            if (pipe.cmd->type == CommandVPRO::LOAD_REVERSE ||
                pipe.cmd->type == CommandVPRO::STORE_REVERSE) {
                // TODO: z always added?
                auto imm = pipe.cmd->dst.getImm();
                if (imm == 0b00)
                    pipe.lm_addr = pipe.cmd->src1.alpha * pipe.cmd->x +
                                   pipe.cmd->src1.beta * pipe.cmd->y +
                                   pipe.cmd->src1.gamma * pipe.cmd->z;
                if (imm == 0b01)
                    pipe.lm_addr = pipe.cmd->src1.alpha * pipe.cmd->x -
                                   pipe.cmd->src1.beta * pipe.cmd->y +
                                   pipe.cmd->src1.gamma * pipe.cmd->z;
                if (imm == 0b10)
                    pipe.lm_addr = -pipe.cmd->src1.alpha * pipe.cmd->x +
                                   pipe.cmd->src1.beta * pipe.cmd->y +
                                   pipe.cmd->src1.gamma * pipe.cmd->z;
                if (imm == 0b11)
                    pipe.lm_addr = -pipe.cmd->src1.alpha * pipe.cmd->x -
                                   pipe.cmd->src1.beta * pipe.cmd->y +
                                   pipe.cmd->src1.gamma * pipe.cmd->z;
            }
        } else if (stage == 3) {  // indirect addressing (chained offsets)
            check_indirect_addressing(pipe.cmd);

            if (pipe.cmd->src1.sel == SRC_SEL_INDIRECT_RIGHT) {
                int i = 0;
            }
            update_offsets(vl, pipe.cmd, SRC_SEL_INDIRECT_LEFT, vl.getLeftNeighbor());
            update_offsets(vl, pipe.cmd, SRC_SEL_INDIRECT_RIGHT, vl.getRightNeighbor());

            pipe.lm_addr += pipe.cmd->src1.offset;
        }

        else if (stage == 4) {  // read chain input, read rf <?>
            /**
                 * DST encodes Data
                 */
            if (pipe.cmd->type == CommandVPRO::STORE ||
                pipe.cmd->type == CommandVPRO::STORE_SHIFT_LEFT ||
                pipe.cmd->type == CommandVPRO::STORE_SHIFT_RIGHT ||
                pipe.cmd->type == CommandVPRO::STORE_REVERSE) {
                pipe.pre_data =
                    get<0>(vl.get_operand(pipe.cmd->dst, pipe.cmd->x, pipe.cmd->y, pipe.cmd->z));
                pipe.pre_data = *__24to32signed(pipe.pre_data);
                pipe.pre_data = *__16to24(pipe.pre_data);
            }
            if (pipe.cmd->type ==
                CommandVPRO::STORE_SHIFT_LEFT) {  // MUX of different shifted datas
                pipe.pre_data = uint32_t(int32_t(pipe.pre_data) << pipe.opb);
                pipe.pre_data = *__24to32signed(pipe.pre_data);
                pipe.pre_data = *__16to24(pipe.pre_data);
            }
            if (pipe.cmd->type == CommandVPRO::STORE_SHIFT_RIGHT) {
                pipe.pre_data = uint32_t(int32_t(pipe.pre_data) >> pipe.opb);
                pipe.pre_data = *__24to32signed(pipe.pre_data);
                pipe.pre_data = *__16to24(pipe.pre_data);
            }

            // LOAD or Write: SRC1: Chain from other __LS__
            if (pipe.cmd->dst.chain_ls) {
                if (pipe.cmd->dst.chain_left) {
                    pipe.pre_data =
                        std::get<0>(vl.vector_unit->getLeftNeighbor()->getLSLane().fifo.pop());
                } else if (pipe.cmd->dst.chain_right) {
                    pipe.pre_data =
                        std::get<0>(vl.vector_unit->getRightNeighbor()->getLSLane().fifo.pop());
                } else
                    printf_error(
                        "Chain from LS LOAD stage 4 gets data but no direction specified!");
            }

            /**
             * SRC2 Imm (LM Addr)
             */
            // add immediate from SRC2 or chained offset    -> TODO: implement in hardware
            switch (pipe.cmd->src2.sel) {
                case SRC_SEL_IMM:
                    pipe.lm_addr += get<0>(
                        vl.get_operand(pipe.cmd->src2, pipe.cmd->x, pipe.cmd->y, pipe.cmd->z));
                    break;
                case SRC_SEL_LEFT:
                case SRC_SEL_RIGHT:
                    if (pipe.cmd->isLoad()) {
                        pipe.lm_addr += get<0>(
                            vl.get_operand(pipe.cmd->src2, pipe.cmd->x, pipe.cmd->y, pipe.cmd->z));
                    } else {
                        printf_error("Cannot chain offset for store instruction");
                    }
                    break;
                default:
                    printf_error("Invalid operand sel for LS Instruction");
            }
        } else if (stage == 5) {  // execute alu, write lm
            if (pipe.cmd->isWriteLM()) {
                // STORE: SRC1: Addr, Src2: Imm (or chained indirect offset)
                pipe.data = pipe.pre_data;
            } else {
                // LOAD
                if (!pipe.cmd->dst.chain_ls) {  // no LS -> LS chain    // TODO: Load of IMMEDIATE?
                    switch (pipe.cmd->type) {
                        case CommandVPRO::LOAD:
                        case CommandVPRO::LOADS:
                        case CommandVPRO::LOADS_SHIFT_LEFT:
                        case CommandVPRO::LOADS_SHIFT_RIGHT:
                        case CommandVPRO::LOADB:
                        case CommandVPRO::LOADBS:
                        case CommandVPRO::LOAD_REVERSE:
                            pipe.pre_data = vl.vector_unit->getLocalMemoryData(pipe.lm_addr);
                        default:
                            break;
                    }
                } else {
                    // pre_data already received (chain from LS)
                }
            }

            switch (pipe.cmd->type) {
                case CommandVPRO::LOAD:
                    pipe.pre_data = *__16to24(pipe.pre_data);
                    pipe.pre_data = *__24to32signed(pipe.pre_data);
                    break;
                case CommandVPRO::LOADS:
                    pipe.pre_data = *__16to24signed(pipe.pre_data);
                    pipe.pre_data = *__24to32signed(pipe.pre_data);
                    break;
                case CommandVPRO::LOADS_SHIFT_LEFT:  //data on stage 9
                    pipe.pre_data = *__16to24signed(pipe.pre_data);
                    pipe.pre_data = *__24to32signed(pipe.pre_data);
                    //stage 7
                    pipe.pre_data = int32_t(pipe.pre_data) << pipe.cmd->dst.offset;
                    pipe.pre_data = *__24to32signed(pipe.pre_data);
                    break;
                case CommandVPRO::LOADS_SHIFT_RIGHT:  //data on stage 9
                    pipe.pre_data = *__16to24signed(pipe.pre_data);
                    pipe.pre_data = *__24to32signed(pipe.pre_data);
                    //stage 7
                    pipe.pre_data = int32_t(pipe.pre_data) >> pipe.cmd->dst.offset;
                    pipe.pre_data = *__24to32signed(pipe.pre_data);
                    break;
                case CommandVPRO::LOADB:
                    pipe.pre_data = *__8to24(pipe.pre_data);
                    pipe.pre_data = *__24to32signed(pipe.pre_data);
                    break;
                case CommandVPRO::LOADBS:
                    pipe.pre_data = *__8to24signed(pipe.pre_data);
                    pipe.pre_data = *__24to32signed(pipe.pre_data);
                    break;
                case CommandVPRO::LOAD_REVERSE:
                    pipe.pre_data = *__8to24signed(pipe.pre_data);
                    pipe.pre_data = *__24to32signed(pipe.pre_data);
                    break;

                case CommandVPRO::STORE:
                    vl.vector_unit->writeLocalMemoryData(pipe.lm_addr, pipe.data);
                    pipe.pre_data = pipe.data;
                    break;
                case CommandVPRO::STORE_SHIFT_LEFT:
                    printf_error("VPRO_SIM ERROR: STORE_SHIFT_LEFT instruction not implemented \n");
                    break;
                case CommandVPRO::STORE_SHIFT_RIGHT:
                    printf_error(
                        "VPRO_SIM ERROR: STORE_SHIFT_RIGHT instruction not implemented \n");
                    break;
                case CommandVPRO::STORE_REVERSE:
                    printf_error("VPRO_SIM ERROR: STORE_REVERSE instruction not implemented \n");
                    break;
                default:
                    break;
            }
            pipe.flag[0] = __is_zero(pipe.pre_data);
            pipe.flag[1] = __is_negative(pipe.pre_data);

            // for LS chain data source:
            //  pipe.data = pipe.pre_data;
            //  take chained data as own result
            // TODO: this is done in stage end-1, could happen earlier here to save some cycles on chain LS
            //  to LS if following command get stalled due to load delay from this Load
        } else if (stage == 8) {
            //delay 2 stages (data access in stage 8)
            switch (pipe.cmd->type) {
                case CommandVPRO::LOAD:
                case CommandVPRO::LOADS:
                case CommandVPRO::LOADS_SHIFT_LEFT:
                case CommandVPRO::LOADS_SHIFT_RIGHT:
                case CommandVPRO::LOADB:
                case CommandVPRO::LOADBS:
                case CommandVPRO::LOAD_REVERSE:
                    vl.fifo.push(std::tie(pipe.pre_data, pipe.flag[0], pipe.flag[1]));
                    break;
                default:
                    break;
            }
        }
    }  // for stages
}

void LSPipeObject::execute_cmd(VectorLane& vl, PipelineDate& pipe) {
    // convert to 24 bit (fill 1 if signed...)
    pipe.data = *__24to32signed(pipe.data);

    printf_error("In L/S Lane, execute_cmd() should not be called ever! [But is!]\n");
}

/**
 * after execution the result of the command is pushed to the pipeline to be able to be accessed from another lane
 * @param newElement Pipeline_data type of result
 */
void PipeObject::process(std::shared_ptr<CommandVPRO> newElement) {
    //@INFO !KOM
    // shift each element and append new
    for (int i = 5 + pipelineALUDepth + 1; i > 0; i--) {
        data[i] = data[i - 1];
    }
    PipelineDate pipe;
    pipe.cmd = std::make_shared<CommandVPRO>(newElement.get());
    data[0] = pipe;
}

void PipeObject::processInStall(int from) {
    // process later half of pipeline that is not affected by missing input data of stalling source lane
    for (int i = 5 + pipelineALUDepth + 1; i > from; i--) {  // process half Pipeline
        data[i] = data[i - 1];  // shift each element and append new empty
    }
    PipelineDate pipe;
    pipe.cmd = std::make_shared<CommandVPRO>();
    // fill the empty pipeline stage with a none cmd
    data[from] = pipe;
}

bool PipeObject::isBusy() const {
    bool pipeline_busy = false;
    for (int i = 0; i < 5 + pipelineALUDepth + 1; i++) {
        pipeline_busy |= (data[i].cmd->type != CommandVPRO::NONE);
    }
    return pipeline_busy;
}

void PipeObject::update() {
    //*********************************************
    // Update Registers
    //*********************************************
    data[5 + pipelineALUDepth].data = data[5 + pipelineALUDepth].pre_data;
}

bool PipeObject::isChaining() const {
    return data[5 + pipelineALUDepth].cmd->is_chain;
}

bool PipeObject::isBlocking(int chain_target_stage) const {
    // any pipeline stage -3 as data is read in stage 4
    bool blocking = false;
    for (int stage = 0; stage <= 5 + pipelineALUDepth - 1 - (chain_target_stage); stage++) {
        //stage 0...3
        blocking |= data[stage].cmd->blocking;
    }
    return blocking;
}

void PipeObject::check_indirect_addressing(CommandVPRO cmd) {
    bool chain_offset_vl =
        chains_from_src(cmd, SRC_SEL_INDIRECT_LEFT) || chains_from_src(cmd, SRC_SEL_INDIRECT_RIGHT);
    bool chain_offset_ls = chains_from_src(cmd, SRC_SEL_INDIRECT_LS);
    bool chain_data_vl = chains_from_src(cmd, SRC_SEL_LEFT) || chains_from_src(cmd, SRC_SEL_RIGHT);
    bool chain_data_ls = chains_from_src(cmd, SRC_SEL_LS);

    if ((chain_offset_vl && chain_data_vl) || (chain_offset_ls && chain_data_ls)) {
        printf_error("Cannot simultaneously chain data and offsets from one lane!\n");
        exit(EXIT_FAILURE);
    }
}

}  // namespace Unit
