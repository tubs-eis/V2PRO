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
// Created by thieu on 03.05.22.
//

#include "bram.h"
#include <sys/mman.h>
#include "../../simulator/helper/debugHelper.h"

Bram::Bram(uint32_t bram_size_byte, uint32_t bram_word_length) {
    this->bram_word_length = bram_word_length;
    this->bram_size_byte = bram_size_byte;
//    printf("BRAM ... bram_word_length: %i, bram_size_byte: %i\n", bram_word_length, bram_size_byte);

    this->memory = (uint8_t*)(mmap(
        NULL, bram_size_byte, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));

    if (this->memory == ((uint8_t*)MAP_FAILED)) {
        printf_error("# [Cache BRAM] Allocating memory failure ... MMAP");
        perror("Cache BRAM");
    }
}

void Bram::read(uint32_t addr, uint8_t* data) {
    auto base_addr = addr * bram_word_length;
    for (int i = 0; i < bram_word_length; ++i) {
        data[i] = memory[base_addr+i];
    }
}

void Bram::write(uint32_t addr, const uint8_t* data) {
    auto base_addr = addr * bram_word_length;
    for (int i = 0; i < bram_word_length; ++i) {
        memory[base_addr+i] = data[i];
    }
}

bool Bram::get_accessed_this_cycle() {
    return (accessed_this_cycle >= dcma_dataword_length_byte / dma_dataword_length_byte);
}

void Bram::set_accessed_this_cycle(bool access) {
    if (access)
        accessed_this_cycle += 1;
    else
        accessed_this_cycle = 0;
}
