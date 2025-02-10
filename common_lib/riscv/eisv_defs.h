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
// Created by gesper on 24.03.22.
//

#ifndef COMMON_EISV_DEFS_H
#define COMMON_EISV_DEFS_H

// Reserved (VPRO): 0xFFFE0000 - 0xFFFF0000

// General Purpose Registers
#define GP_REGISTERS_ADDR 0xFFFFFC00
#define GP_REGISTERS (*((volatile uint32_t*) (GP_REGISTERS_ADDR))) // r/w  // 0xFFFFFC00 - 0xFFFFFCFF (possible addr range -> 256 Byte / 4 = 64 Words of 32-bit)

// UART Module Registers
#define UART_DATA_REG_ADDR 0xFFFFFFB0
#define UART_BAUD_REG_ADDR 0xFFFFFFB4
#define UART_CTRL_REG_ADDR 0xFFFFFFB8
#define UART_STATUS_REG_ADDR 0xFFFFFFBC
#define UART_DATA_REG (*((volatile uint32_t*) (UART_DATA_REG_ADDR))) // r/w // READ: rx_data_reg // WRITE: tx_data_reg
#define UART_BAUD_REG (*((volatile uint32_t*) (UART_BAUD_REG_ADDR))) // r/w // READ + WRITE: bitduration = CLOCK_FREQ/BAUD_RATE
#define UART_CTRL_REG (*((volatile uint32_t*) (UART_CTRL_REG_ADDR))) // r/w // READ + WRITE: ctrl_reg
#define UART_STATUS_REG (*((volatile uint32_t*) (UART_STATUS_REG_ADDR))) // r/w // READ + WRITE: status_reg

// Reserved (VPRO): 0xFFFFFE00 - 0xFFFFFF00

#define RV_DCACHE_FLUSH_ADDR 0xFFFFFF04
#define RV_DCACHE_FLUSH (*((volatile uint32_t*) (RV_DCACHE_FLUSH_ADDR))) // -/w
#define RV_DCACHE_CLEAR_ADDR 0xFFFFFF08
#define RV_DCACHE_CLEAR (*((volatile uint32_t*) (RV_DCACHE_CLEAR_ADDR))) // -/w
//#define RV_ICACHE_FLUSH_ADDR 0xFFFFFF0C
//#define RV_ICACHE_FLUSH (*((volatile uint32_t*) (RV_ICACHE_FLUSH_ADDR))) // -/w
#define RV_ICACHE_CLEAR_ADDR 0xFFFFFF10
#define RV_ICACHE_CLEAR (*((volatile uint32_t*) (RV_ICACHE_CLEAR_ADDR))) // -/w

#define CNT_ICACHE_MISS_ADDR 0xFFFFFF14
#define CNT_ICACHE_MISS (*((volatile uint32_t*) (CNT_ICACHE_MISS_ADDR))) // r/w
#define CNT_ICACHE_HIT_ADDR 0xFFFFFF18
#define CNT_ICACHE_HIT (*((volatile uint32_t*) (CNT_ICACHE_HIT_ADDR))) // r/w
#define CNT_DCACHE_MISS_ADDR 0xFFFFFF1C
#define CNT_DCACHE_MISS (*((volatile uint32_t*) (CNT_DCACHE_MISS_ADDR))) // r/w
#define CNT_DCACHE_HIT_ADDR 0xFFFFFF20
#define CNT_DCACHE_HIT (*((volatile uint32_t*) (CNT_DCACHE_HIT_ADDR))) // r/w

#define DCACHE_PREFETCH_TRIGER_ADDR 0xFFFFFF24
#define DCACHE_PREFETCH_TRIGER (*((volatile uint32_t*) (DCACHE_PREFETCH_TRIGER_ADDR))) // w

#define EXIT_ADDR 0xffffffcc

// all return cycles in eis-v clock domain [TODO]
#define CNT_VPRO_LANE_ACT_ADDR 0xFFFFFFD0
#define CNT_VPRO_LANE_ACT (*((volatile uint32_t*) (CNT_VPRO_LANE_ACT_ADDR))) // any lane is active / vpro somewhere act
#define CNT_VPRO_DMA_ACT_ADDR 0xFFFFFFD4
#define CNT_VPRO_DMA_ACT  (*((volatile uint32_t*) (CNT_VPRO_DMA_ACT_ADDR))) // any dma is active
#define CNT_VPRO_BOTH_ACT_ADDR 0xFFFFFFD8
#define CNT_VPRO_BOTH_ACT (*((volatile uint32_t*) (CNT_VPRO_BOTH_ACT_ADDR))) // any lane AND any dma active
#define CNT_VPRO_TOTAL_ADDR 0xFFFFFFDC
#define CNT_VPRO_TOTAL    (*((volatile uint32_t*) (CNT_VPRO_TOTAL_ADDR))) // cycles in vpro clk
#define CNT_RISC_TOTAL_ADDR    0xFFFFFFE0 // cycles in risc clk
#define CNT_RISC_TOTAL    (*((volatile uint32_t*) (CNT_RISC_TOTAL_ADDR))) // cycles in risc clk
#define CNT_RISC_ENABLED_ADDR 0xFFFFFFE4
#define CNT_RISC_ENABLED  (*((volatile uint32_t*) (CNT_RISC_ENABLED_ADDR))) // eis-v is not synchronizing

// DEBUG FIFO
#define DEBUG_FIFO_ADDR 0xFFFFFFF0
#define DEBUG_FIFO (*((volatile uint32_t*) (DEBUG_FIFO_ADDR))) // -/w

// Sytem time counter
#define SYS_TIME_CNT_LO_ADDR 0xFFFFFFF4
#define SYS_TIME_CNT_LO (*((volatile uint32_t*) (SYS_TIME_CNT_LO_ADDR))) // r/c - read first!
#define SYS_TIME_CNT_HI_ADDR 0xFFFFFFF8
#define SYS_TIME_CNT_HI (*((volatile uint32_t*) (SYS_TIME_CNT_HI_ADDR))) // r/c

// dev/null for dummy read/write transfers
#define DEV_NULL_ADDR 0xFFFFFFFC
#define DEV_NULL (*((volatile uint32_t*) (DEV_NULL_ADDR))) // r/w


#endif //COMMON_EISV_DEFS_H
