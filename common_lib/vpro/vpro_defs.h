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
// # vpro_defs.h - System IO map - visible from RISC processors  #
// # ----------------------------------------------------------- #
// # Created:     Stephan Nolting, 03.03.2017                    #
// # Last update: Stephan Nolting, 14.02.2018                    #
// ###############################################################

#include <inttypes.h>

#ifndef vpro_defs_h
#define vpro_defs_h

// Base address of vector processor system: 0xFFFE0000
// Base address of IO utility devices:      0xFFFF0000

// Global VCP Command Mask Registers
// -----------------------------------------------------------------------------
#define VPRO_CLUSTER_MASK_ADDR 0xFFFE0000
#define VPRO_CLUSTER_MASK (*((volatile uint32_t*) (VPRO_CLUSTER_MASK_ADDR))) // -/w: AND mask for broadcasting VCMDs
#define VPRO_UNIT_MASK_ADDR 0xFFFE0004
#define VPRO_UNIT_MASK    (*((volatile uint32_t*) (VPRO_UNIT_MASK_ADDR))) // -/w: AND mask for broadcasting VCMDs

// BUSY register base address 0xFFFEyy00, yy: 8-bit specifies cluster
#define VPRO_BUSY_BASE_ADDR 0xFFFE0008
#define VPRO_BUSY_BASE (*((volatile uint32_t*) (VPRO_BUSY_BASE_ADDR))) // r/-: bits in reg correspond to VU in cluster

#define VPRO_BUSY_MASK_CL_ADDR 0xFFFE0010
#define VPRO_BUSY_MASK_CL       (*((volatile uint32_t*) (VPRO_BUSY_MASK_CL_ADDR)))   // r/w
#define VPRO_BUSY_MASKED_DMA_ADDR 0xFFFE0018
#define VPRO_BUSY_MASKED_DMA    (*((volatile uint32_t*) (VPRO_BUSY_MASKED_DMA_ADDR)))   // r
#define VPRO_BUSY_MASKED_VPRO_ADDR 0xFFFE001C
#define VPRO_BUSY_MASKED_VPRO   (*((volatile uint32_t*) (VPRO_BUSY_MASKED_VPRO_ADDR)))   // r

// DCMA
#define DCMA_FLUSH_ADDR       0xFFFE0020 // -/w
#define DCMA_RESET_ADDR       0xFFFE0024 // -/w
#define DCMA_WAIT_BUSY_ADDR   0xFFFE0020 // r/-

// Sync Registers
#define VPRO_LANE_SYNC_ADDR 0xFFFE0028
#define VPRO_LANE_SYNC   (*((volatile uint32_t*) (VPRO_LANE_SYNC_ADDR)))   // r
#define VPRO_DMA_SYNC_ADDR 0xFFFE002C
#define VPRO_DMA_SYNC   (*((volatile uint32_t*) (VPRO_DMA_SYNC_ADDR)))   // r
#define VPRO_SYNC_ADDR 0xFFFE0030
#define VPRO_SYNC   (*((volatile uint32_t*) (VPRO_SYNC_ADDR)))   // r

#define VPRO_MUL_SHIFT_REG_ADDR 0xFFFE0040
#define VPRO_MUL_SHIFT_REG    (*((volatile uint32_t*) (VPRO_MUL_SHIFT_REG_ADDR))) // MUL 5-bit of data are used!
#define VPRO_MAC_SHIFT_REG_ADDR 0xFFFE0044
#define VPRO_MAC_SHIFT_REG    (*((volatile uint32_t*) (VPRO_MAC_SHIFT_REG_ADDR))) // MAC 5-bit!

#define VPRO_MAC_INIT_SOURCE_ADDR 0xFFFE0048
#define VPRO_MAC_INIT_SOURCE    (*((volatile uint32_t*) (VPRO_MAC_INIT_SOURCE_ADDR)))

#define VPRO_MAC_RESET_MODE_ADDR 0xFFFE004C
#define VPRO_MAC_RESET_MODE    (*((volatile uint32_t*) (VPRO_MAC_RESET_MODE_ADDR)))

#define IDMA_PAD_TOP_SIZE     (*((volatile uint32_t*) (0xFFFE00A0))) // -/w: padding for e2l 2d operations
#define IDMA_PAD_BOTTOM_SIZE  (*((volatile uint32_t*) (0xFFFE00A4))) // -/w
#define IDMA_PAD_LEFT_SIZE    (*((volatile uint32_t*) (0xFFFE00A8))) // -/w
#define IDMA_PAD_RIGHT_SIZE   (*((volatile uint32_t*) (0xFFFE00AC))) // -/w
#define IDMA_PAD_VALUE        (*((volatile uint32_t*) (0xFFFE00B0))) // -/w

#define IDMA_STATUS_BUSY_ADDR 0xFFFE00BC
#define IDMA_STATUS_BUSY       (*((volatile uint32_t*) (IDMA_STATUS_BUSY_ADDR)))        // r/-: bit 2: queue full, bit 1: fifo not empty, bit 0: fsm busy

// DMA Command generation in VPRO top
// -----------------------------------------------------------------------------
#define IDMA_UNIT_MASK_ADDR    0xFFFE00C0
#define IDMA_UNIT_MASK         (*((volatile uint32_t*) (IDMA_UNIT_MASK_ADDR)))          // -/w: unit mask
#define IDMA_CLUSTER_MASK_ADDR 0xFFFE00C4
#define IDMA_CLUSTER_MASK      (*((volatile uint32_t*) (IDMA_CLUSTER_MASK_ADDR)))       // -/w: cluster mask
#define IDMA_EXT_BASE_ADDR_E2L_ADDR 0xFFFE00C8
#define IDMA_EXT_BASE_ADDR_E2L (*((volatile uint32_t*) (IDMA_EXT_BASE_ADDR_E2L_ADDR)))  // -/w: external main memory base read address + L<=E trigger
#define IDMA_EXT_BASE_ADDR_L2E_ADDR 0xFFFE00CC
#define IDMA_EXT_BASE_ADDR_L2E (*((volatile uint32_t*) (IDMA_EXT_BASE_ADDR_L2E_ADDR)))  // -/w: external main memory base write address + L=>E trigger
#define IDMA_LOC_ADDR_ADDR     0xFFFE00D0
#define IDMA_LOC_ADDR          (*((volatile uint32_t*) (IDMA_LOC_ADDR_ADDR)))           // -/w: (12 downto 0) => local memory base address
#define IDMA_X_BLOCK_SIZE_ADDR 0xFFFE00D4
#define IDMA_X_BLOCK_SIZE      (*((volatile uint32_t*) (IDMA_X_BLOCK_SIZE_ADDR)))       // -/w: (12 downto 0) => x block size in #elements (16-bit words))
#define IDMA_Y_BLOCK_SIZE_ADDR 0xFFFE00D8
#define IDMA_Y_BLOCK_SIZE      (*((volatile uint32_t*) (IDMA_Y_BLOCK_SIZE_ADDR)))       // -/w: (12 downto 0) => x block size in #elements (16-bit words))
#define IDMA_X_STRIDE_ADDR     0xFFFE00DC
#define IDMA_X_STRIDE          (*((volatile uint32_t*) (IDMA_X_STRIDE_ADDR)))           // -/w: (12 downto 0) => block stide in #elements (16-bit words)) [default: 1]
#define IDMA_PAD_ACTIVATION_ADDR 0xFFFE00E0
#define IDMA_PAD_ACTIVATION    (*((volatile uint32_t*) (IDMA_PAD_ACTIVATION_ADDR)))     // -/w:
// dma stats counter
#define IDMA_READ_HIT_CYCLES_ADDR 0xFFFE00E4
#define IDMA_READ_HIT_CYCLES    (*((volatile uint32_t*) (IDMA_READ_HIT_CYCLES_ADDR)))     // r/w:
#define IDMA_READ_MISS_CYCLES_ADDR 0xFFFE00E8
#define IDMA_READ_MISS_CYCLES    (*((volatile uint32_t*) (IDMA_READ_MISS_CYCLES_ADDR)))     // r/w:
#define IDMA_WRITE_HIT_CYCLES_ADDR 0xFFFE00EC
#define IDMA_WRITE_HIT_CYCLES    (*((volatile uint32_t*) (IDMA_WRITE_HIT_CYCLES_ADDR)))     // r/w:
#define IDMA_WRITE_MISS_CYCLES_ADDR 0xFFFE00F0
#define IDMA_WRITE_MISS_CYCLES    (*((volatile uint32_t*) (IDMA_WRITE_MISS_CYCLES_ADDR)))     // r/w:


// for variable passing to VPRO - IO
// These are addresses for RISC io to replace vpro parameter content
#define VPRO_CMD_REGISTER_3_0_ADDR 0xFFFFFE00
#define VPRO_CMD_REGISTER_3_0 (*((volatile uint32_t*) (VPRO_CMD_REGISTER_3_0_ADDR)))
#define VPRO_CMD_REGISTER_3_1_ADDR 0xFFFFFE04
#define VPRO_CMD_REGISTER_3_1 (*((volatile uint32_t*) (VPRO_CMD_REGISTER_3_1_ADDR)))
#define VPRO_CMD_REGISTER_3_2_ADDR 0xFFFFFE08
#define VPRO_CMD_REGISTER_3_2 (*((volatile uint32_t*) (VPRO_CMD_REGISTER_3_2_ADDR)))

// FREE (Deprecated Loop, customs):
//0xFFFFFE10
//0xFFFFFE14
//0xFFFFFE18
//0xFFFFFE1C
//0xFFFFFE20
//0xFFFFFE24
//0xFFFFFE28
//0xFFFFFE2C

#define VPRO_CMD_REGISTER_4_0_ADDR 0xFFFFFE00   // identical to VPRO_CMD_REGISTER_3_0_ADDR
#define VPRO_CMD_REGISTER_4_0 (*((volatile uint32_t*) (VPRO_CMD_REGISTER_4_0_ADDR)))
#define VPRO_CMD_REGISTER_4_1_ADDR 0xFFFFFE04   // identical to VPRO_CMD_REGISTER_3_1_ADDR
#define VPRO_CMD_REGISTER_4_1 (*((volatile uint32_t*) (VPRO_CMD_REGISTER_4_1_ADDR)))
#define VPRO_CMD_REGISTER_4_2_ADDR 0xFFFFFE30
#define VPRO_CMD_REGISTER_4_2 (*((volatile uint32_t*) (VPRO_CMD_REGISTER_4_2_ADDR)))
#define VPRO_CMD_REGISTER_4_3_ADDR 0xFFFFFE34
#define VPRO_CMD_REGISTER_4_3 (*((volatile uint32_t*) (VPRO_CMD_REGISTER_4_3_ADDR)))



#define IDMA_COMMAND_BLOCK_SIZE_ADDR 0xFFFFFE40
#define IDMA_COMMAND_BLOCK_SIZE (*((volatile uint32_t*) (IDMA_COMMAND_BLOCK_SIZE_ADDR))) // -/w

#define IDMA_COMMAND_BLOCK_ADDR_TRIGGER_ADDR 0xFFFFFE44
#define IDMA_COMMAND_BLOCK_ADDR_TRIGGER (*((volatile uint32_t*) (IDMA_COMMAND_BLOCK_ADDR_TRIGGER_ADDR))) // -/w

#define IDMA_COMMAND_DCACHE_ADDR 0xFFFFFE48
#define IDMA_COMMAND_DCACHE (*((volatile uint32_t*) (IDMA_COMMAND_DCACHE_ADDR))) // -/w

#define IDMA_COMMAND_FSM_ADDR 0xFFFFFE4C
#define IDMA_COMMAND_FSM (*((volatile uint32_t*) (IDMA_COMMAND_FSM_ADDR))) // r/-

// FREE until 0xFFFFFF00 (EIS-V continues!)

// part of EIS-V
#include "../riscv/eisv_defs.h"

#endif // vpro_defs_h
