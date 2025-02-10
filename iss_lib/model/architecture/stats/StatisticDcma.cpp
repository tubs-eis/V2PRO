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

#include "StatisticDcma.h"
#include "../../../simulator/ISS.h"
#include "../../../simulator/helper/debugHelper.h"

#include "JSONHelpers.h"

StatisticDcma::StatisticDcma(ISS* core) : StatisticBase(core) {
    uint32_t num_cluster = VPRO_CFG::CLUSTERS;
    cycle_counters.did_dma_read_hit = std::vector<uint32_t>(num_cluster, 0);
    cycle_counters.did_dma_read_hit_but_busy = std::vector<uint32_t>(num_cluster, 0);
    cycle_counters.did_dma_read_miss = std::vector<uint32_t>(num_cluster, 0);
    cycle_counters.did_dma_write_hit = std::vector<uint32_t>(num_cluster, 0);
    cycle_counters.did_dma_write_hit_but_busy = std::vector<uint32_t>(num_cluster, 0);
    cycle_counters.did_dma_write_miss = std::vector<uint32_t>(num_cluster, 0);
    counters.dma_read_hit_cycle_counter = std::vector<uint32_t>(num_cluster + 1, 0);
    counters.dma_read_stall_cycle_counter = std::vector<uint32_t>(num_cluster + 1, 0);
    counters.dma_write_hit_cycle_counter = std::vector<uint32_t>(num_cluster + 1, 0);
    counters.dma_write_stall_cycle_counter = std::vector<uint32_t>(num_cluster + 1, 0);
    counters.read_hit_access_counter = 0;
    counters.read_miss_access_counter = 0;
    counters.write_hit_access_counter = 0;
    counters.write_miss_access_counter = 0;
    dma_access_counter.read_hit_cycles = std::vector<uint32_t>(num_cluster, 0);
    dma_access_counter.read_miss_cycles = std::vector<uint32_t>(num_cluster, 0);
    dma_access_counter.write_hit_cycles = std::vector<uint32_t>(num_cluster, 0);
    dma_access_counter.write_miss_cycles = std::vector<uint32_t>(num_cluster, 0);
}

void StatisticDcma::tick() {
    StatisticBase::tick();

    uint32_t number_cluster = VPRO_CFG::CLUSTERS;

    std::vector<uint32_t> sum(6, 0);
    // count dma accesses in last cycle
    for (int i = 0; i < number_cluster; ++i) {
        sum[0] += cycle_counters.did_dma_read_hit[i];
        sum[1] += cycle_counters.did_dma_read_hit_but_busy[i];
        sum[2] += cycle_counters.did_dma_read_miss[i];
        sum[3] += cycle_counters.did_dma_write_hit[i];
        sum[4] += cycle_counters.did_dma_write_hit_but_busy[i];
        sum[5] += cycle_counters.did_dma_write_miss[i];

        dma_access_counter.read_hit_cycles[i] += cycle_counters.did_dma_read_hit[i];
        dma_access_counter.read_miss_cycles[i] +=
            cycle_counters.did_dma_read_hit_but_busy[i] + cycle_counters.did_dma_read_miss[i];
        dma_access_counter.write_hit_cycles[i] += cycle_counters.did_dma_write_hit[i];
        dma_access_counter.write_miss_cycles[i] +=
            cycle_counters.did_dma_write_hit_but_busy[i] + cycle_counters.did_dma_write_miss[i];
    }
    counters.dma_read_hit_cycle_counter[sum[0]]++;
    counters.dma_read_stall_cycle_counter[sum[1]]++;
    counters.dma_read_stall_cycle_counter[sum[2]]++;
    counters.dma_write_hit_cycle_counter[sum[3]]++;
    counters.dma_write_stall_cycle_counter[sum[4]]++;
    counters.dma_write_stall_cycle_counter[sum[5]]++;

    counters.read_hit_cyle_counter += sum[0];
    counters.read_hit_but_busy_cycle_counter += sum[1];
    counters.read_miss_cycle_counter += sum[2];
    counters.write_hit_cycle_counter += sum[3];
    counters.write_hit_but_busy_cyle_counter += sum[4];
    counters.write_miss_cycle_counter += sum[5];

    cycle_counters.did_dma_read_hit = std::vector<uint32_t>(number_cluster, 0);
    cycle_counters.did_dma_read_hit_but_busy = std::vector<uint32_t>(number_cluster, 0);
    cycle_counters.did_dma_read_miss = std::vector<uint32_t>(number_cluster, 0);
    cycle_counters.did_dma_write_hit = std::vector<uint32_t>(number_cluster, 0);
    cycle_counters.did_dma_write_hit_but_busy = std::vector<uint32_t>(number_cluster, 0);
    cycle_counters.did_dma_write_miss = std::vector<uint32_t>(number_cluster, 0);

    if (core->dcma->isBusy()) counters.dcma_busy_cycles++;
}

void StatisticDcma::reset() {
    StatisticBase::reset();

    uint32_t number_cluster = VPRO_CFG::CLUSTERS;
    cycle_counters = CycleCounters();
    counters = AllCounters();
    cycle_counters.did_dma_read_hit = std::vector<uint32_t>(number_cluster, 0);
    cycle_counters.did_dma_read_hit_but_busy = std::vector<uint32_t>(number_cluster, 0);
    cycle_counters.did_dma_read_miss = std::vector<uint32_t>(number_cluster, 0);
    cycle_counters.did_dma_write_hit = std::vector<uint32_t>(number_cluster, 0);
    cycle_counters.did_dma_write_hit_but_busy = std::vector<uint32_t>(number_cluster, 0);
    cycle_counters.did_dma_write_miss = std::vector<uint32_t>(number_cluster, 0);
    counters.dma_read_hit_cycle_counter = std::vector<uint32_t>(number_cluster + 1, 0);
    counters.dma_read_stall_cycle_counter = std::vector<uint32_t>(number_cluster + 1, 0);
    counters.dma_write_hit_cycle_counter = std::vector<uint32_t>(number_cluster + 1, 0);
    counters.dma_write_stall_cycle_counter = std::vector<uint32_t>(number_cluster + 1, 0);
    counters.read_hit_access_counter = 0;
    counters.read_miss_access_counter = 0;
    counters.write_hit_access_counter = 0;
    counters.write_miss_access_counter = 0;
    dma_access_counter.read_hit_cycles = std::vector<uint32_t>(number_cluster, 0);
    dma_access_counter.read_miss_cycles = std::vector<uint32_t>(number_cluster, 0);
    dma_access_counter.write_hit_cycles = std::vector<uint32_t>(number_cluster, 0);
    dma_access_counter.write_miss_cycles = std::vector<uint32_t>(number_cluster, 0);
}

void StatisticDcma::print(QString& output) {
    uint32_t number_cluster = VPRO_CFG::CLUSTERS;
    QTextStream out(&output);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(2);

    out << "[DCMA] Statistics, Clock: " << MAGENTA << 1000 / core->getDCMAClockPeriod() << " MHz"
        << RESET_COLOR << ", Total Clock Ticks: " << total_ticks
        << ", Runtime: " << total_ticks * core->getDCMAClockPeriod() << "ns \n";

    uint32_t total_read = counters.read_hit_cyle_counter +
                          counters.read_hit_but_busy_cycle_counter +
                          counters.read_miss_cycle_counter;
    uint32_t total_write = counters.write_hit_cycle_counter +
                           counters.write_hit_but_busy_cyle_counter +
                           counters.write_miss_cycle_counter;

    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(2);

    uint32_t read_hit_access_counter_corrected = counters.read_hit_access_counter *
                                                 core->dcma->dma_dataword_length_byte /
                                                 core->dcma->dcma_dataword_length_byte;
    uint32_t write_hit_access_counter_corrected = counters.write_hit_access_counter *
                                                  core->dcma->dma_dataword_length_byte /
                                                  core->dcma->dcma_dataword_length_byte;

    out << "  DCMA Metric Counters"
        << "\n";
    if (total_ticks == 0)
        out << "  Ext Mem Access Busy:      " << 0 << "%\n\n";
    else
        out << "  Ext Mem Access Busy:      "
            << float(counters.dcma_busy_cycles) / total_ticks * 100 << "%\n";
    out << "  Read Hit Rate:            "
        << 100 * float(read_hit_access_counter_corrected) /
               (read_hit_access_counter_corrected + counters.read_miss_access_counter)
        << "%  [" << read_hit_access_counter_corrected + counters.read_miss_access_counter
        << " total 64 bit read accesses]\n";
    out << "  Write Hit Rate:           "
        << 100 * float(write_hit_access_counter_corrected) /
               (write_hit_access_counter_corrected + counters.write_miss_access_counter)
        << "%  [" << write_hit_access_counter_corrected + counters.write_miss_access_counter
        << " total 64 bit write accesses]\n\n";

    out << "  Read Cycles:              " << total_read << "\n";

    out << "      Hit Cycles:           " << counters.read_hit_cyle_counter << "  ["
        << 100 * float(counters.read_hit_cyle_counter) / float(total_read) << "%]\n";

    out << "      Hit But Busy Cycles:  " << counters.read_hit_but_busy_cycle_counter << "  ["
        << 100 * float(counters.read_hit_but_busy_cycle_counter) / float(total_read) << "%]"
        << "\n";

    out << "      Miss Cycles:          " << counters.read_miss_cycle_counter << "  ["
        << 100 * float(counters.read_miss_cycle_counter) / float(total_read) << "%]\n";

    out << "  Write Cycles:             " << total_write << "\n";

    out << "      Hit Cycles:           " << counters.write_hit_cycle_counter << "  ["
        << 100 * float(counters.write_hit_cycle_counter) / float(total_write) << "%]\n";

    out << "      Hit But Busy Cycles:  " << counters.write_hit_but_busy_cyle_counter << "  ["
        << 100 * float(counters.write_hit_but_busy_cyle_counter) / float(total_write) << "%]"
        << "\n";

    out << "      Miss Cycles:          " << counters.write_miss_cycle_counter << "  ["
        << 100 * float(counters.write_miss_cycle_counter) / float(total_write) << "%]\n";

    uint32_t total_dma_rdata_served = 0;
    uint32_t total_dma_wdata_served = 0;
    for (int i = 1; i < number_cluster + 1; ++i) {
        total_dma_rdata_served += i * counters.dma_read_hit_cycle_counter[i];
        total_dma_wdata_served += i * counters.dma_write_hit_cycle_counter[i];
    }
    out << "\n";

    out << "  Bus Counters"
        << "\n";
    out << "      Read Cycles:  " << counters.bus_read_cycles << "\n";
    out << "      Write Cycles: " << counters.bus_write_cycles << "\n";
    out << "      Wait Cycles:  " << counters.bus_wait_cycles << "\n";
    out << "  Average received Bus Read Data per Cycle in "
        << core->dcma->bus_dataword_length_byte * 8
        << "-Bit-Words: " << float(counters.bus_read_cycles) / float(total_ticks) << "\n";
    out << "  Average send Bus Write Data per Cycle in " << core->dcma->bus_dataword_length_byte * 8
        << "-Bit-Words:    " << float(counters.bus_write_cycles) / float(total_ticks) << "\n";
    out << "  Total Bytes read (Bus -> DCMA):                            "
        << counters.bus_read_cycles * core->dcma->bus_dataword_length_byte << "\n";
    out << "  Total Bytes written (DCMA -> Bus):                         "
        << counters.bus_write_cycles * core->dcma->bus_dataword_length_byte << "\n";
    out << "  Average DCMA <-> Bus Bandwidth [GBytes/s]:                 "
        << float(counters.bus_read_cycles + counters.bus_write_cycles) / float(total_ticks) *
               float(core->dcma->bus_dataword_length_byte) / float(core->getDCMAClockPeriod())
        << "\n";
    out << "\n";
}

void StatisticDcma::print_json(QString& output) {
    QTextStream out(&output);
    out << JSON_OBJ_BEGIN;
    out << JSON_FIELD_FLOAT("clock_period", core->getDCMAClockPeriod()) << ",";
    out << JSON_FIELD_INT("total_ticks", total_ticks);
    out << JSON_OBJ_END;
}
