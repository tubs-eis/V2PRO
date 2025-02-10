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

#include "StatisticDma.h"
#include "../../../simulator/ISS.h"
#include "../../../simulator/helper/debugHelper.h"

#include "JSONHelpers.h"

StatisticDma::StatisticDma(ISS* core) : StatisticBase(core) {}

void StatisticDma::tick() {
    StatisticBase::tick();

    bool DMAActive = false;

    for (auto cluster : core->getClusters()) {
        if (cluster->dma->isBusy()) {
            DMAActive = true;
            totalDMAActive++;
        } else {
            totalDMAInActive++;
        }
    }
    if (DMAActive) {
        anyDMAActive++;
    }
}

void StatisticDma::addExecutedCommand(const CommandDMA* cmd, const int& cluster) {
    uint32_t elements = cmd->x_size * cmd->y_size;

    switch (cmd->type) {
        case CommandDMA::EXT_1D_TO_LOC_1D:
            executedCommands[cluster].e2l_1d_transfer_cmds++;
            executedCommands[cluster].e2l_elements_transferred += elements;
            break;
        case CommandDMA::EXT_2D_TO_LOC_1D:
            executedCommands[cluster].e2l_2d_transfer_cmds++;
            executedCommands[cluster].e2l_elements_transferred += elements;
            break;
        case CommandDMA::LOC_1D_TO_EXT_2D:
            executedCommands[cluster].l2e_2d_transfer_cmds++;
            executedCommands[cluster].l2e_elements_transferred += elements;
            break;
        case CommandDMA::LOC_1D_TO_EXT_1D:
            executedCommands[cluster].l2e_1d_transfer_cmds++;
            executedCommands[cluster].l2e_elements_transferred += elements;
            break;
        case CommandDMA::NONE:
        case CommandDMA::WAIT_FINISH:
        case CommandDMA::enumTypeEnd:
            break;
    }
}

void StatisticDma::print(QString& output) {
    unsigned long DMAsTotal = total_ticks * VPRO_CFG::CLUSTERS;

    QTextStream out(&output);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(2);

    out << "[DMA]  Statistics, Clock: " << MAGENTA << 1000 / core->getDMAClockPeriod() << " MHz"
        << RESET_COLOR << ", Total Clock Ticks: " << total_ticks
        << ", Runtime: " << total_ticks * core->getDMAClockPeriod() << "ns \n";

    // DMA commands stats
    executedCommands_s allCmds;
    for (int i = 0; i < VPRO_CFG::CLUSTERS; ++i) {
        allCmds += executedCommands[i];
    }
    out << "  Executed Commands: \n";
    out << "    E2L: " << (allCmds.e2l_1d_transfer_cmds + allCmds.e2l_2d_transfer_cmds)
        << " (1D: " << allCmds.e2l_1d_transfer_cmds << ", 2D: " << allCmds.e2l_2d_transfer_cmds
        << ") with total " << allCmds.e2l_elements_transferred << " transferred elements\n";
    out << "    L2E: " << (allCmds.l2e_1d_transfer_cmds + allCmds.l2e_2d_transfer_cmds)
        << " (1D: " << allCmds.l2e_1d_transfer_cmds << ", 2D: " << allCmds.l2e_2d_transfer_cmds
        << ") with total " << allCmds.l2e_elements_transferred << " transferred elements\n";

    // Total Bandwidth in MB/s
    float total_bandwidth =
        2 * 1000 *
        (float(allCmds.e2l_elements_transferred + allCmds.l2e_elements_transferred) /
            float(total_ticks * core->getDMAClockPeriod()));
    out << "    Average DMA Bandwidth: " << total_bandwidth << " MB/s\n";

    // !busy + active + src stall + dst stall = 100 %
    if (100 * (double)(totalDMAActive) / (anyDMAActive * VPRO_CFG::CLUSTERS) < 99) {
        out << "  [Parallel Architecture utilization] (total DMA active of at least one DMA active "
               "cycles "
            << "[over all clusters]):\n";
        out << "      DMAs:  " << ORANGE
            << 100 * (double)(totalDMAActive) / (anyDMAActive * VPRO_CFG::CLUSTERS) << "%"
            << RESET_COLOR << " (not all DMAs are active together)\n"
            << LIGHT;
        out << "      Possible Reason: DMA instruction issue has offset between Clusters, "
               "different delay (DCMA) "
            << "or length varies between DMAs. \n\n"
            << RESET_COLOR;
    } else {
        out << "  [Parallel Architecture utilization] is >= 99 %%. All DMAs are active together!\n";
    }
    out << "\n  [Algorithm utilization] (active cycles of total cycles, average over all DMAs, "
        << "may overlap with VPRO execution, represents DMA usage):\n"
        << RESET_COLOR;
    out << "      DMA:    " << 100 * (double)totalDMAActive / DMAsTotal << "% \n";
    out << "\n";
}

void StatisticDma::print_json(QString& output) {
    unsigned long DMAsTotal = total_ticks * VPRO_CFG::CLUSTERS;

    QTextStream out(&output);
    out << JSON_OBJ_BEGIN;
    out << JSON_FIELD_FLOAT("clock_period", core->getDMAClockPeriod()) << ",";
    out << JSON_FIELD_INT("total_ticks", total_ticks) << ",";
    out << JSON_FIELD_INT("total_active", totalDMAActive) << ",";
    out << JSON_FIELD_INT("any_active", anyDMAActive) << ",";
    out << JSON_FIELD_INT("dma_count", VPRO_CFG::CLUSTERS) << ",";
    out << JSON_FIELD_INT("total_dma_ticks", DMAsTotal) << ",";

    out << JSON_FIELD_FLOAT("architecture_utilization",
               (double)(totalDMAActive) / (anyDMAActive * VPRO_CFG::CLUSTERS))
        << ",";
    out << JSON_FIELD_FLOAT("algorithm_utilization", (double)totalDMAActive / DMAsTotal);
    out << JSON_OBJ_END;
}

void StatisticDma::reset() {
    StatisticBase::reset();

    totalDMAActive = 0;
    totalDMAInActive = 0;
    anyDMAActive = 0;
}
