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
#ifndef MEMUTILS_H
#define MEMUTILS_H

#include "riscv/eisV_hardware_info.hpp"
#include <cstring>

#ifdef SIMULATION
#include "iss_aux.h"
#endif

inline void _memcopy(void *dest, void *src, size_t n) {
    // memcopy via riscv, which does not allow for burst transfers (~30 cycles per element)
#ifdef SIMULATION
    intptr_t source = intptr_t(src);
    uint8_t * destination = (uint8_t *) dest;
    for (uint i=0; i<n; i++) {
        core_->dbgMemRead(source + i, destination + i);
    }
#else
    memcpy(dest, src, n);
#endif
}

inline void _memcopy_vpro(uint32_t cluster_mask, intptr_t ext_dst, intptr_t ext_src, uint32_t word_count) {
    // memcopy via vpro dma
    // dma_e2l_2d(cluster_mask, 0x1, ext_src, 0x0, word_count, 1, 0);
    // dma_l2e_2d(cluster_mask, 0x1, ext_dst, 0x0, word_count, 1, 0);

    dma_e2l_1d(cluster_mask, 0x1, ext_src, 0x0, word_count);
    dma_l2e_1d(cluster_mask, 0x1, ext_dst, 0x0, word_count);
}

#endif // MEMUTILS_H