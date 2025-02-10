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
// # a class for commands to the simulator                #
// ########################################################

#ifndef VPRO_CPP_COMMANDSIM_H
#define VPRO_CPP_COMMANDSIM_H

#include <string>

#include "CommandBase.h"

class CommandSim : public CommandBase {
   public:
    enum TYPE {
        NONE,
        BIN_FILE_SEND,
        BIN_FILE_DUMP,
        BIN_FILE_RETURN,
        AUX_MEMSET,
        DUMP_LOCAL_MEM,
        DUMP_REGISTER_FILE,
        WAIT_STEP,
        PRINTF,
        AUX_PRINT_FIFO,
        AUX_DEV_NULL
    } type;

    // bin_file_send + bin_file_dump + bin_file_return
    uint32_t addr;
    uint32_t num_bytes;

    // bin_file_send + bin_file_dump
    char* file_name;

    // printf
    char* string;

    // memset
    uint8_t value;
    uint32_t data;

    // dump
    uint32_t cluster;
    uint32_t unit;
    uint32_t lane;

    CommandSim();
    CommandSim(CommandSim* ref);
    CommandSim(CommandSim& ref);

    bool is_done() override;
    void print(FILE* out = stdout) override;

    // String returning functions
    QString get_type() override {
        switch (type) {
            case NONE:
                return {"NONE           "};
            case BIN_FILE_SEND:
                return {"BIN_FILE_SEND  "};
            case BIN_FILE_DUMP:
                return {"BIN_FILE_DUMP  "};
            case BIN_FILE_RETURN:
                return {"BIN_FILE_RETURN"};
            case AUX_MEMSET:
                return {"AUX_MEMSET     "};
            case DUMP_LOCAL_MEM:
                return {"DUMP_LOC_MEM   "};
            case DUMP_REGISTER_FILE:
                return {"DUMP_REG_FILE  "};
            case WAIT_STEP:
                return {"WAIT_STEP      "};
            case PRINTF:
                return {"PRINTF         "};
            case AUX_PRINT_FIFO:
                return {"AUX_PRINT_FIFO "};
            case AUX_DEV_NULL:
                return {"AUX_DEV_NULL   "};
        }
        return {""};
    }

    QString get_string() override;

    static void printType(CommandSim::TYPE t, FILE* out = stdout) {
        switch (t) {
            case NONE:
                fprintf(out, "NONE           ");
                break;
            case BIN_FILE_SEND:
                fprintf(out, "BIN_FILE_SEND  ");
                break;
            case BIN_FILE_DUMP:
                fprintf(out, "BIN_FILE_DUMP  ");
                break;
            case BIN_FILE_RETURN:
                fprintf(out, "BIN_FILE_RETURN");
                break;
            case AUX_MEMSET:
                fprintf(out, "AUX_MEMSET     ");
                break;
            case DUMP_LOCAL_MEM:
                fprintf(out, "DUMP_LOC_MEM   ");
                break;
            case DUMP_REGISTER_FILE:
                fprintf(out, "DUMP_REG_FILE  ");
                break;
            case WAIT_STEP:
                fprintf(out, "WAIT_STEP      ");
                break;
            case PRINTF:
                fprintf(out, "PRINTF         ");
                break;
            case AUX_PRINT_FIFO:
                fprintf(out, "AUX_PRINT_FIFO ");
                break;
            case AUX_DEV_NULL:
                fprintf(out, "AUX_DEV_NULL   ");
                break;
        }
    }
};

#endif  //VPRO_CPP_COMMANDSIM_H
