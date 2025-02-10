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
 * @file CommandVPRO.cpp
 * @author Alexander Köhne, Sven Gesper
 * @date 06.03.2019
 *
 * Commands for the VPRO
 */

#include "CommandVPRO.h"
#include "../../simulator/helper/debugHelper.h"

#include <iomanip>
#include <sstream>

CommandVPRO::CommandVPRO() : CommandBase(VPRO) {
    type = CommandVPRO::NONE;
    blocking = false;
    is_chain = false;
    fu_sel = CLASS_OTHER;
    func = OPCODE_ADD;
    flag_update = false;
    x_end = 0;
    x = 0;
    y_end = 0;
    y = 0;
    z_end = 0;
    z = 0;
    id_mask = 0xffffffff;
    cluster = 0;
    done = false;

    load_offset = 0;
    dst = addr_field_t();
    src1 = addr_field_t();
    src2 = addr_field_t();

    pipelineALUDepth = 3;
}
CommandVPRO::CommandVPRO(CommandVPRO* ref) : CommandBase(ref->class_type) {
    type = ref->type;
    id_mask = ref->id_mask;
    cluster = ref->cluster;
    //    vector_unit_sel = ref->vector_unit_sel;
    // vector_lane_sel = ref->vector_lane_sel;
    dst = ref->dst;
    src1 = ref->src1;
    src2 = ref->src2;
    blocking = ref->blocking;
    is_chain = ref->is_chain;
    fu_sel = ref->fu_sel;
    func = ref->func;
    flag_update = ref->flag_update;
    x_end = ref->x_end;
    x = ref->x;
    y_end = ref->y_end;
    y = ref->y;
    z_end = ref->z_end;
    z = ref->z;
    done = ref->done;
    pipelineALUDepth = ref->pipelineALUDepth;
}
CommandVPRO::CommandVPRO(CommandVPRO& ref) : CommandBase(ref.class_type) {
    type = ref.type;
    id_mask = ref.id_mask;
    cluster = ref.cluster;
    //    vector_unit_sel = ref.vector_unit_sel;
    // vector_lane_sel = ref.vector_lane_sel;
    dst = ref.dst;
    src1 = ref.src1;
    src2 = ref.src2;
    blocking = ref.blocking;
    is_chain = ref.is_chain;
    fu_sel = ref.fu_sel;
    func = ref.func;
    flag_update = ref.flag_update;
    x_end = ref.x_end;
    x = ref.x;
    y_end = ref.y_end;
    y = ref.y;
    z_end = ref.z_end;
    z = ref.z;
    done = ref.done;
    pipelineALUDepth = ref.pipelineALUDepth;
}
CommandVPRO::CommandVPRO(std::shared_ptr<CommandVPRO> ref) : CommandBase(ref->class_type) {
    type = ref->type;
    id_mask = ref->id_mask;
    cluster = ref->cluster;
    //    vector_unit_sel = ref->vector_unit_sel;
    // vector_lane_sel = ref->vector_lane_sel;
    dst = ref->dst;
    src1 = ref->src1;
    src2 = ref->src2;
    blocking = ref->blocking;
    is_chain = ref->is_chain;
    fu_sel = ref->fu_sel;
    func = ref->func;
    flag_update = ref->flag_update;
    x_end = ref->x_end;
    x = ref->x;
    y_end = ref->y_end;
    y = ref->y;
    z_end = ref->z_end;
    z = ref->z;
    done = ref->done;
    pipelineALUDepth = ref->pipelineALUDepth;
}

void CommandVPRO::printType(CommandVPRO::TYPE t, FILE* out) {
    CommandVPRO c;
    c.type = t;
    c.print_type(out);
}

QString CommandVPRO::getType(CommandVPRO::TYPE t) {
    CommandVPRO c;
    c.type = t;
    return c.get_type();
}

bool CommandVPRO::is_done() {
    return done || (x > x_end && y > y_end && z > z_end);
}

bool CommandVPRO::isWriteRF() {
    return (type == ADD || type == SUB || type == MULL || type == MULH || type == DIVL ||
            type == DIVH || type == MACL || type == MACH || type == MACL_PRE || type == MACH_PRE ||
            type == XOR || type == XNOR || type == AND || type == NAND || type == OR ||
            type == NOR || type == SHIFT_LR || type == SHIFT_AR || type == SHIFT_LL ||
            type == MV_ZE || type == MV_NZ || type == MV_MI || type == MV_PL ||
            type == SHIFT_AR_NEG || type == SHIFT_AR_POS || type == MULL_NEG || type == MULL_POS ||
            type == MULH_NEG || type == MULH_POS || type == MIN || type == MAX || type == ABS ||
            type == MIN_VECTOR || type == MAX_VECTOR || type == BIT_REVERSAL);
}
bool CommandVPRO::isWriteLM() {
    return (type == STORE || type == STORE_SHIFT_LEFT || type == STORE_SHIFT_RIGHT ||
            type == STORE_REVERSE);
}

bool CommandVPRO::isLS() const {
    return (type == STORE || type == LOAD || type == LOADB || type == LOADBS || type == LOADS ||
            type == STORE_SHIFT_LEFT || type == STORE_SHIFT_RIGHT || type == LOADS_SHIFT_LEFT ||
            type == LOADS_SHIFT_RIGHT || type == STORE_REVERSE || type == LOAD_REVERSE);
}

bool CommandVPRO::isLoad() const {
    return (type == LOAD || type == LOADB || type == LOADBS || type == LOADS ||
            type == LOADS_SHIFT_LEFT || type == LOADS_SHIFT_RIGHT || type == LOAD_REVERSE);
}

bool CommandVPRO::isChainPartCMD() {
    return (is_chain || src1.sel == SRC_SEL_LEFT || src1.sel == SRC_SEL_RIGHT ||
            src1.sel == SRC_SEL_LS || src2.sel == SRC_SEL_LEFT || src2.sel == SRC_SEL_RIGHT ||
            src2.sel == SRC_SEL_LS);
}

QString CommandVPRO::get_type() {
    switch (type) {
        case NONE:
            return {"NONE     "};
        case LOAD:
            return {"LOAD     "};
        case LOADB:
            return {"LOADB    "};
        case LOADS:
            return {"LOADS    "};
        case LOADS_SHIFT_LEFT:
            return {"LD_SFT_L "};
        case LOADS_SHIFT_RIGHT:
            return {"LD_SFT_R "};
        case LOADBS:
            return {"LOADBS   "};
        case LOAD_REVERSE:
            return {"LOAD_REV "};
        case STORE:
            return {"STORE    "};
        case STORE_SHIFT_LEFT:
            return {"ST_SFT_L "};
        case STORE_SHIFT_RIGHT:
            return {"ST_SFT_R "};
        case STORE_REVERSE:
            return {"STORE_REV"};
        case ADD:
            return {"ADD      "};
        case SUB:
            return {"SUB      "};
        case MULL:
            return {"MULL     "};
        case MULH:
            return {"MULH     "};
        case DIVL:
            return {"DIVL     "};
        case DIVH:
            return {"DIVH     "};
        case MACL:
            return {"MACL     "};
        case MACH:
            return {"MACH     "};
        case MACL_PRE:
            return {"MACL_PRE "};
        case MACH_PRE:
            return {"MACH_PRE "};
        case XOR:
            return {"XOR      "};
        case XNOR:
            return {"XNOR     "};
        case AND:
            return {"AND      "};
        case NAND:
            return {"NAND     "};
        case OR:
            return {"OR       "};
        case NOR:
            return {"NOR      "};
        case SHIFT_LR:
            return {"SHIFT_LR "};
        case SHIFT_AR:
            return {"SHIFT_AR "};
        case SHIFT_LL:
            return {"SHIFT_LL "};
        case ABS:
            return {"ABS      "};
        case MIN:
            return {"MIN      "};
        case MAX:
            return {"MAX      "};
        case MIN_VECTOR:
            return {"MIN_VEC  "};
        case MAX_VECTOR:
            return {"MAX_VEC  "};
        case BIT_REVERSAL:
            return {"BIT_REV  "};
        case MV_ZE:
            return {"MV_ZE    "};
        case MV_NZ:
            return {"MV_NZ    "};
        case MV_MI:
            return {"MV_MI    "};
        case MV_PL:
            return {"MV_PL    "};
        case SHIFT_AR_NEG:
            return {"SHFTR_NEG"};
        case SHIFT_AR_POS:
            return {"SHFTR_POS"};
        case MULL_NEG:
            return {"MULL_NEG "};
        case MULL_POS:
            return {"MULL_POS "};
        case MULH_NEG:
            return {"MULH_NEG "};
        case MULH_POS:
            return {"MULH_POS "};
        case NOP:
            return {"NOP      "};
        case WAIT_BUSY:
            return {"WAIT_BUSY"};
        case PIPELINE_WAIT:
            return {"PIPELN_W8"};
        case IDMASK_GLOBAL:
            return QString("IDMASK_GLOBAL: %1").arg(id_mask);
        default:
            return {"Unknown Function!"};
    }
}

void CommandVPRO::print_type(FILE* out) const {
    switch (type) {
        case NONE:
            fprintf(out, "NONE     ");
            return;
        case LOAD:
            fprintf(out, "LOAD     ");
            break;
        case LOADB:
            fprintf(out, "LOADB    ");
            break;
        case LOADS:
            fprintf(out, "LOADS    ");
            break;
        case LOADS_SHIFT_LEFT:
            fprintf(out, "LD_SFT_L ");
            break;
        case LOADS_SHIFT_RIGHT:
            fprintf(out, "LD_SFT_R ");
            break;
        case LOAD_REVERSE:
            fprintf(out, "LOAD_REV ");
            break;
        case LOADBS:
            fprintf(out, "LOADBS   ");
            break;
        case STORE:
            fprintf(out, "STORE    ");
            break;
        case STORE_SHIFT_LEFT:
            fprintf(out, "ST_SFT_L ");
            break;
        case STORE_SHIFT_RIGHT:
            fprintf(out, "ST_SFT_R ");
            break;
        case STORE_REVERSE:
            fprintf(out, "STORE_REV");
            break;
        case ADD:
            fprintf(out, "ADD      ");
            break;
        case SUB:
            fprintf(out, "SUB      ");
            break;
        case MULL:
            fprintf(out, "MULL     ");
            break;
        case MULH:
            fprintf(out, "MULH     ");
            break;
        case MACL:
            fprintf(out, "MACL     ");
            break;
        case MACH:
            fprintf(out, "MACH     ");
            break;
        case MACL_PRE:
            fprintf(out, "MACL_PRE ");
            break;
        case MACH_PRE:
            fprintf(out, "MACH_PRE ");
            break;
        case XOR:
            fprintf(out, "XOR      ");
            break;
        case XNOR:
            fprintf(out, "XNOR     ");
            break;
        case AND:
            fprintf(out, "AND      ");
            break;
        case NAND:
            fprintf(out, "NAND     ");
            break;
        case OR:
            fprintf(out, "OR       ");
            break;
        case NOR:
            fprintf(out, "NOR      ");
            break;
        case SHIFT_LR:
            fprintf(out, "SHIFT_LR ");
            break;
        case SHIFT_LL:
            fprintf(out, "SHIFT_LL ");
            break;
        case SHIFT_AR:
            fprintf(out, "SHIFT_AR ");
            break;
        case ABS:
            fprintf(out, "ABS      ");
            break;
        case MIN:
            fprintf(out, "MIN      ");
            break;
        case MAX:
            fprintf(out, "MAX      ");
            break;
        case MIN_VECTOR:
            fprintf(out, "MIN_VEC  ");
            break;
        case MAX_VECTOR:
            fprintf(out, "MAX_VEC  ");
            break;
        case BIT_REVERSAL:
            fprintf(out, "BIT_REV  ");
            break;
        case MV_ZE:
            fprintf(out, "MV_ZE    ");
            break;
        case MV_NZ:
            fprintf(out, "MV_NZ    ");
            break;
        case MV_MI:
            fprintf(out, "MV_MI    ");
            break;
        case MV_PL:
            fprintf(out, "MV_PL    ");
            break;
        case SHIFT_AR_NEG:
            fprintf(out, "SHFTR_NEG");
            break;
        case SHIFT_AR_POS:
            fprintf(out, "SHFTR_POS");
            break;
        case MULL_NEG:
            fprintf(out, "MULL_NEG ");
            break;
        case MULL_POS:
            fprintf(out, "MULL_POS ");
            break;
        case MULH_NEG:
            fprintf(out, "MULH_NEG ");
            break;
        case MULH_POS:
            fprintf(out, "MULH_POS ");
            break;
        case NOP:
            fprintf(out, "NOP      ");
            return;
        case WAIT_BUSY:
            fprintf(out, "WAIT_BUSY");
            return;
        case PIPELINE_WAIT:
            fprintf(out, "PIPELN_W8");
            return;
        case IDMASK_GLOBAL:
            fprintf(out, "IDMASK_GLOBAL: %i", id_mask);
            return;
        default:
            fprintf(out, "Unknown Function!");
            printf_warning("Unknown Function!");
            return;
    }
}

QString CommandVPRO::get_string() {
    QString str = QString("");
    str += get_class_type();
    str += ", Func: " + get_type();

    str += QString(" Mask: %1,").arg(id_mask, 3);
    if (type == CommandVPRO::WAIT_BUSY) str += QString(" Cluster: %1, ").arg(cluster, 3);

    str += QString("DST(%1, %2, %3, %4), ")
               .arg(dst.offset, 3)
               .arg(dst.alpha, 3)
               .arg(dst.beta, 3)
               .arg(dst.gamma, 3);

    if (src1.sel == SRC_SEL_ADDR)
        str += QString("SRC1(%1, %2, %3, %4), ")
                   .arg(src1.offset, 3)
                   .arg(src1.alpha, 3)
                   .arg(src1.beta, 3)
                   .arg(src1.beta, 3);
    else if (src1.sel == SRC_SEL_IMM)
        str += QString("SRC1(Imm: %1),     ").arg(src1.getImm(), 4);
    else if (src1.sel == SRC_SEL_LEFT || src1.sel == SRC_SEL_RIGHT)
        str += QString("SRC1(chain ") +
               (src1.sel == SRC_SEL_RIGHT
                       ? "RIGHT"
                       : (src1.sel == SRC_SEL_LEFT ? "LEFT" : "L" + QString::number(src1.gamma))) +
               QString("),    ");
    else if (src1.chain_ls)
        str += QString("SRC1(LS),            ");
    else
        str += QString("SRC1(undefined),     ");

    if (src2.sel == SRC_SEL_ADDR)
        str += QString("SRC2(%1, %2, %3, %4), ")
                   .arg(src2.offset, 3)
                   .arg(src2.alpha, 3)
                   .arg(src2.beta, 3)
                   .arg(dst.gamma, 3);
    else if (src2.sel == SRC_SEL_IMM)
        str += QString("SRC2(Imm: %1),     ").arg(src2.getImm(), 4);
    else if (src2.sel == SRC_SEL_LEFT || src2.sel == SRC_SEL_RIGHT)
        str += QString("SRC2(chain ") +
               (src2.sel == SRC_SEL_RIGHT
                       ? "RIGHT"
                       : (src2.sel == SRC_SEL_LEFT ? "LEFT" : "L" + QString::number(src2.gamma))) +
               QString("),    ");
    else if (src2.chain_ls)
        str += QString("SRC2(LS),            ");
    else
        str += QString("SRC2(undefined),     ");

    str += QString("(x=%1 [0;%2], y=%3 [0;%4], z=%5 [0;%6]), %7%8%9")
               .arg(x)
               .arg(x_end)
               .arg(y)
               .arg(y_end)
               .arg(z)
               .arg(z_end)
               .arg((blocking) ? "BLOCK, " : "")
               .arg((is_chain) ? "CHAIN, " : "")
               .arg((flag_update) ? "FLAG_UPDATE" : "");
    return str;
}

void CommandVPRO::print(FILE* out) {
    print_class_type(out);
    fprintf(out, "Func: ");
    print_type(out);

    fprintf(out, " Mask: %03d,", id_mask);
    if (type == CommandVPRO::WAIT_BUSY) fprintf(out, " Cluster: %03d, ", cluster);

    fprintf(out, "DST(%03d, %03d, %03d, %03d), ", dst.offset, dst.alpha, dst.beta, dst.gamma);

    if (src1.sel == SRC_SEL_ADDR)
        fprintf(
            out, "SRC1(%03d, %03d, %03d, %03d), ", src1.offset, src1.alpha, src1.beta, src1.gamma);
    else if (src1.sel == SRC_SEL_IMM)
        fprintf(out, "SRC1(Imm: %04d),     ", src1.getImm());
    else if (src1.sel == SRC_SEL_LEFT || src1.sel == SRC_SEL_RIGHT)
        fprintf(out,
            "SRC1(chain %s%i),    ",
            (src1.sel == SRC_SEL_RIGHT ? "RIGHT" : (src1.sel == SRC_SEL_LEFT ? "LEFT" : "L")),
            src1.gamma);
    else if (src1.chain_ls)
        fprintf(out, "SRC1(LS),            ");
    else
        fprintf(out, "SRC1(undefined),     ");

    if (src2.sel == SRC_SEL_ADDR)
        fprintf(
            out, "SRC2(%03d, %03d, %03d, %03d), ", src2.offset, src2.alpha, src2.beta, src2.gamma);
    else if (src2.sel == SRC_SEL_IMM)
        fprintf(out, "SRC2(Imm: %04d),     ", src2.getImm());
    else if (src2.sel == SRC_SEL_LEFT || src2.sel == SRC_SEL_RIGHT)
        fprintf(out,
            "SRC2(chain %s%i),    ",
            (src2.sel == SRC_SEL_RIGHT ? "RIGHT" : (src2.sel == SRC_SEL_LEFT ? "LEFT" : "L")),
            src2.gamma);
    else if (src2.chain_ls)
        fprintf(out, "SRC1(LS),            ");
    else
        fprintf(out, "SRC2(undefined),     ");

    fprintf(out,
        "(x=%i [0;%i], y=%i [0;%i], z=%i [0;%i]), %s%s%s\n",
        x,
        x_end,
        y,
        y_end,
        z,
        z_end,
        ((blocking) ? "BLOCK, " : ""),
        ((is_chain) ? "CHAIN, " : ""),
        ((flag_update) ? "FLAG_UPDATE" : ""));
}

void CommandVPRO::updateType() {
    if (type == CommandVPRO::NONE) {
        switch (fu_sel) {
            case CLASS_MEM:
                // CHANGE for L/S Lane
                id_mask = LS;
                if (!is_chain && func != OPCODE_STORE) {
                    printf_warning(
                        "Set IS_CHAIN in L/S commands to IS_CHAIN! Only chaining from L/S commands "
                        "is allowed! [got set to True]\n");
                    is_chain = true;
                }
                switch (func) {
                    case OPCODE_LOAD:
                        type = CommandVPRO::LOAD;
                        break;
                    case OPCODE_LOADS:
                        type = CommandVPRO::LOADS;
                        break;
                    case OPCODE_LOADS_SHIFT_LEFT:
                        type = CommandVPRO::LOADS_SHIFT_LEFT;
                        break;
                    case OPCODE_LOADS_SHIFT_RIGHT:
                        type = CommandVPRO::LOADS_SHIFT_RIGHT;
                        break;
                    case OPCODE_LOADB:
                        type = CommandVPRO::LOADB;
                        break;
                    case OPCODE_LOADBS:
                        type = CommandVPRO::LOADBS;
                        break;
                    case OPCODE_STORE:
                        type = CommandVPRO::STORE;
                        break;
                    case OPCODE_STORE_SHIFT_LEFT:
                        type = CommandVPRO::STORE_SHIFT_LEFT;
                        break;
                    case OPCODE_STORE_SHIFT_RIGHT:
                        type = CommandVPRO::STORE_SHIFT_RIGHT;
                        break;
                    case OPCODE_LOAD_REVERSE:
                        type = CommandVPRO::LOAD_REVERSE;
                        break;
                    case OPCODE_STORE_REVERSE:
                        type = CommandVPRO::STORE_REVERSE;
                        break;

                    default:
                        printf_error(
                            "VPRO_SIM ERROR: Invalid CLASS_MEM function! (fu=%d, func=%d, "
                            "src1_sel=%d, src2_sel=%d)\n",
                            fu_sel,
                            func,
                            src1.sel,
                            src2.sel);
                        break;
                }
                break;
            case CLASS_ALU:
                switch (func) {
                    case OPCODE_ADD:
                        type = CommandVPRO::ADD;
                        break;
                    case OPCODE_SUB:
                        type = CommandVPRO::SUB;
                        break;
                    case OPCODE_MULL:
                        type = CommandVPRO::MULL;
                        break;
                    case OPCODE_MULH:
                        type = CommandVPRO::MULH;
                        break;
                    case OPCODE_DIVL:
                        type = CommandVPRO::DIVL;
                        break;
                    case OPCODE_DIVH:
                        type = CommandVPRO::DIVH;
                        break;
                    case OPCODE_MACL:
                        type = CommandVPRO::MACL;
                        break;
                    case OPCODE_MACH:
                        type = CommandVPRO::MACH;
                        break;
                    case OPCODE_MACL_PRE:
                        type = CommandVPRO::MACL_PRE;
                        break;
                    case OPCODE_MACH_PRE:
                        type = CommandVPRO::MACH_PRE;
                        break;
                    case OPCODE_XOR:
                        type = CommandVPRO::XOR;
                        break;
                    case OPCODE_XNOR:
                        type = CommandVPRO::XNOR;
                        break;
                    case OPCODE_AND:
                        type = CommandVPRO::AND;
                        break;
                    case OPCODE_NAND:
                        type = CommandVPRO::NAND;
                        break;
                    case OPCODE_OR:
                        type = CommandVPRO::OR;
                        break;
                    case OPCODE_NOR:
                        type = CommandVPRO::NOR;
                        break;
                    default:
                        printf_error(
                            "VPRO_SIM ERROR: Invalid CLASS_ALU function! (fu=%d, func=%d, "
                            "src1_sel=%d, src2_sel=%d)\n",
                            fu_sel,
                            func,
                            src1.sel,
                            src2.sel);
                        break;
                }
                break;
            case CLASS_SPECIAL:
                switch (func) {
                    case OPCODE_SHIFT_LL:
                        type = CommandVPRO::SHIFT_LL;
                        break;
                    case OPCODE_SHIFT_LR:
                        type = CommandVPRO::SHIFT_LR;
                        break;
                    case OPCODE_SHIFT_AR:
                        type = CommandVPRO::SHIFT_AR;
                        break;
                    case OPCODE_ABS:
                        type = CommandVPRO::ABS;
                        break;
                    case OPCODE_MIN:
                        type = CommandVPRO::MIN;
                        break;
                    case OPCODE_MAX:
                        type = CommandVPRO::MAX;
                        break;
                    case OPCODE_MIN_VECTOR:
                        type = CommandVPRO::MIN_VECTOR;
                        break;
                    case OPCODE_MAX_VECTOR:
                        type = CommandVPRO::MAX_VECTOR;
                        break;
                    case OPCODE_BIT_REVERSAL:
                        type = CommandVPRO::BIT_REVERSAL;
                        break;
                    default:
                        printf_error(
                            "VPRO_SIM ERROR: Invalid CLASS_SPECIAL function! (fu=%d, func=%d, "
                            "src1_sel=%d, src2_sel=%d)\n",
                            fu_sel,
                            func,
                            src1.sel,
                            src2.sel);
                        break;
                }
                break;
            case CLASS_TRANSFER:
                switch (func) {
                    case OPCODE_MV_ZE:
                        type = CommandVPRO::MV_ZE;
                        break;
                    case OPCODE_MV_NZ:
                        type = CommandVPRO::MV_NZ;
                        break;
                    case OPCODE_MV_MI:
                        type = CommandVPRO::MV_MI;
                        break;
                    case OPCODE_MV_PL:
                        type = CommandVPRO::MV_PL;
                        break;
                    case OPCODE_MULL_NEG:
                        type = CommandVPRO::MULL_NEG;
                        break;
                    case OPCODE_MULL_POS:
                        type = CommandVPRO::MULL_POS;
                        break;
                    case OPCODE_MULH_NEG:
                        type = CommandVPRO::MULH_NEG;
                        break;
                    case OPCODE_MULH_POS:
                        type = CommandVPRO::MULH_POS;
                        break;
                    case OPCODE_SHIFT_AR_NEG:
                        type = CommandVPRO::SHIFT_AR_NEG;
                        break;
                    case OPCODE_SHIFT_AR_POS:
                        type = CommandVPRO::SHIFT_AR_POS;
                        break;
                    default:
                        printf_error(
                            "VPRO_SIM ERROR: Invalid CLASS_TRANSFER function! (fu=%d, func=%d, "
                            "src1_sel=%d, src2_sel=%d)\n",
                            fu_sel,
                            func,
                            src1.sel,
                            src2.sel);
                        break;
                }
                break;
            default:
                printf_error(
                    "VPRO_SIM ERROR: Invalid FU_SEL! (fu=%d, func=%d, src1_sel=%d, src2_sel=%d)\n",
                    fu_sel,
                    func,
                    src1.sel,
                    src2.sel);
                break;
        }
    } else {  // type already set
        return;
    }
};  // updateType

/**
 * TODO: Add missing functions...
*/
uint32_t CommandVPRO::get_func_type() {
    switch (type) {
        case LOAD:
            return FUNC_LOAD;
        case LOADS:
            return FUNC_LOADS;
        case STORE:
            return FUNC_STORE;
        case LOADS_SHIFT_LEFT:
            return FUNC_LOAD_SHIFT_LEFT;
        case LOADS_SHIFT_RIGHT:
            return FUNC_LOAD_SHIFT_RIGHT;
        case STORE_SHIFT_LEFT:
            return FUNC_STORE_SHIFT_LEFT;
        case STORE_SHIFT_RIGHT:
            return FUNC_STORE_SHIFT_RIGHT;
        case ADD:
            return FUNC_ADD;
        case SUB:
            return FUNC_SUB;
        case MULL:
            return FUNC_MULL;
        case MULH:
            return FUNC_MULH;
        case DIVL:
            return FUNC_DIVL;
        case DIVH:
            return FUNC_DIVH;
        case MACL:
            return FUNC_MACL;
        case MACH:
            return FUNC_MACH;
        case MACL_PRE:
            return FUNC_MACL_PRE;
        case MACH_PRE:
            return FUNC_MACH_PRE;
        case XOR:
            return FUNC_XOR;
        case XNOR:
            return FUNC_XNOR;
        case AND:
            return FUNC_AND;
        case NAND:
            return FUNC_NAND;
        case OR:
            return FUNC_OR;
        case NOR:
            return FUNC_NOR;
        case SHIFT_LR:
            return FUNC_SHIFT_LR;
        case SHIFT_AR:
            return FUNC_SHIFT_AR;
        case SHIFT_LL:
            return FUNC_SHIFT_LL;
        case ABS:
            return FUNC_ABS;
        case MIN:
            return FUNC_MIN;
        case MAX:
            return FUNC_MAX;
        case MIN_VECTOR:
            return FUNC_MIN_VECTOR;
        case MAX_VECTOR:
            return FUNC_MAX_VECTOR;
        case BIT_REVERSAL:
            return FUNC_BIT_REVERSAL;
        case MV_ZE:
            return FUNC_MV_ZE;
        case MV_NZ:
            return FUNC_MV_NZ;
        case MV_MI:
            return FUNC_MV_MI;
        case MV_PL:
            return FUNC_MV_PL;
        case SHIFT_AR_NEG:
            return FUNC_SHIFT_AR_NEG;
        case SHIFT_AR_POS:
            return FUNC_SHIFT_AR_POS;
        case MULL_NEG:
            return FUNC_MULL_NEG;
        case MULL_POS:
            return FUNC_MULL_POS;
        case MULH_NEG:
            return FUNC_MULH_NEG;
        case MULH_POS:
            return FUNC_MULH_POS;
        // case NOP:            return FUNC_NOP;
        // case WAIT_BUSY:      return FUNC_WAIT_BUSY;
        default:
            printf("CommandVPRO.cpp, get_func_type reports: %u Unknown Function!", type);
            break;
    }
}

uint32_t CommandVPRO::get_lane_dst_sel() {
    return static_cast<uint32_t>(dst.sel);  // NOT USED!!
}

uint32_t CommandVPRO::get_src2_off() {
    uint32_t l_off = load_offset >> 1;
    l_off = l_off - (y_end | blocking);
    return l_off;
}

uint32_t CommandVPRO::get_load_offset() {
    uint32_t l_off = get_src2_off();
    l_off = (l_off & ((1 << ISA_COMPLEX_LENGTH_3D) - 1)) >> ISA_COMPLEX_LENGTH_3D;
    printf("LOFF (CMD) %u\n", l_off);
    return l_off;
}