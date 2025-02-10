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

#ifndef TEMPLATE_BRAM_H
#define TEMPLATE_BRAM_H

#include <inttypes.h>
#include <vector>

class Bram {
   public:
    Bram(uint32_t bram_size_byte, uint32_t bram_word_length);

    void read(uint32_t addr, uint8_t* data);

    void write(uint32_t addr, const uint8_t* data);

    bool get_accessed_this_cycle();

    void set_accessed_this_cycle(bool access);

   private:
    constexpr static int dma_dataword_length_byte = 16 / 8;
    constexpr static int dcma_dataword_length_byte = 128 / 8;

    uint32_t bram_word_length;  // in bytes
    uint32_t bram_size_byte;         // in words
    uint32_t accessed_this_cycle = 0;

//    std::vector<std::vector<uint8_t>> memory;
    uint8_t *memory;
};

#endif  //TEMPLATE_BRAM_H
