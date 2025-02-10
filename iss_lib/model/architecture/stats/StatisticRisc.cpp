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

#include "StatisticRisc.h"
#include "../../../simulator/ISS.h"
#include "../../../simulator/helper/debugHelper.h"

#include "JSONHelpers.h"

StatisticRisc::StatisticRisc(ISS* core) : StatisticBase(core) {}

void StatisticRisc::tick() {
    StatisticBase::tick();
}

void StatisticRisc::print(QString& output) {
    QTextStream out(&output);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(2);

    out << "[Risc] Statistics, Clock: " << MAGENTA << 1000 / core->getRiscClockPeriod() << " MHz"
        << RESET_COLOR << ", Total Clock Ticks: " << total_ticks
        << ", Runtime: " << total_ticks * core->getRiscClockPeriod() << "ns \n";
    out << "  [AUX Counter] " << LIGHT << "may have reset by Application!\n" << RESET_COLOR;

    uint64_t lane = core->aux_cnt_lane_act;
    uint64_t dma = core->aux_cnt_dma_act;
    uint64_t both = core->aux_cnt_both_act;
    uint64_t tot = core->aux_cnt_vpro_total;
    uint64_t tot2 = core->aux_cnt_riscv_total;
    uint64_t risc = core->aux_cnt_riscv_enabled;

    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setFieldAlignment(QTextStream::AlignRight);
    out.setRealNumberPrecision(2);
    out.setFieldWidth(16);

    out << "  [AUX Counter] (any) Lane active:               " << lane;
    out.setFieldWidth(6);
    out << " Cycles (" << ((tot > 0) ? 100. * lane / tot : 0.0) << " %)      \n";
    out.setFieldWidth(16);
    out << "  [AUX Counter] (any) DMA active:                " << dma;
    out.setFieldWidth(6);
    out << " Cycles (" << ((tot > 0) ? 100. * dma / tot : 0.0) << " %)      \n";
    out.setFieldWidth(16);
    out << "  [AUX Counter] (any) Lane AND (any) DMA active: " << both;
    out.setFieldWidth(6);
    out << " Cycles (" << ((tot > 0) ? 100. * both / tot : 0.0) << " %)      \n";
    out.setFieldWidth(16);
    out << "  [AUX Counter] EISV Total:                      " << tot;
    out.setFieldWidth(6);
    out << " Cycles \n";
    out.setFieldWidth(16);
    out << "  [AUX Counter] VPRO Total:                      " << tot2;
    out.setFieldWidth(6);
    out << " Cycles \n";
    out.setFieldWidth(16);
    out << "  [AUX Counter] Not Synchronizing VPRO:          " << risc;
    out.setFieldWidth(6);
    out << " Cycles (" << ((tot2 > 0) ? 100. * risc / tot2 : 0.0) << " % Busy)  \n";
    out << "\n";
}

void StatisticRisc::print_json(QString& output) {
    uint64_t lane = core->aux_cnt_lane_act;
    uint64_t dma = core->aux_cnt_dma_act;
    uint64_t both = core->aux_cnt_both_act;
    uint64_t tot = core->aux_cnt_vpro_total;
    uint64_t tot2 = core->aux_cnt_riscv_total;
    uint64_t risc = core->aux_cnt_riscv_enabled;

    QTextStream out(&output);
    out << JSON_OBJ_BEGIN;
    out << JSON_FIELD_FLOAT("clock_period", core->getRiscClockPeriod()) << ",";
    out << JSON_FIELD_INT("total_ticks", total_ticks) << ",";

    out << JSON_FIELD_INT("any_lane", lane) << ",";
    out << JSON_FIELD_INT("any_dma", dma) << ",";
    out << JSON_FIELD_INT("any_lane_or_dma", both) << ",";
    out << JSON_FIELD_INT("eisv_cycles", tot) << ",";
    out << JSON_FIELD_INT("vpro_cycles", tot2) << ",";
    out << JSON_FIELD_INT("vpro_busy", risc);
    out << JSON_OBJ_END;
}

void StatisticRisc::reset() {
    StatisticBase::reset();

    core->aux_cnt_lane_act = 0;
    core->aux_cnt_dma_act = 0;
    core->aux_cnt_both_act = 0;
    core->aux_cnt_vpro_total = 0;
    core->aux_cnt_riscv_total = 0;
    core->aux_cnt_riscv_enabled = 0;
}
