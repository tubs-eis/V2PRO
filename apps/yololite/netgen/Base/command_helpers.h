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
#ifndef COMMAND_HELPERS_H
#define COMMAND_HELPERS_H 

#include <assert.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include "bif.h"

//FIXME: where should DMA_DESCRIPTOR actually be defined?
namespace DMA_COMMANDS {

  // copied from cnn_converter/vpro_lib_hw/includes/
  struct DMA_DESCRIPTOR {
    bool isMM_Kernel_offset{};
    bool isMM_Bias_offset{};

    DMA_DIRECTION dir{};
    uint32_t cluster{};
    uint32_t unit{};
    uint64_t mm_addr{};
    uint32_t lm_addr{};
    uint32_t x_size{};
    uint32_t y_size{};
    uint32_t word_count{};
    uint32_t y_leap{};

    bool pad[4]{};
    uint8_t skipped_elements_at_end{0};


    BIF::COMMAND_SEGMENT load(const uint32_t &unit_mask) {
        BIF::COMMAND_SEGMENT cmd;
        cmd.type.type = DMA_CMD;
        cmd.dma.direction = dir;
        assert(dir != l2e1D);
        assert(dir != l2e2D);
        cmd.dma.cluster = cluster;
        cmd.dma.unit_mask = unit_mask;

        cmd.dma.isBiasOffset = isMM_Bias_offset;
        cmd.dma.isKernelOffset = isMM_Kernel_offset;
        cmd.dma.skipped_elements_at_end = skipped_elements_at_end;

        cmd.dma.mm_addr = mm_addr;
        cmd.dma.lm_addr = lm_addr;
        if (dir == e2l1D) {
            cmd.dma.x_size = word_count;
            cmd.dma.y_size = 1;
            cmd.dma.y_leap = 0;
        } else {
            cmd.dma.y_leap = y_leap;
            cmd.dma.x_size = x_size;
            cmd.dma.y_size = y_size;
            cmd.dma.padding = (pad[3] << 3) | (pad[2] << 2) | (pad[1] << 1) | (pad[0] << 0);
        }
        return cmd;
    }

    static std::vector<BIF::COMMAND_SEGMENT> startBroadcastLoad(std::vector<DMA_DESCRIPTOR> dmas_1d, std::vector<DMA_DESCRIPTOR> dmas_2d) {
        std::vector<BIF::COMMAND_SEGMENT> dma_commands;
        dma_commands.reserve(dmas_1d.size() + dmas_2d.size());
        if (!dmas_1d.empty()) {
            std::stable_sort(dmas_1d.begin(), dmas_1d.end(),
                    [](const DMA_DESCRIPTOR &d1, const DMA_DESCRIPTOR &d2) -> bool {
                        return d1.mm_addr < d2.mm_addr; // sort by mm
                    });
            DMA_DESCRIPTOR &starter = dmas_1d[0];
            uint32_t unit_mask = uint32_t(0b1u) << starter.unit;
            for (auto &dma: dmas_1d) {
                assert(dma.dir == l2e1D || dma.dir == e2l1D);
                if (dma.mm_addr == starter.mm_addr &&
                    dma.lm_addr == starter.lm_addr &&
                    dma.word_count == starter.word_count &&
                    dma.cluster == starter.cluster &&
                    dma.pad[0] == starter.pad[0] && dma.pad[1] == starter.pad[1] && dma.pad[2] == starter.pad[2] && dma.pad[3] == starter.pad[3] &&
                    dma.isMM_Bias_offset == starter.isMM_Bias_offset &&
                    dma.isMM_Kernel_offset == starter.isMM_Kernel_offset &&
                    dma.skipped_elements_at_end == starter.skipped_elements_at_end) {
                    unit_mask |= uint32_t(0b1u) << dma.unit;
                } else {
                    dma_commands.push_back(starter.load(unit_mask));
                    starter = dma;
                    unit_mask = uint32_t(0b1u) << dma.unit;
                }
            }
            dma_commands.push_back(starter.load(unit_mask));
        }
        if (!dmas_2d.empty()) {
            std::stable_sort(dmas_2d.begin(), dmas_2d.end(),
                    [](const DMA_DESCRIPTOR &d1, const DMA_DESCRIPTOR &d2) -> bool {
                        return d1.mm_addr < d2.mm_addr; // sort by mm
                    });
            DMA_DESCRIPTOR &starter = dmas_2d[0];
            uint32_t unit_mask = uint32_t(0b1u) << starter.unit;
            for (auto &dma: dmas_2d) {
                assert(dma.dir == l2e2D || dma.dir == e2l2D);
                if (dma.mm_addr == starter.mm_addr &&
                    dma.lm_addr == starter.lm_addr &&
                    dma.y_leap == starter.y_leap &&
                    dma.y_size == starter.y_size &&
                    dma.x_size == starter.x_size &&
                    dma.cluster == starter.cluster &&
                    dma.pad[0] == starter.pad[0] && dma.pad[1] == starter.pad[1] && dma.pad[2] == starter.pad[2] && dma.pad[3] == starter.pad[3] &&
                    dma.isMM_Bias_offset == starter.isMM_Bias_offset &&
                    dma.isMM_Kernel_offset == starter.isMM_Kernel_offset &&
                    dma.skipped_elements_at_end == starter.skipped_elements_at_end) {
                    unit_mask |= uint32_t(0b1u) << dma.unit;
                } else {
                    dma_commands.push_back(starter.load(unit_mask));
                    starter = dma;
                    unit_mask = uint32_t(0b1u) << dma.unit;
                }
            }
            dma_commands.push_back(starter.load(unit_mask));
        }
        return dma_commands;
    }

    static BIF::COMMAND_SEGMENT wait() {
      BIF::COMMAND_SEGMENT cmd;
      cmd.type.type = DMA_WAIT;
      return cmd;
    }
    
  }; // struct DMA_DESCRIPTOR

} // namespace DMA_COMMANDS

namespace VPRO_COMMANDS {
  BIF::COMMAND_SEGMENT wait();
  BIF::COMMAND_SEGMENT sync();
}

void next_hardware_element(unsigned int &cluster, unsigned int &unit, unsigned int &lane);

#endif
