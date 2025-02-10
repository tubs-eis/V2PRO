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
// Created by gesper on 26.05.23.
//

#ifndef TEMPLATE_DMALOOPER_H
#define TEMPLATE_DMALOOPER_H

#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <list>
#include <memory>
#include <vector>

#include "../../../common_lib/vpro/dma_cmd_struct.h"
#include "../../simulator/helper/debugHelper.h"

class ISS;

class DMALooper {
   public:
    explicit DMALooper(ISS* core);

    [[nodiscard]] bool isBusy() const {
        return busy;
    };

    void new_dcache_input(const uint8_t dcache_data_struct[32]);

    void tick();

   private:
    ISS* core;

    bool busy{false};

    COMMAND_DMA::COMMAND_DMA base_dma_cmd;
    COMMAND_DMA::COMMAND_DMA_LOOP dma_loop;

    enum state_t {
        IDLE,
        WAIT_FOR_BASE,
        LOOPING
    } state{IDLE};

    struct input_register_t {
        uint8_t dcache_data_struct[32]{};
        bool is_filled{false};
    } input_register{};

    // values during loop
    uint32_t base_mm_addr{};
    uint32_t base_lm_addr{};
    uint32_t base_unit_mask{};
    uint32_t base_cluster_mask{};

    uint32_t mm_addr{};
    uint32_t lm_addr{};
    uint32_t unit_mask{};
    uint32_t cluster_mask{};

    // loop incremented values
    uint32_t cluster{}, unit{}, inter_unit{};

    uint32_t generated_commands{}, total_generated_commands{0};

    // helper
    std::shared_ptr<CommandDMA> bif_to_dma(COMMAND_DMA::COMMAND_DMA* dma);

    static int16_t signext_13to16(int16_t val) {
        if (val & 0x1000) {  // bit position: 13
            return int16_t(int(val) | 0xE000);
        }
        return val & 0x1fff;
    }
};

#endif  //TEMPLATE_DMALOOPER_H
