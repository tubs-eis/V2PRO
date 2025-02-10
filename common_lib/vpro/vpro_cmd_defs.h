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
// #################################################
// # VPRO instrinsics auxiliary library            #
// # --------------------------------------------- #
// # Stephan Nolting, IMS, Uni Hannover, 2017-2019 #
// #################################################

#ifndef isa_intrinsic_aux_lib
#define isa_intrinsic_aux_lib

#include <stdint.h>
#include <stdlib.h>     /* abs */
#include <inttypes.h>
#include <assert.h>
#include <stdlib.h>

// Right place to hide
// https://stackoverflow.com/questions/18165527/linker-unable-to-find-assert-fail
#pragma GCC visibility push(hidden)

#define LM_BASE_VU(u)   (u*(1<<22)) // lm base address of VU "u"


// -----------------------------------------------------------------------------
// OPCODE definitions
// -----------------------------------------------------------------------------
enum LANE{
    L0      = 0b001,
    L1      = 0b010,
    L0_1    = 0b011,
    LS      = 0b100 // some high bit
};


#define BLOCKING        0b1
#define BL              BLOCKING
#define NONBLOCKING     0b0
#define NBL             NONBLOCKING

// is source of chain?
#define IS_CHAIN        0b1
#define CH              IS_CHAIN
#define NO_CHAIN        0b0
#define NCH             NO_CHAIN

// function class + opcode
constexpr uint32_t FUNC(uint32_t class_, uint32_t opcode){
    return (class_ << 4) | opcode;
}
#define CLASS_MEM       0b00
#define CLASS_ALU       0b01
#define CLASS_SPECIAL   0b10
#define CLASS_TRANSFER  0b11
#define CLASS_OTHER     99          // no HW representation / support

// function opcodes
// MEM class
#define OPCODE_LOAD                 0b0000
#define OPCODE_LOADB                0b0001
#define OPCODE_LOADS                0b0010
#define OPCODE_LOADBS               0b0011
#define OPCODE_LOADS_SHIFT_LEFT     0b0110
#define OPCODE_LOADS_SHIFT_RIGHT    0b0111
#define OPCODE_LOAD_REVERSE         0b0101
#define OPCODE_STORE                0b1000
#define OPCODE_STORE_SHIFT_LEFT     0b1001
#define OPCODE_STORE_SHIFT_RIGHT    0b1010
#define OPCODE_STORE_REVERSE        0b1011
// actual operation ("function macro")
#define FUNC_LOAD               FUNC(CLASS_MEM, OPCODE_LOAD)
#define FUNC_LOADS              FUNC(CLASS_MEM, OPCODE_LOADS)
#define FUNC_LOADB              FUNC(CLASS_MEM, OPCODE_LOADB)
#define FUNC_LOADBS             FUNC(CLASS_MEM, OPCODE_LOADBS)
#define FUNC_LOAD_SHIFT_RIGHT   FUNC(CLASS_MEM, OPCODE_LOADS_SHIFT_RIGHT)
#define FUNC_LOAD_SHIFT_LEFT    FUNC(CLASS_MEM, OPCODE_LOADS_SHIFT_LEFT)
#define FUNC_LOAD_REVERSE       FUNC(CLASS_MEM, OPCODE_LOAD_REVERSE)

#define FUNC_STORE              FUNC(CLASS_MEM, OPCODE_STORE)
#define FUNC_STORE_SHIFT_RIGHT  FUNC(CLASS_MEM, OPCODE_STORE_SHIFT_RIGHT)
#define FUNC_STORE_SHIFT_LEFT   FUNC(CLASS_MEM, OPCODE_STORE_SHIFT_LEFT)
#define FUNC_STORE_REVERSE      FUNC(CLASS_MEM, OPCODE_STORE_REVERSE)

// ALU class
#define OPCODE_ADD          0b0000
#define OPCODE_SUB          0b0001
#define OPCODE_MACL_PRE     0b0010
#define OPCODE_MACH_PRE     0b0011
#define OPCODE_MULL         0b0100
#define OPCODE_MACL         0b0101
#define OPCODE_MULH         0b0110
#define OPCODE_MACH         0b0111

#define OPCODE_XOR          0b1000
#define OPCODE_XNOR         0b1001
#define OPCODE_AND          0b1010
//#define OPCODE_ANDN         0b1011  // not supported any longer!
#define OPCODE_NAND         0b1100
#define OPCODE_OR           0b1101
//#define OPCODE_ORN          0b1110  // not supported any longer!
#define OPCODE_NOR          0b1111

#define OPCODE_DIVL         0b10000
#define OPCODE_DIVH         0b10001
// actual operation ("function macro")
#define FUNC_ADD      FUNC(CLASS_ALU, OPCODE_ADD)
#define FUNC_SUB      FUNC(CLASS_ALU, OPCODE_SUB)
#define FUNC_MULL     FUNC(CLASS_ALU, OPCODE_MULL)
#define FUNC_MULH     FUNC(CLASS_ALU, OPCODE_MULH)
#define FUNC_DIVL	  FUNC(CLASS_ALU, OPCODE_DIVL)    // no synth
#define FUNC_DIVH     FUNC(CLASS_ALU, OPCODE_DIVH)    // no synth - 5 bit
#define FUNC_MACL     FUNC(CLASS_ALU, OPCODE_MACL)
#define FUNC_MACH     FUNC(CLASS_ALU, OPCODE_MACH)
#define FUNC_MACL_PRE FUNC(CLASS_ALU, OPCODE_MACL_PRE)
#define FUNC_MACH_PRE FUNC(CLASS_ALU, OPCODE_MACH_PRE)
#define FUNC_XOR      FUNC(CLASS_ALU, OPCODE_XOR)
#define FUNC_XNOR     FUNC(CLASS_ALU, OPCODE_XNOR)
#define FUNC_AND      FUNC(CLASS_ALU, OPCODE_AND)
//#define FUNC_ANDN     FUNC(CLASS_ALU, OPCODE_ANDN) // not supported any longer!
#define FUNC_NAND     FUNC(CLASS_ALU, OPCODE_NAND)
#define FUNC_OR       FUNC(CLASS_ALU, OPCODE_OR)
//#define FUNC_ORN      FUNC(CLASS_ALU, OPCODE_ORN) // not supported any longer!
#define FUNC_NOR      FUNC(CLASS_ALU, OPCODE_NOR)

// Special class
#define OPCODE_SHIFT_LL     0b0000
#define OPCODE_SHIFT_LR     0b0001
#define OPCODE_SHIFT_AR     0b0011
#define OPCODE_ABS          0b0100
#define OPCODE_MIN          0b0110
#define OPCODE_MAX          0b0111
#define OPCODE_MAX_VECTOR   0b1101
#define OPCODE_MIN_VECTOR   0b1110
#define OPCODE_BIT_REVERSAL 0b1111
// actual operation ("function macro")
#define FUNC_SHIFT_LL     FUNC(CLASS_SPECIAL, OPCODE_SHIFT_LL)
#define FUNC_SHIFT_LR     FUNC(CLASS_SPECIAL, OPCODE_SHIFT_LR)
#define FUNC_SHIFT_AR     FUNC(CLASS_SPECIAL, OPCODE_SHIFT_AR)
#define FUNC_ABS          FUNC(CLASS_SPECIAL, OPCODE_ABS)
#define FUNC_MIN          FUNC(CLASS_SPECIAL, OPCODE_MIN)
#define FUNC_MAX          FUNC(CLASS_SPECIAL, OPCODE_MAX)
#define FUNC_MIN_VECTOR   FUNC(CLASS_SPECIAL, OPCODE_MIN_VECTOR)
#define FUNC_MAX_VECTOR   FUNC(CLASS_SPECIAL, OPCODE_MAX_VECTOR)
#define FUNC_BIT_REVERSAL FUNC(CLASS_SPECIAL, OPCODE_BIT_REVERSAL)

// Transfer class
#define OPCODE_MV_ZE        0b0000
#define OPCODE_MV_NZ        0b0001
#define OPCODE_MV_MI        0b0010
#define OPCODE_MV_PL        0b0011
#define OPCODE_MULL_NEG     0b0100
#define OPCODE_MULL_POS     0b0101
#define OPCODE_MULH_NEG     0b0110
#define OPCODE_MULH_POS     0b0111
#define OPCODE_SHIFT_AR_NEG 0b1010
#define OPCODE_SHIFT_AR_POS 0b1011

//#define OPCODE_VARIABLE_VPRO_INSTRUCTION     0b1111 // replaced by io register content in subsystem (todo...)
//#define FUNC_VARIABLE_VPRO_INSTRUCTION    CLASS_TRANSFER, OPCODE_VARIABLE_VPRO_INSTRUCTION // replaced by io register content in subsystem (todo...)

// actual operation ("function macro")
#define FUNC_MV_ZE       FUNC(CLASS_TRANSFER, OPCODE_MV_ZE)
#define FUNC_MV_NZ       FUNC(CLASS_TRANSFER, OPCODE_MV_NZ)
#define FUNC_MV_MI       FUNC(CLASS_TRANSFER, OPCODE_MV_MI)
#define FUNC_MV_PL       FUNC(CLASS_TRANSFER, OPCODE_MV_PL)
#define FUNC_MULL_NEG    FUNC(CLASS_TRANSFER, OPCODE_MULL_NEG)
#define FUNC_MULL_POS    FUNC(CLASS_TRANSFER, OPCODE_MULL_POS)
#define FUNC_MULH_NEG    FUNC(CLASS_TRANSFER, OPCODE_MULH_NEG)
#define FUNC_MULH_POS    FUNC(CLASS_TRANSFER, OPCODE_MULH_POS)
#define FUNC_SHIFT_AR_NEG FUNC(CLASS_TRANSFER, OPCODE_SHIFT_AR_NEG)
#define FUNC_SHIFT_AR_POS FUNC(CLASS_TRANSFER, OPCODE_SHIFT_AR_POS)
//#define FUNC_ADD_NEG    FUNC(CLASS_TRANSFER, OPCODE_MULL_NEG)
//#define FUNC_ADD_POS    FUNC(CLASS_TRANSFER, OPCODE_MULL_POS)

// status flag update
#define FLAG_UPDATE    0b1
#define FU             FLAG_UPDATE
#define NO_FLAG_UPDATE 0b0
#define NFU            NO_FLAG_UPDATE

// src selection
#define SRC_SEL_ADDR            0b000
#define SRC_SEL_IMM             0b001
#define SRC_SEL_LS              0b010
#define SRC_SEL_LEFT            0b011
#define SRC_SEL_RIGHT           0b100
#define SRC_SEL_INDIRECT_LS     0b101
#define SRC_SEL_INDIRECT_LEFT   0b110
#define SRC_SEL_INDIRECT_RIGHT  0b111

/**
 * defines ISA attributes
 * x_end, y_end limit vector length
 *  [e.g. 4-bit used in instruction => max 15]
 * alpha, beta and offset are used for complex addressing
 *  [e.g. 5-bit used for each occurence in instruction => max 31]
 */
// 6-bit -> max 63
// 5-bit -> max 31
// 4-bit -> max 15

constexpr unsigned int ISA_X_END_LENGTH = 6;
constexpr unsigned int ISA_Y_END_LENGTH = 6;
constexpr unsigned int ISA_Z_END_LENGTH = 10;

constexpr unsigned int ISA_BETA_LENGTH = 6;
constexpr unsigned int ISA_ALPHA_LENGTH = 6;
constexpr unsigned int ISA_GAMMA_LENGTH = 6;
constexpr unsigned int ISA_OFFSET_LENGTH = 10;

/**
 * highest possible value for x_end
 */
constexpr unsigned int MAX_X_END  = (1u << ISA_X_END_LENGTH) - 1;   // checked in simCore.cpp
/**
 * highest possible value for y_end
 */
constexpr unsigned int MAX_Y_END  = (1u << ISA_Y_END_LENGTH) - 1;   // checked in simCore.cpp
/**
 * highest possible value for z_end
 */
constexpr unsigned int MAX_Z_END  = (1u << ISA_Z_END_LENGTH) - 1;   // checked in simCore.cpp

constexpr unsigned int MAX_BETA   = (1u << ISA_BETA_LENGTH) - 1;    // checked here
constexpr unsigned int MAX_ALPHA  = (1u << ISA_ALPHA_LENGTH) - 1;   // checked here
constexpr unsigned int MAX_GAMMA  = (1u << ISA_GAMMA_LENGTH) - 1;   // checked here
constexpr unsigned int MAX_OFFSET = (1u << ISA_OFFSET_LENGTH) - 1;  // checked here

// start of each parameter
constexpr unsigned int ISA_ALPHA_SHIFT_3D = ISA_GAMMA_LENGTH+ISA_BETA_LENGTH;
constexpr unsigned int ISA_BETA_SHIFT_3D = ISA_GAMMA_LENGTH;
constexpr unsigned int ISA_GAMMA_SHIFT_3D = 0;
constexpr unsigned int ISA_OFFSET_SHIFT_3D = ISA_ALPHA_LENGTH + ISA_BETA_LENGTH + ISA_GAMMA_LENGTH;

constexpr unsigned int ISA_ALPHA_SHIFT_2D = ISA_BETA_LENGTH;
constexpr unsigned int ISA_BETA_SHIFT_2D = 0;
constexpr unsigned int ISA_OFFSET_SHIFT_2D = ISA_ALPHA_LENGTH + ISA_BETA_LENGTH;

constexpr unsigned int ISA_COMPLEX_LENGTH_3D = ISA_OFFSET_SHIFT_3D + ISA_OFFSET_LENGTH; // equal immediate width... e.g. 20-bit
constexpr unsigned int ISA_COMPLEX_LENGTH_2D = ISA_OFFSET_SHIFT_2D + ISA_OFFSET_LENGTH; // equal immediate width... e.g. 20-bit

// for sim core, to return to 3 values from single uint32 here
constexpr unsigned int ISA_ALPHA_MASK = 0xffffffffu >> (32-ISA_ALPHA_LENGTH);
constexpr unsigned int ISA_BETA_MASK = 0xffffffffu >> (32-ISA_BETA_LENGTH);
constexpr unsigned int ISA_GAMMA_MASK = 0xffffffffu >> (32-ISA_GAMMA_LENGTH);
constexpr unsigned int ISA_OFFSET_MASK = 0xffffffffu >> (32-ISA_OFFSET_LENGTH);

// '1' to perform a sign extension e.g. for 20-bit: 0xFFF00000
constexpr unsigned int signed_TOP_FILL_3D = 0xffffffff << ISA_COMPLEX_LENGTH_3D;
constexpr unsigned int signed_TOP_FILL_2D = 0xffffffff << ISA_COMPLEX_LENGTH_2D;
constexpr unsigned int ISA_COMPLEX_MASK_3D = 0xffffffffu >> (32-ISA_COMPLEX_LENGTH_3D);
constexpr unsigned int ISA_COMPLEX_MASK_2D = 0xffffffffu >> (32-ISA_COMPLEX_LENGTH_2D);

// -----------------------------------------------------------------------------
// Instruction macros
// -----------------------------------------------------------------------------
constexpr uint32_t complex_ADDR_3D(uint32_t sel, uint32_t offset, uint32_t alpha, uint32_t beta, uint32_t gamma) {
assert (offset <= MAX_OFFSET);
assert (alpha <= MAX_ALPHA);
assert (beta <= MAX_BETA);
assert (gamma <= MAX_GAMMA);
unsigned int concat = (sel << ISA_COMPLEX_LENGTH_3D) | (offset << ISA_OFFSET_SHIFT_3D) | (alpha << ISA_ALPHA_SHIFT_3D) | (beta << ISA_BETA_SHIFT_3D) | (gamma << ISA_GAMMA_SHIFT_3D);
return concat;
}
constexpr uint32_t complex_ADDR_2D(uint32_t sel, uint32_t offset, uint32_t alpha, uint32_t beta) {
assert (offset <= MAX_OFFSET);
assert (alpha <= MAX_ALPHA);
assert (beta <= MAX_BETA);
unsigned int concat = (sel << ISA_COMPLEX_LENGTH_2D) | (offset << ISA_OFFSET_SHIFT_2D) | (alpha << ISA_ALPHA_SHIFT_2D) | (beta << ISA_BETA_SHIFT_2D);
return concat;
}

// General utility macro, Macro overloading feature support
#define PP_CAT( A, B ) A ## B
#define PP_EXPAND(...) __VA_ARGS__
#define PP_VA_ARG_SIZE(...) PP_EXPAND(PP_APPLY_ARG_N((PP_ZERO_ARGS_DETECT(__VA_ARGS__), PP_RSEQ_N)))
#define PP_ZERO_ARGS_DETECT(...) PP_EXPAND(PP_ZERO_ARGS_DETECT_PREFIX_ ## __VA_ARGS__ ## _ZERO_ARGS_DETECT_SUFFIX)
#define PP_ZERO_ARGS_DETECT_PREFIX__ZERO_ARGS_DETECT_SUFFIX ,,,,,,,,,,,0
#define PP_APPLY_ARG_N(ARGS) PP_EXPAND(PP_ARG_N ARGS)
#define PP_ARG_N(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N,...) N
#define PP_RSEQ_N 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define PP_OVERLOAD_SELECT(NAME, NUM) PP_CAT( NAME ## _, NUM)
#define PP_MACRO_OVERLOAD(NAME, ...) PP_OVERLOAD_SELECT(NAME, PP_VA_ARG_SIZE(__VA_ARGS__))(__VA_ARGS__)

// destination
#define DST_ADDR(...) PP_MACRO_OVERLOAD(DST_ADDR, __VA_ARGS__)
#define DST_ADDR_3(offset, alpha, beta)         complex_ADDR_2D(SRC_SEL_ADDR, offset, alpha, beta)
#define DST_ADDR_4(offset, alpha, beta, gamma)  complex_ADDR_3D(SRC_SEL_ADDR, offset, alpha, beta, gamma)

#define DST_DONT_CARE                  0

// indirect destination addressing: chain from LS lane
#define DST_INDIRECT_LS(...) PP_MACRO_OVERLOAD(DST_INDIRECT_LS, __VA_ARGS__)
#define DST_INDIRECT_LS_0()  complex_ADDR_3D((uint32_t) SRC_SEL_INDIRECT_LS, 0, 0, 0, 0)
#define DST_INDIRECT_LS_3(alpha, beta, gamma) complex_ADDR_3D((uint32_t) SRC_SEL_INDIRECT_LS, 0, alpha, beta, gamma)

// indirect destination addressing: chain from left neighbor
#define DST_INDIRECT_LEFT(...) PP_MACRO_OVERLOAD(DST_INDIRECT_LEFT, __VA_ARGS__)
#define DST_INDIRECT_LEFT_0()  complex_ADDR_3D((uint32_t) SRC_SEL_INDIRECT_LEFT, 0, 0, 0, 0)
#define DST_INDIRECT_LEFT_3(alpha, beta, gamma) complex_ADDR_3D((uint32_t) SRC_SEL_INDIRECT_LEFT, 0, alpha, beta, gamma)

// indirect destination addressing: chain from right neighbor
#define DST_INDIRECT_RIGHT(...) PP_MACRO_OVERLOAD(DST_INDIRECT_RIGHT, __VA_ARGS__)
#define DST_INDIRECT_RIGHT_0()  complex_ADDR_3D((uint32_t) SRC_SEL_INDIRECT_RIGHT, 0, 0, 0, 0)
#define DST_INDIRECT_RIGHT_3(alpha, beta, gamma) complex_ADDR_3D((uint32_t) SRC_SEL_INDIRECT_RIGHT, 0, alpha, beta, gamma)

// source
#define SRC_ADDR(...) PP_MACRO_OVERLOAD(SRC_ADDR, __VA_ARGS__)
#define SRC1_ADDR(...) PP_MACRO_OVERLOAD(SRC_ADDR, __VA_ARGS__)
#define SRC2_ADDR(...) PP_MACRO_OVERLOAD(SRC_ADDR, __VA_ARGS__)

#define SRC_ADDR_3(offset, alpha, beta)         complex_ADDR_2D(SRC_SEL_ADDR, offset, alpha, beta)
#define SRC_ADDR_4(offset, alpha, beta, gamma)  complex_ADDR_3D(SRC_SEL_ADDR, offset, alpha, beta, gamma)

// indirect source addressing: chain from LS lane
#define SRC_INDIRECT_LS(...) PP_MACRO_OVERLOAD(SRC_INDIRECT_LS, __VA_ARGS__)
#define SRC1_INDIRECT_LS(...) PP_MACRO_OVERLOAD(SRC_INDIRECT_LS, __VA_ARGS__)
#define SRC2_INDIRECT_LS(...) PP_MACRO_OVERLOAD(SRC_INDIRECT_LS, __VA_ARGS__)

#define SRC_INDIRECT_LS_0()  complex_ADDR_3D((uint32_t) SRC_SEL_INDIRECT_LS, 0, 0, 0, 0)
#define SRC_INDIRECT_LS_3(alpha, beta, gamma)  complex_ADDR_3D((uint32_t) SRC_SEL_INDIRECT_LS, 0, alpha, beta, gamma)

// indirect source addressing: chain from left neighbor
#define SRC_INDIRECT_LEFT(...) PP_MACRO_OVERLOAD(SRC_INDIRECT_LEFT, __VA_ARGS__)
#define SRC1_INDIRECT_LEFT(...) PP_MACRO_OVERLOAD(SRC_INDIRECT_LEFT, __VA_ARGS__)
#define SRC2_INDIRECT_LEFT(...) PP_MACRO_OVERLOAD(SRC_INDIRECT_LEFT, __VA_ARGS__)

#define SRC_INDIRECT_LEFT_0()  complex_ADDR_3D((uint32_t) SRC_SEL_INDIRECT_LEFT, 0, 0, 0, 0)
#define SRC_INDIRECT_LEFT_3(alpha, beta, gamma)  complex_ADDR_3D((uint32_t) SRC_SEL_INDIRECT_LEFT, 0, alpha, beta, gamma)

// indirect source addressing: chain from right neighbor
#define SRC_INDIRECT_RIGHT(...) PP_MACRO_OVERLOAD(SRC_INDIRECT_RIGHT, __VA_ARGS__)
#define SRC1_INDIRECT_RIGHT(...) PP_MACRO_OVERLOAD(SRC_INDIRECT_RIGHT, __VA_ARGS__)
#define SRC2_INDIRECT_RIGHT(...) PP_MACRO_OVERLOAD(SRC_INDIRECT_RIGHT, __VA_ARGS__)
#define SRC_INDIRECT_RIGHT_0()  complex_ADDR_3D((uint32_t) SRC_SEL_INDIRECT_RIGHT, 0, 0, 0, 0)
#define SRC_INDIRECT_RIGHT_3(alpha, beta, gamma)  complex_ADDR_3D((uint32_t) SRC_SEL_INDIRECT_RIGHT, 0, alpha, beta, gamma)

// chanining lane id (LS source)
constexpr unsigned int ISA_CHAIN_ID_MASK_2D = 0xffffffffu >> (32-ISA_COMPLEX_LENGTH_2D-1); // to extract id in sim core
constexpr unsigned int ISA_CHAIN_ID_MASK_3D = 0xffffffffu >> (32-ISA_COMPLEX_LENGTH_3D-1); // to extract id in sim core

// the upper bit of src (imm) are used to differ between neighbor and id chain source
// use chaining from Lane with ID...
// using 10 bit / offset -> select one of 1024 lanes!
constexpr uint32_t CHAIN_ID_2D(uint32_t id){
    assert(id < (1u<<(10-1)));
    return (id << ISA_BETA_SHIFT_2D);
}
constexpr uint32_t CHAIN_ID_3D(uint32_t id){
    assert(id < (1u<<(10-1)));
    return (id << ISA_GAMMA_SHIFT_3D);
}
constexpr uint32_t SRC_CHAINING_2D(uint32_t id) {
    return (SRC_SEL_LEFT << ISA_COMPLEX_LENGTH_2D) | CHAIN_ID_2D(id);
}
constexpr uint32_t SRC_CHAINING_3D(uint32_t id) {
    return (SRC_SEL_LEFT << ISA_COMPLEX_LENGTH_3D) | CHAIN_ID_3D(id);
}

constexpr uint32_t SRC1_CHAINING_2D(uint32_t id) { return SRC_CHAINING_2D(id); }
constexpr uint32_t SRC2_CHAINING_2D(uint32_t id) { return SRC_CHAINING_2D(id); }
constexpr uint32_t SRC1_CHAINING_3D(uint32_t id) { return SRC_CHAINING_3D(id); }
constexpr uint32_t SRC2_CHAINING_3D(uint32_t id) { return SRC_CHAINING_3D(id); }

// immediates
constexpr uint32_t SRC_IMM_2D(uint32_t imm, uint32_t SEL = SRC_SEL_IMM) {
#ifdef SIMULATION
    assert (abs(int32_t(imm)) < (1<<ISA_COMPLEX_LENGTH_2D));
#endif
    // cut immediate to ISA_COMPLEX_LENGTH_2D bits
    return (SEL << ISA_COMPLEX_LENGTH_2D) | (imm & ((1 << ISA_COMPLEX_LENGTH_2D) - 1));
}
/**
 * even if more bits available, use only 22 (2D) to reduce hardware multi version support   // TODO? IMM for 3D extended?
 */
constexpr uint32_t SRC_IMM_3D(uint32_t imm, uint32_t SEL = SRC_SEL_IMM) {
#ifdef SIMULATION
    assert (abs(int32_t(imm)) < (1<<ISA_COMPLEX_LENGTH_3D));
#endif
    // cut immediate to ISA_COMPLEX_LENGTH_3D bits
    return (SEL << ISA_COMPLEX_LENGTH_3D) | (imm & ((1 << ISA_COMPLEX_LENGTH_3D) - 1));
}
//constexpr uint32_t SRC_IMM(uint32_t imm) { return SRC_IMM_2D(imm); }
//constexpr uint32_t SRC1_IMM(uint32_t imm) { return SRC_IMM_2D(imm); }
//constexpr uint32_t SRC2_IMM(uint32_t imm) { return SRC_IMM_2D(imm); }

constexpr uint32_t SRC1_IMM_2D(uint32_t imm) { return SRC_IMM_2D(imm); }
constexpr uint32_t SRC2_IMM_2D(uint32_t imm) { return SRC_IMM_2D(imm); }
constexpr uint32_t SRC1_IMM_3D(uint32_t imm) { return SRC_IMM_3D(imm); }
constexpr uint32_t SRC2_IMM_3D(uint32_t imm) { return SRC_IMM_3D(imm); }

constexpr uint32_t PREPEND_SEL_2D(uint32_t SEL, uint32_t imm=0){
    return (SEL << ISA_COMPLEX_LENGTH_2D) | imm;
}
constexpr uint32_t PREPEND_SEL_3D(uint32_t SEL, uint32_t imm=0){
    return (SEL << ISA_COMPLEX_LENGTH_3D) | imm;
}

/*
 * 2D
 */
// use chaining from left side
constexpr uint32_t SRC_CHAINING_LEFT_2D = PREPEND_SEL_2D(SRC_SEL_LEFT);
// use chaining from right side
constexpr uint32_t SRC_CHAINING_RIGHT_2D = PREPEND_SEL_2D(SRC_SEL_RIGHT);
// use chaining from left side, dont start chain yet
constexpr uint32_t  SRC_CHAINING_LEFT_DELAYED_2D = PREPEND_SEL_2D(SRC_SEL_LEFT, 0x8102);
// use chaining from right side, dont start chain yet
constexpr uint32_t  SRC_CHAINING_RIGHT_DELAYED_2D = PREPEND_SEL_2D(SRC_SEL_RIGHT, 0x8202);
// use chaining from local LS
constexpr uint32_t SRC_LS_2D = PREPEND_SEL_2D(SRC_SEL_LS);
// use chaining from left side
constexpr uint32_t SRC_LS_LEFT_2D = PREPEND_SEL_2D(SRC_SEL_LS, 0x8100);
// use chaining from left side
constexpr uint32_t  SRC_LS_LEFT_DELAYED_2D = PREPEND_SEL_2D(SRC_SEL_LS, 0x8102);
// use chaining from right side
constexpr uint32_t  SRC_LS_RIGHT_2D = PREPEND_SEL_2D(SRC_SEL_LS, 0x8200);
// use chaining from right side
constexpr uint32_t  SRC_LS_RIGHT_DELAYED_2D = PREPEND_SEL_2D(SRC_SEL_LS, 0x8202);
// use chaining from local LS but not starting
constexpr uint32_t  SRC_LS_DELAYED_2D = PREPEND_SEL_2D(SRC_SEL_LS, 0x0002);
constexpr uint32_t  SRC_DONT_CARE_2D = PREPEND_SEL_2D(SRC_SEL_ADDR);

constexpr uint32_t  SRC1_CHAINING_LEFT_2D = SRC_CHAINING_LEFT_2D;
constexpr uint32_t  SRC1_CHAINING_RIGHT_2D = SRC_CHAINING_RIGHT_2D;
constexpr uint32_t  SRC1_CHAINING_LEFT_DELAYED_2D = SRC_CHAINING_LEFT_DELAYED_2D;
constexpr uint32_t  SRC1_CHAINING_RIGHT_DELAYED_2D = SRC_CHAINING_RIGHT_DELAYED_2D;
constexpr uint32_t  SRC1_LS_2D = SRC_LS_2D;
constexpr uint32_t  SRC1_LS_LEFT_2D = SRC_LS_LEFT_2D;
constexpr uint32_t  SRC1_LS_LEFT_DELAYED_2D = SRC_LS_LEFT_DELAYED_2D;
constexpr uint32_t  SRC1_LS_RIGHT_2D = SRC_LS_RIGHT_2D;
constexpr uint32_t  SRC1_LS_RIGHT_DELAYED_2D = SRC_LS_RIGHT_DELAYED_2D;
constexpr uint32_t  SRC1_LS_DELAYED_2D = SRC_LS_DELAYED_2D;
constexpr uint32_t  SRC1_DONT_CARE_2D = SRC_DONT_CARE_2D;

constexpr uint32_t  SRC2_CHAINING_LEFT_2D = SRC_CHAINING_LEFT_2D;
constexpr uint32_t  SRC2_CHAINING_RIGHT_2D = SRC_CHAINING_RIGHT_2D;
constexpr uint32_t  SRC2_CHAINING_LEFT_DELAYED_2D = SRC_CHAINING_LEFT_DELAYED_2D;
constexpr uint32_t  SRC2_CHAINING_RIGHT_DELAYED_2D = SRC_CHAINING_RIGHT_DELAYED_2D;
constexpr uint32_t  SRC2_LS_2D = SRC_LS_2D;
constexpr uint32_t  SRC2_LS_LEFT_2D = SRC_LS_LEFT_2D;
constexpr uint32_t  SRC2_LS_LEFT_DELAYED_2D = SRC_LS_LEFT_DELAYED_2D;
constexpr uint32_t  SRC2_LS_RIGHT_2D = SRC_LS_RIGHT_2D;
constexpr uint32_t  SRC2_LS_RIGHT_DELAYED_2D = SRC_LS_RIGHT_DELAYED_2D;
constexpr uint32_t  SRC2_LS_DELAYED_2D = SRC_LS_DELAYED_2D;
constexpr uint32_t  SRC2_DONT_CARE_2D = SRC_DONT_CARE_2D;


/*
 * 3D
 */
// use chaining from left side
constexpr uint32_t SRC_CHAINING_LEFT_3D = PREPEND_SEL_3D(SRC_SEL_LEFT);
// use chaining from right side
constexpr uint32_t SRC_CHAINING_RIGHT_3D = PREPEND_SEL_3D(SRC_SEL_RIGHT);
// use chaining from left side, dont start chain yet
constexpr uint32_t  SRC_CHAINING_LEFT_DELAYED_3D = PREPEND_SEL_3D(SRC_SEL_LEFT, 0x8102);
// use chaining from right side, dont start chain yet
constexpr uint32_t  SRC_CHAINING_RIGHT_DELAYED_3D = PREPEND_SEL_3D(SRC_SEL_RIGHT, 0x8202);
// use chaining from local LS
constexpr uint32_t SRC_LS_3D = PREPEND_SEL_3D(SRC_SEL_LS, 0);
// use chaining from left side
constexpr uint32_t SRC_LS_LEFT_3D = PREPEND_SEL_3D(SRC_SEL_LS, 0x8100);
// use chaining from left side
constexpr uint32_t  SRC_LS_LEFT_DELAYED_3D = PREPEND_SEL_3D(SRC_SEL_LS, 0x8102);
// use chaining from right side
constexpr uint32_t  SRC_LS_RIGHT_3D = PREPEND_SEL_3D(SRC_SEL_LS, 0x8200);
// use chaining from right side
constexpr uint32_t  SRC_LS_RIGHT_DELAYED_3D = PREPEND_SEL_3D(SRC_SEL_LS, 0x8202);
// use chaining from local LS but not starting
constexpr uint32_t  SRC_LS_DELAYED_3D = PREPEND_SEL_3D(SRC_SEL_LS, 0x0002);
constexpr uint32_t  SRC_DONT_CARE_3D = PREPEND_SEL_3D(SRC_SEL_ADDR, 0);

constexpr uint32_t  SRC1_CHAINING_LEFT_3D = SRC_CHAINING_LEFT_3D;
constexpr uint32_t  SRC1_CHAINING_RIGHT_3D = SRC_CHAINING_RIGHT_3D;
constexpr uint32_t  SRC1_CHAINING_LEFT_DELAYED_3D = SRC_CHAINING_LEFT_DELAYED_3D;
constexpr uint32_t  SRC1_CHAINING_RIGHT_DELAYED_3D = SRC_CHAINING_RIGHT_DELAYED_3D;
constexpr uint32_t  SRC1_LS_3D = SRC_LS_3D;
constexpr uint32_t  SRC1_LS_LEFT_3D = SRC_LS_LEFT_3D;
constexpr uint32_t  SRC1_LS_LEFT_DELAYED_3D = SRC_LS_LEFT_DELAYED_3D;
constexpr uint32_t  SRC1_LS_RIGHT_3D = SRC_LS_RIGHT_3D;
constexpr uint32_t  SRC1_LS_RIGHT_DELAYED_3D = SRC_LS_RIGHT_DELAYED_3D;
constexpr uint32_t  SRC1_LS_DELAYED_3D = SRC_LS_DELAYED_3D;
constexpr uint32_t  SRC1_DONT_CARE_3D = SRC_DONT_CARE_3D;

constexpr uint32_t  SRC2_CHAINING_LEFT_3D = SRC_CHAINING_LEFT_3D;
constexpr uint32_t  SRC2_CHAINING_RIGHT_3D = SRC_CHAINING_RIGHT_3D;
constexpr uint32_t  SRC2_CHAINING_LEFT_DELAYED_3D = SRC_CHAINING_LEFT_DELAYED_3D;
constexpr uint32_t  SRC2_CHAINING_RIGHT_DELAYED_3D = SRC_CHAINING_RIGHT_DELAYED_3D;
constexpr uint32_t  SRC2_LS_3D = SRC_LS_3D;
constexpr uint32_t  SRC2_LS_LEFT_3D = SRC_LS_LEFT_3D;
constexpr uint32_t  SRC2_LS_LEFT_DELAYED_3D = SRC_LS_LEFT_DELAYED_3D;
constexpr uint32_t  SRC2_LS_RIGHT_3D = SRC_LS_RIGHT_3D;
constexpr uint32_t  SRC2_LS_RIGHT_DELAYED_3D = SRC_LS_RIGHT_DELAYED_3D;
constexpr uint32_t  SRC2_LS_DELAYED_3D = SRC_LS_DELAYED_3D;
constexpr uint32_t  SRC2_DONT_CARE_3D = SRC_DONT_CARE_3D;

#pragma GCC visibility pop

#endif // isa_intrinsic_aux_lib
