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
// ###############################################################
// # DMA_asm.h ASSEMBLE Wrapper Functions for DMA Issueing     #
// ###############################################################

#ifndef dma_asm_h
#define dma_asm_h

//#ifndef SIMULATION
#include <cassert>
#include <cinttypes>
#include <cstdarg>
#include <array>
#include <list>
//#include <stdio.h>

/**
 * Emit of DMA Commands via special Registerfile
 * Call examples:
 * ```
 *      // to DMA[1]
 *      // trigger
 *      int a = 1, b = 0x11000000;
 *      c_dma_lw<1, cluster, ext_addr, Trigger>(a, b);
 *
 *      // to DMA[7]
 *      // all 16 parameters <= a (= 0)
 *      // no trigger
 *      c_dma_li<7, 0b1111111111111111, NoTrigger>(a);
 *  ```
 */
namespace VPRO_RISC_EXT_DMA {

#define DMA_REGISTER_INDEX_SIZE 3
#define DMA_PARAMETERS 9
#define DMA_PARAMETER_INDEX_SIZE 4
#define DMA_TRIGGER_SIZE 1

#define N_ONES(n) (-1u >> (32-(n)))
#define N_ZEROS(n) ~(-1u >> (32-(n)))

    /**
     * Inside the DMA CMD Register, these indizes can be used to modify several parameters of the cmd
     */
    enum DMA_PARAMETER_INDIZES {
        ext_addr = 0,
        int_addr = 1,
        x_size = 2,
        y_size = 3,
        x_stride = 4,
        broadcast_mask = 5,
        pad_flags = 6,
        type = 7,
        cluster = 8,/*!< =8 | The selected DMA (Cluster) */
        nowhere = 15,   /*!< Relevant for lw */
    };

    /**
     * generates the mask to modify selected parameters of the command at once
     * @param d e.g. id / func
     * @return the mask
     */
    constexpr uint32_t create_index_mask(DMA_PARAMETER_INDIZES d) {
        uint32_t result = 0;
        result |= 0b1 << d;
        return result;
    }

    constexpr uint32_t create_index_mask(DMA_PARAMETER_INDIZES d, DMA_PARAMETER_INDIZES d2) {
        uint32_t result = 0;
        result |= 0b1 << d;
        result |= 0b1 << d2;
        return result;
    }

    constexpr uint32_t
    create_index_mask(DMA_PARAMETER_INDIZES d, DMA_PARAMETER_INDIZES d2, DMA_PARAMETER_INDIZES d3) {
        uint32_t result = 0;
        result |= 0b1 << d;
        result |= 0b1 << d2;
        result |= 0b1 << d3;
        return result;
    }

    constexpr uint32_t create_index_mask(DMA_PARAMETER_INDIZES d, DMA_PARAMETER_INDIZES d2, DMA_PARAMETER_INDIZES d3,
                                         DMA_PARAMETER_INDIZES d4) {
        uint32_t result = 0;
        result |= 0b1 << d;
        result |= 0b1 << d2;
        result |= 0b1 << d3;
        result |= 0b1 << d4;
        return result;
    }

    /**
     * constexpr to generate single immediate (12 bit)
     * @param register_index
     * @param src1_target_index
     * @param src2_target_index
     * @param trigger
     * @return
     */
    constexpr int32_t dma_lw_imm(uint32_t register_index, uint32_t src1_target_index, uint32_t src2_target_index,
                                 uint8_t trigger) {
        static_assert((DMA_REGISTER_INDEX_SIZE + 2 * DMA_PARAMETER_INDEX_SIZE + DMA_TRIGGER_SIZE) <= 12);

        register_index &= N_ONES(DMA_REGISTER_INDEX_SIZE);        // 0...7
        src1_target_index &= N_ONES(DMA_PARAMETER_INDEX_SIZE);    // 0...15
        src2_target_index &= N_ONES(DMA_PARAMETER_INDEX_SIZE);    // 0...15
        trigger &= N_ONES(DMA_TRIGGER_SIZE);                // 0...1

        // in total 12 bit immediate here!

        uint32_t val = (
                (register_index << (2 * DMA_PARAMETER_INDEX_SIZE + DMA_TRIGGER_SIZE)) |
                (src1_target_index << (DMA_PARAMETER_INDEX_SIZE + DMA_TRIGGER_SIZE)) |
                (src2_target_index << DMA_TRIGGER_SIZE) |
                trigger
        );

        // asm dont take 12-bit unsigned, but signed values... fake as signed

        if ((val & 0x800) != 0)
            return int32_t(val | 0xfffff800);
        return int32_t(val);
    }

    /**
     *  Use DMA CMD Registerfile to generate a command for the DMA.
     *  Execution can be started by the triger flag.
     *  Command is modified with 2 values from the EIS-V Registerfile with this instruction
     * @tparam register_index   Target Register of DMA Cmd Registerfile
     * @tparam src1_target_index    Target Parameter Index in DMA Cmd to be written by a (inside DMA CMD Registerfile)
     * @tparam src2_target_index    Target Parameter Index in DMA Cmd to be written by b (inside DMA CMD Registerfile)
     * @tparam trigger  Flag if DMA CMD should be emitted by this command
     * @param a value to be written in DMA CMD
     * @param b value to be written in DMA CMD
     */
    template<uint32_t register_index, uint32_t src1_target_index, uint32_t src2_target_index, uint8_t trigger>
    inline void c_dma_lw(const uint32_t a, const uint32_t b) {

        static_assert((register_index & N_ZEROS(DMA_REGISTER_INDEX_SIZE)) == 0);
        static_assert((src1_target_index & N_ZEROS(DMA_PARAMETER_INDEX_SIZE)) == 0);
        static_assert((src2_target_index & N_ZEROS(DMA_PARAMETER_INDEX_SIZE)) == 0);
        static_assert((trigger & N_ZEROS(DMA_TRIGGER_SIZE)) == 0);

        asm ("vpro.dma.lw %0, %1, %2\n\t"
                : /* No outputs. */
                : "r" (a), "r" (b), "i" (dma_lw_imm(register_index, src1_target_index, src2_target_index, trigger)));
    }

//    /**
//     * constexpr to generate single immediate (20 bit)
//     * @param register_index
//     * @param index_mask
//     * @param trigger
//     * @param is_increment
//     * @param inc_imm
//     * @return
//     */
//    constexpr int32_t dma_li_imm(uint32_t register_index, uint32_t index_mask, uint8_t trigger,
//                                  uint8_t is_increment = 0, uint8_t inc_imm = 0) {
//        static_assert((DMA_REGISTER_INDEX_SIZE + DMA_PARAMETERS + DMA_TRIGGER_SIZE + 1 + 3) <= 20);
//
//        register_index &= N_ONES(DMA_REGISTER_INDEX_SIZE);     // 0...7 = 3 bit
//        index_mask &= N_ONES(DMA_PARAMETERS);                  // 9 bit
//        trigger &= N_ONES(DMA_TRIGGER_SIZE);                   // 0...1 = 1 bit
//
//        // in total 20 bit immediate here!
//        uint32_t val = (
//                ((inc_imm & 0b111111) << (1 + DMA_REGISTER_INDEX_SIZE + DMA_PARAMETERS + DMA_TRIGGER_SIZE)) |
//                ((is_increment & 0b1) << (DMA_REGISTER_INDEX_SIZE + DMA_PARAMETERS + DMA_TRIGGER_SIZE)) |
//                (register_index << (DMA_PARAMETERS + DMA_TRIGGER_SIZE)) |
//                (index_mask << DMA_TRIGGER_SIZE) |
//                trigger
//        );
//
//        // asm take 20-bit signed...
//        if ((val & 0x80000) != 0)
//            val = val | 0xffff8000;
//
//        // jal offset needs to be aligned to 16-bit (this immediate is interpreted as jal immediate)
//        val = val << 1;
//
//        return int32_t(val);
//    }
//
//    /**
//     *  Use DMA CMD Registerfile to generate a command for the DMA.
//     *  Execution can be started by the triger flag.
//     *  Command is modified with 1 values from the EIS-V Registerfile with this instruction
//     * @tparam register_index   Target Register of DMA Cmd Registerfile
//     * @tparam index_mask Target Parameter Index Mask in DMA Cmd to be written by a (inside DMA CMD Registerfile)
//     * @tparam trigger Flag if DMA CMD should be emitted by this command
//     * @tparam is_increment If set, the value will be added to the specific selected parameters of the DMA
//     * @tparam increment_use_imm_instead_rf If set, the value will not be used from Registerfile (variable) but as immediate with 6-bit
//     * @param value value to be written in DMA CMD
//     */
//    template<uint32_t register_index, uint32_t index_mask, uint8_t trigger, uint8_t is_increment = false,
//            uint8_t increment_use_imm_instead_rf = false>
//    inline void c_dma_li(const uint32_t value) {
//        static_assert((register_index & N_ZEROS(DMA_REGISTER_INDEX_SIZE)) == 0);
//        static_assert((index_mask & N_ZEROS(DMA_PARAMETERS)) == 0); // bits 0...8 are ok
//        static_assert((trigger & N_ZEROS(DMA_TRIGGER_SIZE)) == 0);
//        assert((value & N_ZEROS(11)) == 0);
//
//        uint8_t inc_imm;
//        uint8_t inc_imm_r;
//        if (is_increment && increment_use_imm_instead_rf) {
//            inc_imm = (value & 0x3e0) >> 5; // bits 11,10,9,8,7,6
//            inc_imm_r = value & 0x1f; // 5 bits
//            switch (inc_imm_r) {
//                case 0:
//                    asm ("vpro.dma.li x0, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 1:
//                    asm ("vpro.dma.li x1, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 2:
//                    asm ("vpro.dma.li x2, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 3:
//                    asm ("vpro.dma.li x3, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 4:
//                    asm ("vpro.dma.li x4, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 5:
//                    asm ("vpro.dma.li x5, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 6:
//                    asm ("vpro.dma.li x6, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 7:
//                    asm ("vpro.dma.li x7, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 8:
//                    asm ("vpro.dma.li x8, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 9:
//                    asm ("vpro.dma.li x9, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 10:
//                    asm ("vpro.dma.li x10, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 11:
//                    asm ("vpro.dma.li x11, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 12:
//                    asm ("vpro.dma.li x12, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 13:
//                    asm ("vpro.dma.li x13, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 14:
//                    asm ("vpro.dma.li x14, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 15:
//                    asm ("vpro.dma.li x15, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 16:
//                    asm ("vpro.dma.li x16, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 17:
//                    asm ("vpro.dma.li x17, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 18:
//                    asm ("vpro.dma.li x18, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 19:
//                    asm ("vpro.dma.li x19, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 20:
//                    asm ("vpro.dma.li x20, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 21:
//                    asm ("vpro.dma.li x21, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 22:
//                    asm ("vpro.dma.li x22, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 23:
//                    asm ("vpro.dma.li x23, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 24:
//                    asm ("vpro.dma.li x24, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 25:
//                    asm ("vpro.dma.li x25, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 26:
//                    asm ("vpro.dma.li x26, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 27:
//                    asm ("vpro.dma.li x27, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 28:
//                    asm ("vpro.dma.li x28, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 29:
//                    asm ("vpro.dma.li x29, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                case 30:
//                    asm ("vpro.dma.li x30, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                    break;
//                case 31:
//                    asm ("vpro.dma.li x31, %0\n\t"
//                            : /* No outputs. */
//                            : "i" (dma_li_imm(register_index, index_mask, trigger, is_increment, inc_imm)));
//                    break;
//                default:
////                    printf("[Error][DMA] Issue with imm in c_dma_li OOR!\n");
//                    break;
//            }
//        } else {
//            // 20bit immm, 5bit reg
//            asm ("vpro.dma.li %0, %1\n\t"
//                    : /* No outputs. */
//                    : "r" (value), "i" (dma_li_imm(register_index, index_mask, trigger)));
//        }
//    }

    /**
     * Trigger the current DMA Cmd from DMA Registerfile
     * Dont modify the Command
     */
    template<uint32_t register_index>
    inline void c_dma_trigger() {
        asm ("vpro.dma.lw x0, x0, %0\n\t"
                : /* No outputs. */
                : "i" (dma_lw_imm(register_index, 15, 15, 1)));
    }

//    /**
//     * Helper to call c_dma_li with incrementing the selected parameters with variable integer
//     * @tparam register_index   Target Register of DMA Cmd Registerfile
//     * @tparam index_mask Target Parameter Index Mask in DMA Cmd to be written by a (inside DMA CMD Registerfile)
//     * @tparam trigger Flag if DMA CMD should be emitted by this command
//     * @param value incremented amount
//     */
//    template<uint32_t register_index, uint32_t index_mask, uint8_t trigger>
//    inline void c_dma_add(const uint32_t value) {
//        c_dma_li<register_index, index_mask, trigger, true, false>(value);
//    }
//
//    /**
//     * Helper to call c_dma_li with incrementing the selected parameters with an Immediate value
//     * @tparam register_index   Target Register of DMA Cmd Registerfile
//     * @tparam index_mask Target Parameter Index Mask in DMA Cmd to be written by a (inside DMA CMD Registerfile)
//     * @tparam trigger Flag if DMA CMD should be emitted by this command
//     * @param value incremented amount. Needs to be an immediate value!
//     */
//    template<uint32_t register_index, uint32_t index_mask, uint8_t trigger>
//    inline void c_dma_addi(const uint32_t value) {
//        c_dma_li<register_index, index_mask, trigger, true, true>(value);
//    }
}

// TODO: add DMA Registerfile

//#endif // ndef simulation
#endif // dma_asm_h
