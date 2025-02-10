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
// ############################################################
// # VPRO memory map - Seen from the 'host' side              #
// # -> Access from ETH MAC / SIM IO to the OCP bus           #
// #                                                          #
// # Stephan Nolting, Uni Hannover, Feb 2017                  #
// ############################################################

#ifndef vpro_memory_map_h
#define vpro_memory_map_h

// Global parameters
// -----------------------------------------------------------------------------
#define RAM_SIZE      0xF0000000 // size of main memory in bytes // unused... see memory map of mips/vpro in vpro repo - wiki
#define RAM_BASE_ADDR 0x00000000 // base address of RAM (ext DDR)

// Special function registers
// -----------------------------------------------------------------------------
#define DEBUG_FIFO_ADDR 0xE0000000 // read-only (for host!) debug fifo
#define CTRL_REG_ADDR   0xE0800000 // control register (for reset)

#define MIPS_INSTR_ADDR 0xE0400000

#endif // vpro_memory_map_h
