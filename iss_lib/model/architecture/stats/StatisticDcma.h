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
// Created by gesper on 24.06.22.
//

#ifndef CONV2DADD_STATISTICDCMA_H
#define CONV2DADD_STATISTICDCMA_H

#include "StatisticBase.h"

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

class StatisticDcma : public StatisticBase {
   public:
    struct CycleCounters {
        std::vector<uint32_t> did_dma_read_hit;
        std::vector<uint32_t> did_dma_read_hit_but_busy;
        std::vector<uint32_t> did_dma_read_miss;
        std::vector<uint32_t> did_dma_write_hit;
        std::vector<uint32_t> did_dma_write_hit_but_busy;
        std::vector<uint32_t> did_dma_write_miss;
    } cycle_counters;

    struct AllCounters {
        uint32_t read_hit_cyle_counter = 0;
        uint32_t read_hit_but_busy_cycle_counter = 0;
        uint32_t read_miss_cycle_counter = 0;
        uint32_t write_hit_cycle_counter = 0;
        uint32_t write_hit_but_busy_cyle_counter = 0;
        uint32_t write_miss_cycle_counter = 0;
        std::vector<uint32_t> dma_read_hit_cycle_counter;
        std::vector<uint32_t> dma_read_stall_cycle_counter;
        std::vector<uint32_t> dma_write_hit_cycle_counter;
        std::vector<uint32_t> dma_write_stall_cycle_counter;
        uint32_t bus_write_cycles = 0;
        uint32_t bus_read_cycles = 0;
        uint32_t bus_wait_cycles = 0;
        uint32_t dcma_busy_cycles = 0;
        uint32_t read_hit_access_counter = 0;
        uint32_t read_miss_access_counter = 0;
        uint32_t write_hit_access_counter = 0;
        uint32_t write_miss_access_counter = 0;
    } counters;

    struct DmaAccessCounters {
        std::vector<uint32_t> read_hit_cycles;
        std::vector<uint32_t> read_miss_cycles;
        std::vector<uint32_t> write_hit_cycles;
        std::vector<uint32_t> write_miss_cycles;
    } dma_access_counter;

    explicit StatisticDcma(ISS* core);

    void tick() override;

    void print(QString& output) override;
    void print_json(QString& output) override;

    void reset() override;
};

#endif  //CONV2DADD_STATISTICDCMA_H
