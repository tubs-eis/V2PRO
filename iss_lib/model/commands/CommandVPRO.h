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
 * @file CommandVPRO.h
 * @author Sven Gesper, IMS, Uni Hannover, 2019
 * @date 2019
 *
 * VPRO instruction & system simulation library
 * a class for commands to the Vector processor itself
 */

#ifndef VPRO_CPP_COMMANDVPRO_H
#define VPRO_CPP_COMMANDVPRO_H

#include <QString>
#include <memory>

#include "../../simulator/helper/structTypes.h"
#include "CommandBase.h"
#include "vpro/vpro_cmd_defs.h"

class CommandVPRO : public CommandBase {
   public:
    static const int MAX_ALU_DEPTH = 16;

    enum TYPE {
        NONE,
        LOAD,
        LOADB,
        LOADS,
        LOADBS,
        STORE,
        LOADS_SHIFT_LEFT,
        LOADS_SHIFT_RIGHT,  // data = data << pipe.dst.offset;
        STORE_SHIFT_LEFT,
        STORE_SHIFT_RIGHT,  // data = data << pipe.opb;
        LOAD_REVERSE,
        STORE_REVERSE,
        LOOP_START,
        LOOP_END,
        LOOP_MASK,
        ADD,
        SUB,
        MULL,
        MULH,
        MACL,
        DIVL,
        DIVH,
        MACH,
        MACL_PRE,
        MACH_PRE,
        XOR,
        XNOR,
        AND,
        NAND,
        OR,
        NOR,
        SHIFT_LR,
        SHIFT_AR,
        SHIFT_LL,
        ABS,
        MIN,
        MAX,
        MIN_VECTOR,
        MAX_VECTOR,
        BIT_REVERSAL,
        MV_ZE,
        MV_NZ,
        MV_MI,
        MV_PL,
        SHIFT_AR_NEG,
        SHIFT_AR_POS,
        MULL_NEG,
        MULL_POS,
        MULH_NEG,
        MULH_POS,
        NOP,
        WAIT_BUSY,
        PIPELINE_WAIT,
        IDMASK_GLOBAL,
        enumTypeEnd
    } type;

    uint32_t load_offset;
    uint32_t id_mask;
    uint32_t cluster;
    //    uint32_t vector_unit_sel;
    //    uint32_t vector_lane_sel;
    bool blocking;
    bool is_chain;
    uint32_t fu_sel;
    uint32_t func;
    bool flag_update;

    addr_field_t dst, src1, src2;

    uint32_t x_end;
    uint32_t x;
    uint32_t y_end;
    uint32_t y;
    uint32_t z_end;
    uint32_t z;

    uint32_t pipelineALUDepth;

    bool done;

    CommandVPRO();
    CommandVPRO(CommandVPRO* ref);
    CommandVPRO(CommandVPRO& ref);
    CommandVPRO(std::shared_ptr<CommandVPRO> ref);

    /**
     * update 'type' based on bits on 'fu_sel' && 'func'
     */
    void updateType();

    void print(FILE* out = stdout) override;
    void print_type(FILE* out = stdout) const;
    static void printType(CommandVPRO::TYPE t, FILE* out = stdout);

    // String returning functions
    QString get_type() override;
    QString get_string() override;

    static QString getType(CommandVPRO::TYPE t);

    void updateSrc1(int addToOffset = -1) {
        src1.offset += addToOffset;
    };  // update Src1

    void updateSrc2(int addToOffset = -1) {
        src2.offset += addToOffset;
    };  // update Src1

    bool isWriteRF();
    bool isWriteLM();
    bool isLS() const;
    bool isLoad() const;

    bool isChainPartCMD();
    bool is_done() override;

    uint32_t get_func_type();
    uint32_t get_lane_dst_sel();
    uint32_t create_IMM();
    uint32_t get_src2_off();
    uint32_t get_load_offset();

   private:
};

#endif  //VPRO_CPP_COMMANDVPRO_H
