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
// Created by gesper on 01.06.23.
//

#include "DMABlockExtractor.h"
#include "DMALooper.h"
#include "NonBlockingMainMemory.h"
#include "simulator/ISS.h"

void DMABlockExtractor::newAddrTrigger(uint32_t addr, uint32_t offset_shift) {
    if (offset_shift == 0) {
        busy = true;
        addr_ff += addr;
    } else {
        addr_ff = uint64_t(addr) << offset_shift;
    }
}

void DMABlockExtractor::tick() {
    if (busy) {
#ifndef ISS_STANDALONE
        printf_error("DMA BLock Extractor should only be used by ISS Standalone!\n");
#endif

        if (!core->getDmaLooper()->isBusy()) {
            if ((addr_ff >> 32) == 0) {
                uint8_t* mem = ((NonBlockingMainMemory*)(core->bus))->getMemory();
                core->gen_direct_dma_instruction(&mem[addr_ff]);
            } else {
                core->gen_direct_dma_instruction((uint8_t*)addr_ff);
            }

            size_ff--;
            addr_ff += 32;
            if (size_ff <= 0) busy = false;
        }
    }
}
