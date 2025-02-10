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
// ########################################################
// # VPRO instruction & system simulation library         #
// # Sven Gesper, IMS, Uni Hannover, 2019                 #
// ########################################################
// # debug settings and functions                         #
// ########################################################

#include "debugHelper.h"
#include <stdarg.h>  // For va_start, etc.
#include <memory>    // For std::unique_ptr
#include <string>
#include "../setting.h"

uint64_t debug = 0;

QString debugToText(DebugOptions op) {
    switch (op) {
        case DEBUG_INSTRUCTIONS:
            return {"DEBUG_INSTRUCTIONS"};
        case DEBUG_DMA:
            return {"DEBUG_DMA"};
        case DEBUG_DEV_NULL:
            return {"DEBUG_DEV_NULL"};
        case DEBUG_FIFO_MSG:
            return {"DEBUG_FIFO_MSG"};
        case DEBUG_PRINTF:
            return {"DEBUG_PRINTF"};
        case DEBUG_USER_DUMP:
            return {"DEBUG_USER_DUMP"};
        case DEBUG_INSTR_STATISTICS:
            return {"DEBUG_INSTR_STATISTICS"};
        case DEBUG_MODE:
            return {"DEBUG_USER_DUMP"};
        case DEBUG_TICK:
            return {"DEBUG_TICK"};
        case DEBUG_GLOBAL_TICK:
            return {"DEBUG_GLOBAL_TICK"};
        case DEBUG_INSTRUCTION_SCHEDULING:
            return {"DEBUG_INSTRUCTION_SCHEDULING"};
        case DEBUG_INSTRUCTION_DATA:
            return {"DEBUG_INSTRUCTION_DATA"};
        case DEBUG_PIPELINE:
            return {"DEBUG_PIPELINE"};
        case DEBUG_PIPELINE_9:
            return {"DEBUG_PIPELINE_9"};
        case DEBUG_DMA_DETAIL:
            return {"DEBUG_DMA_DETAIL"};
        case DEBUG_GLOBAL_VARIABLE_CHECK:
            return {"DEBUG_GLOBAL_VARIABLE_CHECK"};
        case DEBUG_CHAINING:
            return {"DEBUG_CHAINING"};
        case DEBUG_DUMP_FLAGS:
            return {"DEBUG_DUMP_FLAGS"};
        case DEBUG_INSTRUCTION_SCHEDULING_BASIC:
            return {"DEBUG_INSTRUCTION_SCHEDULING_BASIC"};
        case DEBUG_INSTR_STATISTICS_ALL:
            return {"DEBUG_INSTR_STATISTICS_ALL"};
        case DEBUG_DMA_ACCESS_TO_EXT_VARIABLE:
            return {"DEBUG_DMA_ACESS_TO_EXT_VARIABLE"};
        case DEBUG_PIPELINELENGTH_CHANGES:
            return {"DEBUG_PIPELINELENGTH_CHANGES"};
        case DEBUG_LANE_STALLS:
            return {"DEBUG_LANE_STALLS"};
        case end:
            return {"end"};
        default:
            return "unknown";
    }
}

int printf_warning(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    printf(ORANGE);
    //    printf("\n");
    vprintf(format, ap);
    //    printf("\n");
    printf(RESET_COLOR);
    va_end(ap);
    if (PAUSE_ON_WARNING_PRINT) {
        printf(ORANGE);
        printf("Press [Enter] to continue...\n");
        getchar();
        printf("execution resumed!\n");
        printf(RESET_COLOR);
    }
    return 0;
}

int printf_error(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    printf(RED);
    vprintf(format, ap);
    printf(RESET_COLOR);
    va_end(ap);
    if (PAUSE_ON_ERROR_PRINT) {
        printf(ORANGE);
        printf("Press [Enter] to continue...\n");
        getchar();
        printf("execution resumed!\n");
        printf(RESET_COLOR);
    }
    return 0;
}

int printf_failure(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    printf(RED);
    vprintf(format, ap);
    printf(RESET_COLOR);
    va_end(ap);
    if (PAUSE_ON_ERROR_PRINT) {
        printf(ORANGE);
        printf("Press [Enter] to continue...\n");
        getchar();
        printf("execution resumed!\n");
        printf(RESET_COLOR);
    }
    exit(1);
    return 0;
}

int printf_info(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    printf(LBLUE);
    vprintf(format, ap);
    printf(RESET_COLOR);
    va_end(ap);
    return 0;
}

int printf_success(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    printf(LGREEN);
    vprintf(format, ap);
    printf(RESET_COLOR);
    va_end(ap);
    return 0;
}

// ***********************************************************************
// Print Instruction
// ***********************************************************************

void print_cmd(CommandVPRO* cmd) {
    printf("VPRO-Command: ");
    cmd->print();
#if old_print_cmd
    if (cmd->type != CommandVPRO::NONE && cmd->type != CommandVPRO::IDMASK_GLOBAL &&
        cmd->type != CommandVPRO::WAIT_BUSY) {
        printf("\n\t\t details: (x: %i, y: %i) ", cmd->x, cmd->y);
        print_cmd(cmd->vector_unit_sel,
            cmd->vector_lane_sel,
            cmd->blocking,
            cmd->is_chain,
            cmd->fu_sel,
            cmd->func,
            cmd->flag_update,
            cmd->dst,
            cmd->src1,
            cmd->src2,
            cmd->x_end,
            cmd->y_end,
            cmd->id_mask);
    }
#endif
}
void print_cmd(CommandDMA* cmd) {
    printf("DMA-Command: ");
    cmd->print();
}
void print_cmd(CommandBase* cmd) {
    printf("BASE-Command: ");
    cmd->print_class_type();
}
void print_cmd(uint32_t vector_unit_sel,
    uint32_t vector_lane_sel,
    bool blocking,
    bool is_chain,
    uint32_t fu_sel,
    uint32_t func,
    bool flag_update,
    addr_field_t dst,
    addr_field_t src1,
    addr_field_t src2,
    uint32_t x_end,
    uint32_t y_end,
    uint32_t id_mask) {
    printf("Global_Unit %03d lane %01d mask %01d: ", vector_unit_sel, vector_lane_sel, id_mask);

    // print mnemonic
    switch (fu_sel) {
        case CLASS_MEM:
            switch (func) {
                case OPCODE_LOAD:
                    printf("LOAD     ");
                    break;
                case OPCODE_LOADS:
                    printf("LOADS    ");
                    break;
                case OPCODE_LOADB:
                    printf("LOADB    ");
                    break;
                case OPCODE_LOADBS:
                    printf("LOADBS   ");
                    break;
                case OPCODE_STORE:
                    printf("STORE    ");
                    break;
                default:
                    printf_warning("MEM_???  ");
                    break;
            }
            break;

        case CLASS_ALU:
            switch (func) {
                case OPCODE_ADD:
                    printf("ADD      ");
                    break;
                case OPCODE_SUB:
                    printf("SUB      ");
                    break;
                case OPCODE_MULL:
                    printf("MULL     ");
                    break;
                case OPCODE_MULH:
                    printf("MULH     ");
                    break;
                case OPCODE_MACL:
                    printf("MACL     ");
                    break;
                case OPCODE_MACH:
                    printf("MACH     ");
                    break;
                case OPCODE_MACL_PRE:
                    printf("MACL_PRE ");
                    break;
                case OPCODE_MACH_PRE:
                    printf("MACH_PRE ");
                    break;
                case OPCODE_XOR:
                    printf("XOR      ");
                    break;
                case OPCODE_XNOR:
                    printf("XNOR     ");
                    break;
                case OPCODE_AND:
                    printf("AND      ");
                    break;
                case OPCODE_NAND:
                    printf("NAND     ");
                    break;
                case OPCODE_OR:
                    printf("OR       ");
                    break;
                case OPCODE_NOR:
                    printf("NOR      ");
                    break;
                default:
                    printf_warning("ALU_???  ");
                    break;
            }
            break;

        case CLASS_SPECIAL:
            switch (func) {
                case OPCODE_SHIFT_LR:
                    printf("SHIFT_LR ");
                    break;
                case OPCODE_SHIFT_AR:
                    printf("SHIFT_AR ");
                    break;
                case OPCODE_ABS:
                    printf("ABS      ");
                    break;
                case OPCODE_MIN:
                    printf("MIN      ");
                    break;
                case OPCODE_MAX:
                    printf("MAX      ");
                    break;
                default:
                    printf_warning("SPEC_??? ");
                    break;
            }
            break;

        case CLASS_TRANSFER:
            switch (func) {
                case OPCODE_MV_ZE:
                    printf("MV_ZE    ");
                    break;
                case OPCODE_MV_NZ:
                    printf("MV_NZ    ");
                    break;
                case OPCODE_MV_MI:
                    printf("MV_MI    ");
                    break;
                case OPCODE_MV_PL:
                    printf("MV_PL    ");
                    break;
                default:
                    printf_warning("TRAN_??? ");
                    break;
            }
            break;
        default:
            printf_warning("????_??? ");
            break;
    }

    // blocking?
    if (blocking)
        printf("BLOCK    ");
    else
        printf("NONBLOCK ");

    // chaining?
    if (is_chain)
        printf("IS_CHAIN ");
    else
        printf("NO_CHAIN ");

    // destination
    printf("DST(%04d, %02d, %02d) ", dst.offset, dst.alpha, dst.beta);

    // source 1
    if (src1.sel == SRC_SEL_ADDR)
        printf("SRC1(%04d, %02d, %02d) ", src1.offset, src1.alpha, src1.beta);
    else if (src1.sel == SRC_SEL_IMM)
        printf("SRC1(imm %04d)   ", src1.getImm());
    else if (src1.sel == SRC_SEL_LEFT || src1.sel == SRC_SEL_RIGHT)
        printf("SRC1(chain %s%i),    ",
            (src1.sel == SRC_SEL_RIGHT ? "RIGHT" : (src1.sel == SRC_SEL_LEFT ? "LEFT" : "L")),
            src1.gamma);
    else
        printf("SRC1( undefined)   ");

    // source 2
    if (src2.sel == SRC_SEL_ADDR)
        printf("SRC2(%04d, %02d, %02d) ", src2.offset, src1.alpha, src1.beta);
    else if (src2.sel == SRC_SEL_IMM)
        printf("SRC2(imm %04d)   ", src2.getImm());
    else if (src2.sel == SRC_SEL_LEFT || src2.sel == SRC_SEL_RIGHT)
        printf("SRC1(chain %s%i),    ",
            (src2.sel == SRC_SEL_RIGHT ? "RIGHT" : (src2.sel == SRC_SEL_LEFT ? "LEFT" : "L")),
            src2.gamma);
    else
        printf("SRC2( undefined)   ");

    // loop delimiters
    printf("x_end = %02d, y_end = %02d\n", x_end, y_end);
}

bool if_debug(DebugOptions op) {
    return (debug & op);
}

void ifm_debug(DebugOptions op, const char* msg) {
    if (if_debug(op)) {
        printf("%s", msg);
    }
}
