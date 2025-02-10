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
// Created by gesper on 6/11/21.
//

#ifndef DMA_CMD_STRUCT_H
#define DMA_CMD_STRUCT_H

#include <cinttypes>

/**
 * For DMA Commands
 */
namespace COMMAND_DMA {

    /**
     * Each DMA Command may pad 2D data on load (extern -> local) \n
     * The direction is build by a bool[4] with those indices
     */
    enum PAD : uint8_t {
        /**
         * Padding index for TOP
         */
        TOP = 0, //3
        /**
         * Padding index for RIGHT
         */
        RIGHT = 1, //0
        /**
         * Padding index for BOTTOM
         */
        BOTTOM = 2,
        /**
         * Padding index for LEFT
         */
        LEFT = 3 //1
    };

    /**
     * DMA Commands can load data 2D, or 1D \n
     * DMA Commands can load data extern -> local, or local -> extern
     */
    enum DMA_DIRECTION : uint8_t {
        /**
         * extern to local, 1D
         */
        e2l1D = 0,
        /**
         * extern to local, 2D
         */
        e2l2D = 1,
        /**
         * local to extern, 1D
         */
        l2e1D = 2,
        /**
         * local to extern, 2D
         */
        l2e2D = 3,
        /**
         * DMA Command Loop Generation
         */
        loop = 4  // may not be used in COMMAND_DMA directly!
    };

    /**
     * DMA Commands can be created by DCache (Hardware) by loading one of this structs  \n
     * These structs need to match 32-bytes (loaded parallel by dcache) \n
     * These elements need to be placed aligned to memory space \n
     *    in Instanciation: \_\_attribute\_\_ ((aligned (16)))
     */
    struct COMMAND_DMA {
        COMMAND_DMA()= default;
        COMMAND_DMA(DMA_DIRECTION direction, uint32_t cluster, uint32_t unit_mask, uint32_t mm_addr, uint32_t lm_addr, uint16_t x_size):
                direction(direction), padding(0), cluster(cluster), unit_mask(unit_mask), mm_addr(mm_addr), lm_addr(lm_addr), y_leap(1), x_size(x_size), y_size(1)
        {};
        COMMAND_DMA(DMA_DIRECTION direction, uint32_t cluster, uint32_t unit_mask, uint32_t mm_addr, uint32_t lm_addr, uint16_t x_size, uint16_t y_size, uint16_t y_leap):
                direction(direction), padding(0), cluster(cluster), unit_mask(unit_mask), mm_addr(mm_addr), lm_addr(lm_addr), y_leap(y_leap), x_size(x_size), y_size(y_size)
        {};
        COMMAND_DMA(DMA_DIRECTION direction, uint32_t cluster, uint32_t unit_mask, uint32_t mm_addr, uint32_t lm_addr, uint16_t x_size, uint16_t y_size, uint16_t y_leap, uint8_t padding):
            direction(direction), padding(padding), cluster(cluster), unit_mask(unit_mask), mm_addr(mm_addr), lm_addr(lm_addr), y_leap(y_leap), x_size(x_size), y_size(y_size)
        {};
        /**
         * this Commands direction (e2l/l2e) and dimension (1D/2D)
         */
        volatile DMA_DIRECTION direction{0};
        bool isBiasOffset{false}; // no longer used by hardware but still useful for debug
        bool isKernelOffset{false}; // no longer used by hardware but still useful for debug
        // switch padding on/off per transfer; padding length is configured layer-wide via LAYER.pad
        volatile uint8_t padding{0}; // 7 downto 0 := '0 = top, '1 = right, '2 = bottom, '3 = left |  order see CommandDMA::PAD

        volatile uint32_t cluster{0};
        volatile uint32_t unit_mask{0};
        volatile uint32_t mm_addr{0}; // byte address of first non-padding element
        uint32_t mm_addr_64{0};  // used in simulation (ISS) @ 02.06.2023 -> order of elements changed
        volatile uint32_t lm_addr{0}; // word address

        volatile uint16_t y_leap{1}; // distance of last transferred element in row n to first element of row n+1; =1 for gapless
        // misleadingly called "y_leap" in ISS and HW
        volatile uint16_t x_size{0}; // in 16-bit words

        volatile uint16_t y_size{1}; // in 16-bit words
//        COMMAND_SEGMENT_TYPE type{DMA_CMD};
//    uint16_t &word_count = y_leap;
        uint8_t struct_filler[2]{0};    // to match 32-byte [6]

        static const char *dir_to_char(DMA_DIRECTION dir) {
            switch (dir) {
                case e2l1D:
                    return "e2l1D";
                case e2l2D:
                    return "e2l2D";
                case l2e1D:
                    return "l2e1D";
                case l2e2D:
                    return "l2e2D";
                case loop:
                    return "loop";
            }
            return "unkown DMA_DIRECTION";
        }

        static const char *to_bin(size_t const size, void volatile const *const ptr) {
            static char buf[256];
            uint64_t v = *(uint64_t *) ptr;

            char *p = buf;
            for (int i = size - 1; i >= 0; i--) {
                *p++ = '0' + ((v >> i) & 1);
            }
            return buf;
        }

        const char *to_char() const {
            static char buf[1024];
            sprintf(buf, "direction %s, " "isKernelOffset %d, " "isBiasOffset %d, " "cluster %d, "
                         "unit_mask %s, " "mm_addr 0x%08" PRIx32 ", mm_addr_64 0x%08" PRIx32 ", lm_addr 0x%06" PRIx32 ", " "y_leap %d, " "x_size %d, "
                         "y_size %d, " "pad_0 %d, " "pad_1 %d, " "pad_2 %d, " "pad_3 %d",
                    dir_to_char(direction), isKernelOffset, isBiasOffset, cluster, to_bin(32, &unit_mask),
                    mm_addr, mm_addr_64, lm_addr, y_leap, x_size, y_size, (padding & 0b0001), (padding & 0b0010), (padding & 0b0100), (padding & 0b1000));
            return buf;
        }

        const char *unit_mask_to_char() const {
            static char buf[32];
            sprintf(buf, "%d",
                    unit_mask);
            return buf;
        }

        bool equals(const COMMAND_DMA &ref) const {
            bool equal = true;
            equal &= (ref.direction == direction);
            equal &= (ref.isBiasOffset == isBiasOffset);
            equal &= (ref.isKernelOffset == isKernelOffset);
            equal &= (ref.cluster == cluster);
            equal &= (ref.unit_mask == unit_mask);
            //equal &= (ref.mm_addr == mm_addr);//FIXME: uncomment this, once MM addresses are handled correctly
            equal &= (ref.lm_addr == lm_addr);
            equal &= (ref.y_leap == y_leap);
            equal &= (ref.x_size == x_size);
            equal &= (ref.y_size == y_size);
            equal &= (ref.padding == padding);
            return equal;
        }
    };

    static_assert(sizeof(COMMAND_DMA) == 32, "Memory layout of packed struct");


    /**
     * COMMAND_DMA_LOOP usage requires a COMMAND_DMA afterwards (base)
     */
    struct COMMAND_DMA_LOOP {
        volatile DMA_DIRECTION direction{loop};// '0.
        volatile uint8_t cluster_loop_len{};
        volatile int8_t cluster_loop_shift_incr{};
        volatile uint8_t unit_loop_len{};

        volatile int8_t unit_loop_shift_incr{};  // '1.
        volatile uint8_t inter_unit_loop_len{};
        uint8_t struct_padding0[2]{};

        volatile int16_t lm_incr{};  // 13-bit signed! // '2.
        uint8_t struct_padding1[2]{};

        volatile int32_t mm_incr{}; // '3.

        volatile uint16_t dma_cmd_count{};// '4.
        uint8_t struct_padding2[14]{};//pad structure to 32 byte

        const char *to_char() const {
            static char buf[1024];
            sprintf(buf, " DMA LOOP, " "cluster_loop_len %d, " "cluster_loop_shift_incr %d, " "unit_loop_len %d, "
                         "unit_loop_shift_incr %d, " "inter_unit_loop_len %d, " "lm_incr 0x%04" PRIx32 ", " "mm_incr 0x%08" PRIx32 ", "
                         "dma_cmd_count %d",
                    cluster_loop_len, cluster_loop_shift_incr, unit_loop_len, unit_loop_shift_incr,
                    inter_unit_loop_len, lm_incr, mm_incr, dma_cmd_count);
            return buf;
        }

        bool equals(const COMMAND_DMA_LOOP &ref) const {
            bool equal = true;
            equal &= (ref.direction == direction);
            equal &= (ref.cluster_loop_len == cluster_loop_len);
            equal &= (ref.cluster_loop_shift_incr == cluster_loop_shift_incr);
            equal &= (ref.unit_loop_len == unit_loop_len);
            equal &= (ref.unit_loop_shift_incr == unit_loop_shift_incr);
            equal &= (ref.inter_unit_loop_len == inter_unit_loop_len);
            equal &= (ref.lm_incr == lm_incr);
            equal &= (ref.mm_incr == mm_incr);
            equal &= (ref.dma_cmd_count == dma_cmd_count);
            return equal;
        }
    };

    static_assert(sizeof(COMMAND_DMA_LOOP) == 32, "Memory layout of packed struct");

//    in Instanciation:  __attribute__ ((aligned (16)))
//               maybe:  __attribute__ ((section (".vpro")))

} // namespace



#endif // DMA_CMD_STRUCT_H
