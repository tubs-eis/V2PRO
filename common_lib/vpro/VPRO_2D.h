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
//
// Created by gesper on 28.03.22.
//

#ifndef COMMON_VPRO_2D_H
#define COMMON_VPRO_2D_H

#include "../vpro/vpro_defs.h"
#include "../vpro/vpro_cmd_defs.h"
#include "VPRO.h"

#ifndef SIMULATION
#include "../vpro/vpro_aux.h"
#else
// requires gcc-flag: -I TOOLS/VPRO/ISS/isa_intrinsic_lib/
//             cmake: target_include_directories(target PUBLIC ...)

#include <iss_aux.h>
#include <simulator/helper/debugHelper.h> // printf_...
#include <core_wrapper.h> // __vpro()
#endif

namespace VPRO {
    /**
     * 2-Dimensional Functions of the Vector-Processor Array
     */
    namespace DIM2 {
        /**
         * Load-Store Lane related Commands
         */
        namespace LOADSTORE {
            /**
             * load unsigned 16-bit data word to chain
             * @param offset LM offset to address (whole LM address can be used)
             * @param src_offset LM offset (10-bit: 0-1024)
             * @param src_alpha vector alpha
             * @param src_beta vector beta
             * @param x_end vector x end
             * @param y_end vector y end
             * @param flag_update
             */
            inline static void __attribute__((always_inline))
            load(uint32_t offset, uint32_t src_offset, uint32_t src_alpha, uint32_t src_beta, uint32_t x_end,
                 uint32_t y_end, bool flag_update = false) {
                // always is-chain
                // always lane ls
                // always func_loads:  0b00  0b0010
                // dst always: 0 0 0

                // src2_imm -> 13-bit
                // src1_off -> 10-bit
                // src1_a   -> 6-bit
                // src1_b   -> 6-bit
                // x/y      -> ea 6-bit
                // flag_update -> 1-bit
                // => 48-bit

                // src1_g   -> 6-bit
                // z        -> 10-bit
                // => 64-bit

                __vpro(LS, NONBLOCKING, IS_CHAIN, FUNC_LOAD, flag_update ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       DST_ADDR(0, 0, 0),
                       SRC1_ADDR(src_offset, src_alpha, src_beta),
                       SRC2_IMM_2D(offset),
                       x_end, y_end);
            }; // load

            /**
             * load signed 16-bit data word to chain
             * @param offset
             * @param src_offset
             * @param src_alpha
             * @param src_beta
             * @param x_end vector x end
             * @param y_end vector y end
             * @param flag_update
             */
            inline static void __attribute__((always_inline))
            loads(uint32_t offset, uint32_t src_offset, uint32_t src_alpha, uint32_t src_beta, uint32_t x_end,
                  uint32_t y_end, bool flag_update = false) {
                // always is-chain
                // always lane ls
                // always func_loads:  0b00  0b0010
                // dst always: 0 0 0

                // src2_imm -> 13-bit
                // src1_off -> 10-bit
                // src1_a   -> 6-bit
                // src1_b   -> 6-bit
                // x/y      -> ea 6-bit
                // flag_update -> 1-bit
                // => 48-bit

                // src1_g   -> 6-bit
                // z        -> 10-bit
                // => 64-bit

                __vpro(LS, NONBLOCKING, IS_CHAIN, FUNC_LOADS, flag_update ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       DST_ADDR(0, 0, 0),
                       SRC1_ADDR(src_offset, src_alpha, src_beta),
                       SRC2_IMM_2D(offset),
                       x_end, y_end);
            }; // loads

            /**
             * load unsigned 8-bit data word to chain
             * @param offset
             * @param src_offset
             * @param src_alpha
             * @param src_beta
             * @param x_end vector x end
             * @param y_end vector y end
             * @param flag_update
             */
            inline static void __attribute__((always_inline))
            loadb(uint32_t offset, uint32_t src_offset, uint32_t src_alpha, uint32_t src_beta, uint32_t x_end,
                  uint32_t y_end,
                  bool flag_update = false) {

                // always is-chain
                // always lane ls
                // always func_loads:  0b00  0b0010
                // dst always: 0 0 0

                // src2_imm -> 13-bit
                // src1_off -> 10-bit
                // src1_a   -> 6-bit
                // src1_b   -> 6-bit
                // x/y      -> ea 6-bit
                // flag_update -> 1-bit
                // => 48-bit

                // src1_g   -> 6-bit
                // z        -> 10-bit
                // => 64-bit

                __vpro(LS, NONBLOCKING, IS_CHAIN, FUNC_LOADB, flag_update ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       DST_ADDR(0, 0, 0),
                       SRC1_ADDR(src_offset, src_alpha, src_beta),
                       SRC2_IMM_2D(offset),
                       x_end, y_end);
            } // loadb

            /**
             * load signed 8-bit data word to chain
             * @param offset
             * @param src_offset
             * @param src_alpha
             * @param src_beta
             * @param x_end vector x end
             * @param y_end vector y end
             * @param flag_update
             */
            inline static void __attribute__((always_inline))
            loadbs(uint32_t offset, uint32_t src_offset, uint32_t src_alpha, uint32_t src_beta, uint32_t x_end,
                   uint32_t y_end,
                   bool flag_update = false) {

                // always is-chain
                // always lane ls
                // always func_loads:  0b00  0b0010
                // dst always: 0 0 0

                // src2_imm -> 13-bit
                // src1_off -> 10-bit
                // src1_a   -> 6-bit
                // src1_b   -> 6-bit
                // x/y      -> ea 6-bit
                // flag_update -> 1-bit
                // => 48-bit

                // src1_g   -> 6-bit
                // z        -> 10-bit
                // => 64-bit

                __vpro(LS, NONBLOCKING, IS_CHAIN, FUNC_LOADBS, flag_update ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       DST_ADDR(0, 0, 0),
                       SRC1_ADDR(src_offset, src_alpha, src_beta),
                       SRC2_IMM_2D(offset),
                       x_end, y_end);
            } // loadbs

            /**
             * load with source addressing mode changed: [NOT HARDWARE COMPATIBLE]
             * dst.imm == 0 => lm_addr = src2.imm + src1.offset + src1.alpha * x + src1.beta * y;
             * dst.imm == 1 => lm_addr = src2.imm + src1.offset + src1.alpha * x - src1.beta * y;
             * dst.imm == 2 => lm_addr = src2.imm + src1.offset - src1.alpha * x + src1.beta * y;
             * dst.imm == 3 => lm_addr = src2.imm + src1.offset - src1.alpha * x - src1.beta * y;
             * @param offset
             * @param src_offset
             * @param src_alpha
             * @param src_beta
             * @param x_end vector x end
             * @param y_end vector y end
             * @param flag_update
             */
            inline static void __attribute__((always_inline))
            load_reverse(uint32_t offset, uint32_t src_offset, int32_t src_alpha, int32_t src_beta, uint32_t x_end,
                         uint32_t y_end, bool flag_update = false) {

                // always is-chain
                // always lane ls
                // always func_loads:  0b00  0b0010
                // dst always: 0 0 0

                // src2_imm -> 13-bit
                // src1_off -> 10-bit
                // src1_a   -> 6-bit
                // src1_b   -> 6-bit
                // x/y      -> ea 6-bit
                // flag_update -> 1-bit
                // => 48-bit

                // src1_g   -> 6-bit
                // z        -> 10-bit
                // => 64-bit

                //            if(pipe.cmd->dst.imm == 0b00) pipe.lm_addr = pipe.cmd->src2.imm + pipe.cmd->src1.offset + pipe.cmd->src1.alpha * pipe.cmd->x + pipe.cmd->src1.beta * pipe.cmd->y;
                //            if(pipe.cmd->dst.imm == 0b01) pipe.lm_addr = pipe.cmd->src2.imm + pipe.cmd->src1.offset + pipe.cmd->src1.alpha * pipe.cmd->x - pipe.cmd->src1.beta * pipe.cmd->y;
                //            if(pipe.cmd->dst.imm == 0b10) pipe.lm_addr = pipe.cmd->src2.imm + pipe.cmd->src1.offset - pipe.cmd->src1.alpha * pipe.cmd->x + pipe.cmd->src1.beta * pipe.cmd->y;
                //            if(pipe.cmd->dst.imm == 0b11) pipe.lm_addr = pipe.cmd->src2.imm + pipe.cmd->src1.offset - pipe.cmd->src1.alpha * pipe.cmd->x - pipe.cmd->src1.beta * pipe.cmd->y;
                // TODO: this should be offset not immediate!

#ifdef SIMULATION
                if (!has_printed_load_reverse_warning) {
                    // one time print
                    printf_warning("[LS] Load Reverse command not yet implemented in Hardware!");
                    has_printed_load_reverse_warning = true;
                }
#endif

                uint32_t dst_imm = 0b00;
                if (src_alpha < 0) {
                    dst_imm |= 0b10;
                }
                if (src_beta < 0) {
                    dst_imm |= 0b01;
                }

                __vpro(LS, NONBLOCKING, IS_CHAIN, FUNC_LOAD_REVERSE, flag_update ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       DST_ADDR(dst_imm, 0, 0),
                       SRC1_ADDR(src_offset, src_alpha, src_beta),
                       SRC2_IMM_2D(offset),
                       x_end, y_end);
            } // load_reverse

            /**
             * load and shift value left [requires Hardware support - optional]
             * @param offset
             * @param src_offset
             * @param src_alpha
             * @param src_beta
             * @param x_end vector x end
             * @param y_end vector y end
             * @param shift_amount
             * @param flag_update
             */
            inline static void __attribute__((always_inline))
            load_shift_left(uint32_t offset, uint32_t src_offset, int32_t src_alpha, int32_t src_beta, uint32_t x_end,
                            uint32_t y_end, uint32_t shift_amount, bool flag_update = false) {
#ifdef SIMULATION
                printf_error("[LS] TODO: load_shift_left!\n");
#endif

                __vpro(LS, NONBLOCKING, IS_CHAIN, FUNC_LOAD_SHIFT_LEFT, FLAG_UPDATE,
                       DST_ADDR(shift_amount, 0, 0),
                       SRC1_ADDR(src_offset, src_alpha, src_beta),
                       SRC2_IMM_2D(offset),
                       x_end, y_end);
            }

            /**
             * load and shift value right [requires Hardware support - optional]
             * @param offset
             * @param src_offset
             * @param src_alpha
             * @param src_beta
             * @param x_end vector x end
             * @param y_end vector y end
             * @param shift_amount
             * @param flag_update
             */
            inline static void __attribute__((always_inline))
            load_shift_right(uint32_t offset, uint32_t src_offset, int32_t src_alpha, int32_t src_beta, uint32_t x_end,
                             uint32_t y_end, uint32_t shift_amount, bool flag_update = false) {
#ifdef SIMULATION
                printf_error("[LS] TODO: load_shift_right!\n");
#endif
                __vpro(LS, NONBLOCKING, IS_CHAIN, FUNC_LOAD_SHIFT_RIGHT, FLAG_UPDATE,
                       DST_ADDR(shift_amount, 0, 0),
                       SRC1_ADDR(src_offset, src_alpha, src_beta),
                       SRC2_IMM_2D(offset),
                       x_end, y_end);
            }

            /**
             * load unsigned 16-bit data word to chain with chained offset
             * @param src_lane lane to chain offset from
             * @param src_offset LM offset (10-bit: 0-1024)
             * @param src_alpha vector alpha
             * @param src_beta vector beta
             * @param x_end vector x end
             * @param y_end vector y end
             * @param flag_update
             */
            inline static void __attribute__((always_inline))
            dynamic_load(uint32_t src_lane, uint32_t src_offset, uint32_t src_alpha, uint32_t src_beta, uint32_t x_end,
                 uint32_t y_end, bool flag_update = false) {
                printf_error("[LS] Dynamic Load: use 3D!... not impl in 2D!\n");
//                if (src_lane == L0) {
//                    __vpro(LS, NONBLOCKING, IS_CHAIN, FUNC_LOAD, flag_update ? FLAG_UPDATE : NO_FLAG_UPDATE,
//                        DST_ADDR(0, 0, 0),
//                        SRC1_ADDR(src_offset, src_alpha, src_beta),
//                        SRC2_CHAINING_2D(0),
//                        x_end, y_end);
//                } else if (src_lane == L1) {
//                    __vpro(LS, NONBLOCKING, IS_CHAIN, FUNC_LOAD, flag_update ? FLAG_UPDATE : NO_FLAG_UPDATE,
//                        DST_ADDR(0, 0, 0),
//                        SRC2_ADDR(src_offset, src_alpha, src_beta),
//                        SRC2_CHAINING_2D(1),
//                        x_end, y_end);
//                }
            }; // dynamic_load

            /**
             * load signed 16-bit data word to chain with chained offset
             * @param src_lane lane to chain offset from
             * @param src_offset LM offset (10-bit: 0-1024)
             * @param src_alpha vector alpha
             * @param src_beta vector beta
             * @param x_end vector x end
             * @param y_end vector y end
             * @param flag_update
             */
            inline static void __attribute__((always_inline))
            dynamic_loads(uint32_t src_lane, uint32_t src_offset, uint32_t src_alpha, uint32_t src_beta, uint32_t x_end,
                 uint32_t y_end, bool flag_update = false) {
                printf_error("[LS] Dynamic Load: use 3D!... not impl in 2D!\n");
//                if (src_lane == L0) {
//                    __vpro(LS, NONBLOCKING, IS_CHAIN, FUNC_LOADS, flag_update ? FLAG_UPDATE : NO_FLAG_UPDATE,
//                        DST_ADDR(0, 0, 0),
//                        SRC1_ADDR(src_offset, src_alpha, src_beta),
//                        SRC2_CHAINING_2D(0),
//                        x_end, y_end);
//                } else if (src_lane == L1) {
//                    __vpro(LS, NONBLOCKING, IS_CHAIN, FUNC_LOADS, flag_update ? FLAG_UPDATE : NO_FLAG_UPDATE,
//                        DST_ADDR(0, 0, 0),
//                        SRC2_ADDR(src_offset, src_alpha, src_beta),
//                        SRC2_CHAINING_2D(1),
//                        x_end, y_end);
//                }
            }; // dynamic_loads

            /**
             * load unsigned 8-bit data word to chain with chained offset
             * @param src_lane lane to chain offset from
             * @param src_offset LM offset (10-bit: 0-1024)
             * @param src_alpha vector alpha
             * @param src_beta vector beta
             * @param x_end vector x end
             * @param y_end vector y end
             * @param flag_update
             */
            inline static void __attribute__((always_inline))
            dynamic_loadb(uint32_t src_lane, uint32_t src_offset, uint32_t src_alpha, uint32_t src_beta, uint32_t x_end,
                 uint32_t y_end, bool flag_update = false) {
                printf_error("[LS] Dynamic Load: use 3D!... not impl in 2D!\n");
//                if (src_lane == L0) {
//                    __vpro(LS, NONBLOCKING, IS_CHAIN, FUNC_LOADB, flag_update ? FLAG_UPDATE : NO_FLAG_UPDATE,
//                        DST_ADDR(0, 0, 0),
//                        SRC1_ADDR(src_offset, src_alpha, src_beta),
//                        SRC2_CHAINING_2D(0),
//                        x_end, y_end);
//                } else if (src_lane == L1) {
//                    __vpro(LS, NONBLOCKING, IS_CHAIN, FUNC_LOADB, flag_update ? FLAG_UPDATE : NO_FLAG_UPDATE,
//                        DST_ADDR(0, 0, 0),
//                        SRC2_ADDR(src_offset, src_alpha, src_beta),
//                        SRC2_CHAINING_2D(1),
//                        x_end, y_end);
//                }
            }; // dynamic_loadb

            /**
             * load signed 8-bit data word to chain with chained offset
             * @param src_lane lane to chain offset from
             * @param src_offset LM offset (10-bit: 0-1024)
             * @param src_alpha vector alpha
             * @param src_beta vector beta
             * @param x_end vector x end
             * @param y_end vector y end
             * @param flag_update
             */
            inline static void __attribute__((always_inline))
            dynamic_loadbs(uint32_t src_lane, uint32_t src_offset, uint32_t src_alpha, uint32_t src_beta, uint32_t x_end,
                 uint32_t y_end, bool flag_update = false) {
                printf_error("[LS] Dynamic Load: use 3D!... not impl in 2D!\n");
//                if (src_lane == L0) {
//                    __vpro(LS, NONBLOCKING, IS_CHAIN, FUNC_LOADBS, flag_update ? FLAG_UPDATE : NO_FLAG_UPDATE,
//                        DST_ADDR(0, 0, 0),
//                        SRC1_ADDR(src_offset, src_alpha, src_beta),
//                        SRC2_CHAINING_2D(0),
//                        x_end, y_end);
//                } else if (src_lane == L1) {
//                    __vpro(LS, NONBLOCKING, IS_CHAIN, FUNC_LOADBS, flag_update ? FLAG_UPDATE : NO_FLAG_UPDATE,
//                        DST_ADDR(0, 0, 0),
//                        SRC2_ADDR(src_offset, src_alpha, src_beta),
//                        SRC2_CHAINING_2D(1),
//                        x_end, y_end);
//                }
            }; // dynamic_loadbs

            /**
             * Store 16-bit data (cut) from chain input (another processing lane)
             * @param offset
             * @param dst_offset
             * @param dst_alpha
             * @param dst_beta
             * @param x_end vector x end
             * @param y_end vector y end
             * @param src_lane
             */
            inline static void __attribute__((always_inline))
            store(uint32_t offset, uint32_t dst_offset, uint32_t dst_alpha, uint32_t dst_beta, uint32_t x_end,
                  uint32_t y_end,
                  uint32_t src_lane) {
                if (src_lane == L0) {
                    __vpro(LS, NONBLOCKING, NO_CHAIN, FUNC_STORE, NO_FLAG_UPDATE,
                           CHAIN_ID_2D(0),
                           SRC1_ADDR(dst_offset, dst_alpha, dst_beta),
                           SRC2_IMM_2D(offset),
                           x_end, y_end);
                } else if (src_lane == L1) {
                    __vpro(LS, NONBLOCKING, NO_CHAIN, FUNC_STORE, NO_FLAG_UPDATE,
                           CHAIN_ID_2D(1),
                           SRC1_ADDR(dst_offset, dst_alpha, dst_beta),
                           SRC2_IMM_2D(offset),
                           x_end, y_end);
                }
#ifdef SIMULATION
                else {
                    printf_error("[LS] Store: received src_lane != L0 | L1");
                }
#endif
            }   //store

            /**
             * Store a left-shifted value (24-bit input from chain shifted left) [TODO]
             * @param offset
             * @param dst_offset
             * @param dst_alpha
             * @param dst_beta
             * @param x_end
             * @param y_end
             * @param src_lane
             * @param shift_amount
             */
            inline static void __attribute__((always_inline))
            store_shift_left(uint32_t offset, uint32_t dst_offset, uint32_t dst_alpha, uint32_t dst_beta,
                             uint32_t x_end,
                             uint32_t y_end, uint32_t src_lane, uint32_t shift_amount) {
#ifdef SIMULATION
                printf_error("[LS] TODO: store_shift_left!\n");
#endif
            }

            /**
             * Store a right-shifted value (24-bit input from chain shifted left) [TODO]
             * @param offset
             * @param dst_offset
             * @param dst_alpha
             * @param dst_beta
             * @param x_end
             * @param y_end
             * @param src_lane
             * @param shift_amount
             */
            inline static void __attribute__((always_inline))
            store_shift_right(uint32_t offset, uint32_t dst_offset, uint32_t dst_alpha, uint32_t dst_beta,
                              uint32_t x_end,
                              uint32_t y_end, uint32_t src_lane, uint32_t shift_amount) {
#ifdef SIMULATION
                printf_error("[LS] TODO: store_shift_right!\n");
#endif
            }
        } // namespace LOADSTORE
        /**
         * regular Processing Lane
         */
        namespace PROCESSING {
            /**
             *
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_ADDR(offset, alpha, beta, gamma)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            abs(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_ABS,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }

            /**
             *
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_ADDR(offset, alpha, beta, gamma)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            min(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MIN,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }

            /**
             *
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_ADDR(offset, alpha, beta, gamma)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            max(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MAX,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }

            /**
             *
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_ADDR(offset, alpha, beta, gamma)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            min_vector_index(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t x_end, uint32_t y_end,
                             bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MIN_VECTOR,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, SRC2_ADDR(1, 0, 0), x_end, y_end);
            }

            /**
             *
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_ADDR(offset, alpha, beta, gamma)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            min_vector_value(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t x_end, uint32_t y_end,
                             bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MIN_VECTOR,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, SRC2_ADDR(0, 0, 0), x_end, y_end);
            }

            /**
             *
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_ADDR(offset, alpha, beta, gamma)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            max_vector_index(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t x_end, uint32_t y_end,
                             bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MAX_VECTOR,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, SRC2_ADDR(1, 0, 0), x_end, y_end);
            }

            /**
             *
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_ADDR(offset, alpha, beta, gamma)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            max_vector_value(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t x_end, uint32_t y_end,
                             bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MAX_VECTOR,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, SRC2_ADDR(0, 0, 0), x_end, y_end);
            }

            /**
             *
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_ADDR(offset, alpha, beta, gamma)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            bit_reversal(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t x_end, uint32_t y_end,
                         uint32_t amount_of_bits = 24,
                         bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_BIT_REVERSAL,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, SRC2_IMM_2D(amount_of_bits), x_end, y_end);
            }


            /**
             * move if zero: if (src1[i] == 0) dst[i] = src2[i]
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_ADDR(offset, alpha, beta, gamma)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            mv_zero(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                    bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MV_ZE,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }


            /**
             * move if not zero: if (src1[i] != 0) dst[i] = src2[i]
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_ADDR(offset, alpha, beta, gamma)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            mv_non_zero(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                        bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MV_NZ,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }


            /**
             * move if negative: if (src1[i] < 0) dst[i] = src2[i]
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_ADDR(offset, alpha, beta, gamma)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            mv_negative(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                        bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MV_MI,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }


            /**
             * move if not negative: if (src1[i] >= 0) dst[i] = src2[i]
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_ADDR(offset, alpha, beta, gamma)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            mv_non_negative(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end,
                            uint32_t y_end,
                            bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MV_PL,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }

            /**
             * Logic
             */

            /**
             * xor vector elements dst[i] = (src1[i] xor src2[i])
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_ADDR(offset, alpha, beta, gamma)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            xor_(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                 bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_XOR,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }

            /**
             * xnor vector elements dst[i] = (src1[i] xor not src2[i])
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_ADDR(offset, alpha, beta, gamma)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            xnor(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                 bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_XNOR,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }

            /**
             * and vector elements dst[i] = (src1[i] and src2[i])
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_ADDR(offset, alpha, beta, gamma)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            and_(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                 bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_AND,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }

            /**
             * andn vector elements dst[i] = (src1[i] and not src2[i])
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_ADDR(offset, alpha, beta, gamma)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            andn(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                 bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                printf_error("[ANDN] not supported any longer!\n");
//                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_ANDN,
//                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
//                       dst, src1, src2, x_end, y_end);
            }

            /**
             * nand vector elements dst[i] = not (src1[i] and src2[i])
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_ADDR(offset, alpha, beta, gamma)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            nand(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                 bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_NAND,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }

            /**
             * or vector elements dst[i] = (src1[i] or src2[i])
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_ADDR(offset, alpha, beta, gamma)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            or_(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_OR,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }

            /**
             * orn vector elements dst[i] = (src1[i] or not src2[i])
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_ADDR(offset, alpha, beta, gamma)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            orn(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                printf_error("[ORN] not supported any longer!\n");
//                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_ORN,
//                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
//                       dst, src1, src2, x_end, y_end);
            }

            /**
             * nor vector elements dst[i] = not (src1[i] or src2[i])
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_ADDR(offset, alpha, beta, gamma)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            nor(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_NOR,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }

            /**
             * Shift
             */

            /**
             * shift logical right (0 extended) vector elements dst[i] = (src1[i] >> SRC2_IMM )
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_IMM(shift_amount)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            shift_lr(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                     bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_SHIFT_LR,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }

            /**
             * shift arithmetical right ('left extended) vector elements dst[i] = (src1[i] >> SRC2_IMM )
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_IMM(shift_amount)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            shift_ar(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                     bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_SHIFT_AR,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }

            /**
             * shift logical left (0 extended) vector elements dst[i] = (src1[i] << SRC2_IMM )
             * @param lane_mask L0, L1, L0_1 (not LS)
             * @param dst generated by (example) DST_ADDR(offset, alpha, beta, gamma)
             * @param src1 generated by (example) SRC1_ADDR(offset, alpha, beta, gamma)
             * @param src2 generated by (example) SRC2_IMM(shift_amount)
             * @param x_end
             * @param y_end
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            shift_ll(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                     bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_SHIFT_LL,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }


            inline static void __attribute__((always_inline))
            shift_ar_neg(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                         bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_SHIFT_AR_NEG,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }


            inline static void __attribute__((always_inline))
            shift_ar_pos(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                         bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_SHIFT_AR_POS,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }

            /**
             * MATH
             */

            /**
             * add vector elements: dst[i] = src1[i] + src2[i]
             * @param lane_mask
             * @param dst
             * @param src1
             * @param src2
             * @param x_end 0 ... MAX_X_END (ISA_X_END_LENGTH bits)
             * @param y_end 0 ... MAX_Y_END (ISA_Y_END_LENGTH bits)
             * @param is_chain
             * @param update_flags
             * @param blocking
             */
            inline static void __attribute__((always_inline))
            add(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_ADD,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }


            inline static void __attribute__((always_inline))
            sub(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_SUB,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }


            inline static void __attribute__((always_inline))
            mull(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                 bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MULL,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }


            inline static void __attribute__((always_inline))
            mulh(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                 bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MULH,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }


            inline static void __attribute__((always_inline))
            macl(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                 bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MACL,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }


            inline static void __attribute__((always_inline))
            mach(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                 bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MACH,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }


            inline static void __attribute__((always_inline))
            macl_pre(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                     bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MACL_PRE,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }


            inline static void __attribute__((always_inline))
            mach_pre(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                     bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MACH_PRE,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }

            inline static void __attribute__((always_inline))
            mach_init_imm(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end,
                          uint32_t y_end,
                          uint32_t imm,
                          bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
#ifdef SIMULATION
                if (((src1 >> ISA_COMPLEX_LENGTH_2D) & 0b11) != SRC_SEL_LS) {
                    printf_error(
                            "[MACH_PRE] SRC1 is not chaining from LS but Accu shall reset to imm (only possibility if chaining from LS)!\n");
                }
                if (vpro_get_mac_init_source() != IMM) {
                    printf_error("[MACH_] need to set init source to IMM before using mac with addr init of accu\n");
                }
#endif
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MACH,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, SRC_IMM_2D(imm, SRC_SEL_LS), src2, x_end, y_end);
            }

            inline static void __attribute__((always_inline))
            mach_init_addr(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end,
                           uint32_t y_end,
                           uint32_t init_offset, uint32_t init_alpha, uint32_t init_beta,
                           bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
#ifdef SIMULATION
                if (((src1 >> ISA_COMPLEX_LENGTH_2D) & 0b11) != SRC_SEL_LS) {
                    printf_error(
                            "[MACH_PRE] SRC1 is not chaining from LS but Accu shall reset to addr (only possibility if chaining from LS)!\n");
                }
                if (vpro_get_mac_init_source() != ADDR) {
                    printf_error("[MACH_] need to set init source to ADDR before using mac with addr init of accu\n");
                }
#endif
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MACH,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, complex_ADDR_2D(SRC_SEL_LS, init_offset, init_alpha, init_beta), src2, x_end, y_end);
            }

            inline static void __attribute__((always_inline))
            mull_neg(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                     bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MULL_NEG,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }


            inline static void __attribute__((always_inline))
            mull_pos(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                     bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MULL_POS,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }


            inline static void __attribute__((always_inline))
            mulh_neg(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                     bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MULH_NEG,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }


            inline static void __attribute__((always_inline))
            mulh_pos(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                     bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_MULH_POS,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }


            inline static void __attribute__((always_inline))
            divl(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                 bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
#ifdef SIMULATION
                if (!has_printed_divl_warning) {
                    // one time print
                    printf_warning("[Processing] DIVL command not yet implemented in Hardware!");
                    has_printed_divl_warning = true;
                }
#endif
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_DIVL,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }

            inline static void __attribute__((always_inline))
            divh(uint32_t lane_mask, uint32_t dst, uint32_t src1, uint32_t src2, uint32_t x_end, uint32_t y_end,
                 bool is_chain = false, bool update_flags = false, bool blocking = false
            ) {
#ifdef SIMULATION
                if (!has_printed_divh_warning) {
                    // one time print
                    printf_warning("[Processing] DIVH command not yet implemented in Hardware!");
                    has_printed_divh_warning = true;
                }
#endif
                __vpro(lane_mask, blocking ? BLOCKING : NONBLOCKING, is_chain ? IS_CHAIN : NO_CHAIN, FUNC_DIVH,
                       update_flags ? FLAG_UPDATE : NO_FLAG_UPDATE,
                       dst, src1, src2, x_end, y_end);
            }

        } // namespace PROCESSING
    } // namespace DIM2
} // namespace VPRO
#endif //COMMON_VPRO_2D_H
