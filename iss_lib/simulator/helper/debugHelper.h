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

#ifndef VPRO_CPP_DEBUGHELPER_H
#define VPRO_CPP_DEBUGHELPER_H

#ifndef SIMULATION
"USE common_lib/vpro.h !";
#endif

#include <string>
#include "../../model/commands/CommandDMA.h"
#include "../../model/commands/CommandVPRO.h"
#include "../setting.h"
#include "structTypes.h"

/**
* ----------------------------------------------------------------------------------------------
* Simulator debugging options
* ----------------------------------------------------------------------------------------------
* DEBUG_INSTRUCTIONS      = enable instruction issue information
* DEBUG_DMA               = enable DMA descriptors and details regarding transferred data
* DEBUG_FIFO_MSG          = enable DEBUG_FIFO messages
* DEBUG_PRINTF            = enable SIM_PRINTF console ouput
* DEBUG_USER_DUMP         = enable user debug functions (sim_dump_register_file, sim_dump_local_memory)
* DEBUG_INSTR_STATISTICS  = enable statistics regarding instrunction type usage
* DEBUG_MODE              = enable advanced debug prints (obsolete!!!)
*/
namespace VPRO {

enum DebugOptions {
    DEBUG_INSTRUCTIONS = 1 << 0,  // complete instruction print
    DEBUG_DMA = 1 << 1,           // dma transfers (load from/store to)
    DEBUG_FIFO_MSG = 1 << 2,      // values in debug_fifo (AUX)
    DEBUG_PRINTF = 1 << 3,
    DEBUG_USER_DUMP = 1 << 4,
    DEBUG_INSTR_STATISTICS = 1 << 5,
    DEBUG_MODE = 1 << 6,
    DEBUG_TICK = 1 << 7,  // clock ticks
    DEBUG_GLOBAL_TICK = 1 << 11,
    DEBUG_INSTRUCTION_SCHEDULING = 1 << 8,  // prints cluster queue scheduling...
    DEBUG_INSTRUCTION_DATA = 1 << 10,       // prints operands and result from vpro instructions
    DEBUG_PIPELINE = 1 << 9,                // prints pipeline instructions
    DEBUG_PIPELINE_9 = 1 << 12,             //finishing data in stage 9 (going into reg-file)
    DEBUG_DEV_NULL = 1 << 13,
    DEBUG_DMA_DETAIL = 1 << 14,
    DEBUG_GLOBAL_VARIABLE_CHECK =
        1
        << 15,  // print info on program start of check results for global variables (address, size of filters...)
    DEBUG_CHAINING =
        1
        << 16,  // print message on chaining access (which lane formwards which data -- print only on active access)
    DEBUG_DUMP_FLAGS =
        1 << 17,  // Whether the dumps of rf should print flags for Neg (+/-) and Zero (z/d) too
    DEBUG_INSTRUCTION_SCHEDULING_BASIC = 1 << 18,
    DEBUG_LOOPER = 1 << 19,
    DEBUG_LOOPER_DETAILED = 1 << 20,
    DEBUG_INSTR_STATISTICS_ALL = 1 << 21,
    DEBUG_DMA_ACCESS_TO_EXT_VARIABLE = 1 << 22,  // like coefficients included from .h
    DEBUG_PIPELINELENGTH_CHANGES =
        1
        << 23,  // if cmd length changes during lanes execution, print msg (lane [regular/ls] tick)
    DEBUG_LANE_STALLS = 1 << 24,  // lane tick includes stall check print msg if stall is performed
    DEBUG_DMA_DCACHE_ISSUE =
        1 << 25,  // special dma command issuing from dcache (struct in vpro/dma_command_struct.h)
    DEBUG_EXT_MEM = 1 << 26,  // DCMA/DMA access to MM (nonblocking mm impl -> not for VP)
    end = 1 << 27
};  // last: 12

}  // namespace VPRO

using namespace VPRO;

#ifdef SIMULATION
QString debugToText(DebugOptions op);
#endif

extern uint64_t debug;

/**
 * @brief Checks whether debug and the specified option is set
 *
 * @param op DEBUG_MASK which will be checked
 * @return true
 * @return false
 */
bool if_debug(DebugOptions op);

/**
 * @brief Printf the msg if debug case is enabled.
 *
 * @param op debug case
 * @param msg message to be printed
 */
void ifm_debug(DebugOptions op, const char* msg);

/**
 * @brief Debug Helper. Executes Lambda if debug case is enabled
 *
 * @tparam T Lambda Datatyp
 * @param op debug case
 * @param lambda to be executed if debug case is active.
 */
template <class T>
void ifx_debug(DebugOptions op, T lambda) {
    if (if_debug(op)) {
        lambda();
    }
};

#define REDBG "\e[41m"
#define RED "\e[91m"  // 31
#define ORANGE "\e[93m"
#define BLACK "\033[22;30m"
#define GREEN "\e[32m"
#define LGREEN "\e[92m"
#define LYELLOW "\e[93m"
#define YELLOW "\e[33m"
#define MAGENTA "\e[95m"  // or 35?
#define BLUE "\e[36m"     // 34 hard to read on thin clients
#define LBLUE "\e[96m"    // 94 hard to read on thin clients

#define INVERTED "\e[7m"
#define UNDERLINED "\e[4m"
#define BOLD "\e[1m"
#define RESET_COLOR "\e[m"

#define LIGHT "\e[2m"
//FIXME crash because of global define NORMAL and usage of same identifier in opencv/core.hpp
#define NORMAL_ "\e[22m"

int printf_warning(const char* format, ...);

int printf_error(const char* format, ...);

int printf_failure(const char* format, ...);

int printf_info(const char* format, ...);

int printf_success(const char* format, ...);

// ***********************************************************************
// Print Instruction
// ***********************************************************************
void print_cmd(CommandVPRO* cmd);

void print_cmd(CommandDMA* cmd);

void print_cmd(CommandBase* cmd);

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
    uint32_t id_mask = -1);

#endif  //VPRO_CPP_DEBUGHELPER_H
