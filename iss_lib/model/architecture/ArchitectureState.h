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
 * @file ArchitectureState.h
 *
 * Holds the global architectural state including DMA padding and MAC configuration
 */

#ifndef ARCHITECTURE_STATE_H
#define ARCHITECTURE_STATE_H

#include <vpro/vpro_special_register_enums.h>
#include <cstdint>

struct ArchitectureState {
    // global register
    uint32_t cluster_mask_global{0xffffffff};
    uint32_t unit_mask_global{0xffffffff};

    uint32_t sync_cluster_mask_global{0};

    /**
     * The Result after Multiply is inside a Accu Register (48-bit, FPGA).
     * The Result to write into RF is 24-bit.
     * So different regions of accu are taken. (High or Low region)
     * high region is defined here (24+HIGH_BIT_SHIFT downto HIGH_BIT_SHIFT)
     * 18 due to restriction of OPB to 18-bit on DSP slice on FPGA...
     */
    uint32_t ACCU_MAC_HIGH_BIT_SHIFT{0};
    uint32_t ACCU_MUL_HIGH_BIT_SHIFT{0};

    /**
     * The Accumulator Register (MACH, MACL) can be reset to 0 (MAC_PRE) or by use of this register,
     * to any specified value. This is only possible if SRC1 is using LS Chain data source!
     * In this case, the SRC1 offset, alpha, ... can be used as seperate data:
     *   If this register holds ::ZERO, the same function as _PRE is done with MACH/MACL.
     *   IF this register holds ::IMM, the src1 will be used as immediate for resetting the accumulator
     *   IF this register holds ::IMM, the src1 will be used as address in RF for resetting the accumulator
     */
    VPRO::MAC_INIT_SOURCE MAC_ACCU_INIT_SOURCE{VPRO::MAC_INIT_SOURCE::NONE};

    VPRO::MAC_RESET_MODE MAC_ACCU_RESET_MODE{VPRO::MAC_RESET_MODE::Z_INCREMENT};
    /**
     * Registers of the DMA to configure the Padding performed in EXT2D->LOC transfers
     * Dimension in each direction is stored here
     */
    uint32_t dma_pad_top;
    uint32_t dma_pad_right;
    uint32_t dma_pad_bottom;
    uint32_t dma_pad_left;

    /**
     * Registers of the DMA to configure the Padding performed in EXT2D->LOC transfers
     * Value for padded region is stored here
     */
    uint32_t dma_pad_value;

    // TODO(Jasper): This is not architecture state: Ask Sven and remove if possible!
    // to check whether simulation only program error message has already been printed
    bool SIM_ONLY_WARNING_PRINTED_ONCE_XEND_YEND = true;
    bool SIM_ONLY_WARNING_PRINTED_ONCE_SHIFT_LL = true;
};

#endif
