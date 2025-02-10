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

#include "StatisticVpro.h"
#include "../../../simulator/ISS.h"
#include "../../../simulator/helper/debugHelper.h"

#include "JSONHelpers.h"

StatisticVpro::StatisticVpro(ISS* core) : StatisticBase(core) {
    typeCount = std::vector<std::map<CommandVPRO::TYPE, double[2]>>(VPRO_CFG::LANES + 1);
}

void StatisticVpro::tick() {
    StatisticBase::tick();

    bool L0active = false;
    bool L1active = false;
    bool LSactive = false;

    for (auto cluster : core->getClusters()) {
        for (auto unit : cluster->getUnits()) {
            auto lane0 = unit->getLanes()[0];
            if (lane0->isBusy()) {
                L0active = true;
                if (lane0->is_src_lane_stalling()) {
                    totalL0LanesSrcStall++;
                } else if (lane0->is_dst_lane_stalling()) {
                    totalL0LanesDstStall++;
                } else {
                    totalL0LanesActive++;
                }
                if (lane0->isBlocking()) {
                    totalLSLanesBlocking++;
                }
            } else {
                totalL0LanesInActive++;
            }

            auto lane1 = unit->getLanes()[1];
            if (lane1->isBusy()) {
                L1active = true;
                if (lane1->is_src_lane_stalling()) {
                    totalL1LanesSrcStall++;
                } else if (lane1->is_dst_lane_stalling()) {
                    totalL1LanesDstStall++;
                } else {
                    totalL1LanesActive++;
                }
                if (lane1->isBlocking()) {
                    totalLSLanesBlocking++;
                }
            } else {
                totalL1LanesInActive++;
            }

            auto& lanels = unit->getLSLane();
            if (lanels.isBusy()) {
                LSactive = true;
                if (lanels.is_src_lane_stalling()) {
                    totalLSLanesSrcStall++;
                } else if (lanels.is_dst_lane_stalling()) {
                    totalLSLanesDstStall++;
                } else {
                    totalLSLanesActive++;
                }
                if (lanels.isBlocking()) {
                    totalLSLanesBlocking++;
                }
            } else {
                totalLSLanesInActive++;
            }
            //            }
        }
    }

    if (L0active) anyL0LaneActive++;
    if (L1active) anyL1LaneActive++;
    if (LSactive) anyLSLaneActive++;
}

double StatisticVpro::getCyclesNotNONE(int lane) {
    double sum = 0;
    for (auto& it : typeCount[lane]) {
        sum += it.second[1];
    }
    sum -= getCyclesNONE(lane);
    return sum;
}

double StatisticVpro::getCyclesNONE(int lane) {
    if (typeCount[lane].find(CommandVPRO::NONE) == typeCount[lane].end()) return 0;
    return typeCount[lane][CommandVPRO::NONE][1];
}

void StatisticVpro::addExecutedCmdQueue(CommandVPRO* cmd, int vector_lane_id) {
    typeCount[vector_lane_id][cmd->type][0]++;
}

void StatisticVpro::addExecutedCmdTick(CommandVPRO* cmd, int vector_lane_id) {
    typeCount[vector_lane_id][cmd->type][1]++;
}

void StatisticVpro::print(QString& output) {
    uint32_t parallelUnits = VPRO_CFG::UNITS * VPRO_CFG::CLUSTERS;
    unsigned long LaneTotal = total_ticks * parallelUnits;

    QTextStream out(&output);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(2);

    out << "[VPRO] Statistics, Clock: " << MAGENTA << 1000 / core->getVPROClockPeriod() << " MHz"
        << RESET_COLOR << ", Total Clock Ticks: " << total_ticks
        << ", Runtime: " << total_ticks * core->getVPROClockPeriod() << "ns \n";

    // getCmd   // TODO instr. statistics
    // no src / dst stall -> cmd is executed (1 vector element is processed)
    // => active

    out.setFieldAlignment(QTextStream::AlignRight);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(2);
    out.setFieldWidth(5);
    // !busy + active + src stall + dst stall = 100 %
    if (100 * (double)(totalL0LanesActive + totalL0LanesSrcStall + totalL0LanesDstStall) /
                (anyL0LaneActive * parallelUnits) <
            99 ||
        100 * (double)(totalL1LanesActive + totalL1LanesSrcStall + totalL1LanesDstStall) /
                (anyL1LaneActive * parallelUnits) <
            99 ||
        100 * (double)(totalLSLanesActive + totalLSLanesSrcStall + totalLSLanesDstStall) /
                (anyLSLaneActive * parallelUnits) <
            99) {
        out << "  [Parallel Architecture utilization] (lane<x> active cycles of max. possible "
               "active cycles [over all units/clusters]):"
            << RESET_COLOR << "\n";
        out << "      Lane 0:  " << ORANGE
            << 100 * (double)(totalL0LanesActive + totalL0LanesSrcStall + totalL0LanesDstStall) /
                   (anyL0LaneActive * parallelUnits)
            << "%" << RESET_COLOR << " (should be @ 100%!)\n";
        out << "      Lane 1:  " << ORANGE
            << 100 * (double)(totalL1LanesActive + totalL1LanesSrcStall + totalL1LanesDstStall) /
                   (anyL1LaneActive * parallelUnits)
            << "%" << RESET_COLOR << " (should be @ 100%!)\n";
        out << "      Lane LS: " << ORANGE
            << 100 * (double)(totalLSLanesActive + totalLSLanesSrcStall + totalLSLanesDstStall) /
                   (anyLSLaneActive * parallelUnits)
            << "%" << RESET_COLOR << " (should be @ 100%!)\n";
        out << LIGHT << "      Possible Reason: Cluster / Unit Mask is set" << RESET_COLOR
            << "\n\n";
    } else {
        out << " [Parallel Architecture utilization] is >= 99 %%. Units and Clusters are all in "
               "parallel use!\n";
    }
    out << "\n  [Algorithm utilization] (active cycles of total cycles, average on all "
           "corresponding lanes, higher is better):"
        << RESET_COLOR << "\n";
    out << "      Lane 0:  " << 100 * (double)totalL0LanesActive / LaneTotal
        << " % ,dst stall: " << 100 * (double)totalL0LanesDstStall / LaneTotal
        << " %, src stall: " << 100 * (double)totalL0LanesSrcStall / LaneTotal << "%      \n";
    out << "      Lane 1:  " << 100 * (double)totalL1LanesActive / LaneTotal
        << " % ,dst stall: " << 100 * (double)totalL1LanesDstStall / LaneTotal
        << " %, src stall: " << 100 * (double)totalL1LanesSrcStall / LaneTotal << "%      \n";
    out << "      Lane LS: " << 100 * (double)totalLSLanesActive / LaneTotal
        << " % ,dst stall: " << 100 * (double)totalLSLanesDstStall / LaneTotal
        << " %, src stall: " << 100 * (double)totalLSLanesSrcStall / LaneTotal << "%      \n";
    out << LIGHT
        << "        A Lane counts as active if its pipeline has a command in any stage which is "
           "not NONE"
        << RESET_COLOR;
    out << "\n\n  [Instructions] Averaged on all Lanes, #Count and Cycles\n";

    out.setFieldAlignment(QTextStream::AlignRight);
    out.setFieldWidth(16);
    out.setRealNumberPrecision(0);
    for (int lane = 0; lane < VPRO_CFG::LANES + 1; lane++) {
        QString prefix;
        switch (lane) {
            case 0:
                prefix = "      L0 ";
                break;
            case 1:
                prefix = "      L1 ";
                break;
            case 2:
                prefix = "      LS ";
                break;
            default:
                prefix = "      [unknown Lane] ";
        }
        auto sum = new double[2]();
        for (auto& it : typeCount[lane]) {
            sum[0] += it.second[0];
            sum[1] += it.second[1];
        }

        for (auto& it : typeCount[lane]) {
            out << prefix + CommandVPRO::getType(it.first) + "       = ";
            if (it.first == CommandVPRO::NONE) {
                out << "                                 " << it.second[1] / parallelUnits
                    << " clock cycles \n";
            } else {
                out << it.second[0] / parallelUnits << " Instructions,  "
                    << it.second[1] / parallelUnits << " clock cycles\n";
            }
        }
        //        out << "%s Sum                = %10.0lf (%10.0lf clock cycles)\n", prefix.c_str(), sum[0]/parallelUnits, sum[1]/parallelUnits);
        if (typeCount[lane].find(CommandVPRO::NONE) != typeCount[lane].end()) {
            sum[0] -= typeCount[lane][CommandVPRO::NONE][0];
            sum[1] -= typeCount[lane][CommandVPRO::NONE][1];
            out << prefix + " Sum (not NONE) = " << sum[0] / parallelUnits << " Instructions,  "
                << sum[1] / parallelUnits << " clock cycles\n";
        }
        out << "      "
               "-----------------------------------------------------------------------------------"
               "-\n";

        delete[] sum;
    }
    out << "\n";
}

void StatisticVpro::print_json(QString& output) {
    QTextStream out(&output);
    out << JSON_OBJ_BEGIN;
    out << JSON_FIELD_FLOAT("clock_period", core->getVPROClockPeriod()) << ",";
    out << JSON_FIELD_INT("total_ticks", total_ticks) << ',';
    for (int lane = 0; lane < VPRO_CFG::LANES + 1; lane++) {
        json_print_lane_stats(out, lane);
        if (lane != VPRO_CFG::LANES) {
            out << ",";
        }
    }
    out << JSON_OBJ_END;
}

void StatisticVpro::json_print_lane_stats(QTextStream& out, int lane) {
    QString lane_identifier;
    uint64_t total_active;
    uint64_t total_src_stall;
    uint64_t total_dst_stall;
    uint64_t any_active;
    switch (lane) {
        case 0:
            lane_identifier = "L0";
            total_active = totalL0LanesActive;
            total_src_stall = totalL0LanesSrcStall;
            total_dst_stall = totalL0LanesDstStall;
            any_active = anyL0LaneActive;
            break;
        case 1:
            lane_identifier = "L1";
            total_active = totalL1LanesActive;
            total_src_stall = totalL1LanesSrcStall;
            total_dst_stall = totalL1LanesDstStall;
            any_active = anyL1LaneActive;
            break;
        case 2:
            lane_identifier = "LS";
            total_active = totalLSLanesActive;
            total_src_stall = totalLSLanesSrcStall;
            total_dst_stall = totalLSLanesDstStall;
            any_active = anyLSLaneActive;
            break;
        default:
            return;
    }

    uint64_t count = VPRO_CFG::UNITS * VPRO_CFG::CLUSTERS;
    uint64_t total_parallel_ticks = total_ticks * count;

    double architecture_utilization =
        (double)(total_active + total_src_stall + total_dst_stall) / (any_active * count);

    double algorithm_utilization = (double)total_active / total_parallel_ticks;
    double src_stall_ratio = (double)total_src_stall / total_parallel_ticks;
    double dst_stall_ratio = (double)total_dst_stall / total_parallel_ticks;

    out << JSON_FIELD_OBJ(lane_identifier);
    out << JSON_FIELD_INT("any_active", any_active) << ",";
    out << JSON_FIELD_INT("src_stall", total_src_stall) << ",";
    out << JSON_FIELD_INT("dst_stall", total_dst_stall) << ",";

    out << JSON_FIELD_FLOAT("architecture_utilization", architecture_utilization) << ",";
    out << JSON_FIELD_FLOAT("algorithm_utilization", algorithm_utilization) << ",";
    out << JSON_FIELD_FLOAT("src_stall_ratio", src_stall_ratio) << ",";
    out << JSON_FIELD_FLOAT("dst_stall_ratio", dst_stall_ratio) << ",";
    json_print_lane_instruction_stats(out, lane);
    out << JSON_OBJ_END;
}

void StatisticVpro::json_print_lane_instruction_stats(QTextStream& out, int lane) {
    uint64_t count = VPRO_CFG::UNITS * VPRO_CFG::CLUSTERS;

    uint64_t total_instructions = 0;
    uint64_t total_instruction_cycles = 0;
    for (auto& it : typeCount[lane]) {
        total_instructions += (uint64_t)it.second[0];
        total_instruction_cycles += (uint64_t)it.second[1];
    }

    out << JSON_FIELD_OBJ("instructions");
    for (auto& it : typeCount[lane]) {
        out << JSON_FIELD_OBJ(CommandVPRO::getType(it.first).trimmed());
        out << JSON_FIELD_INT("instructions", it.second[0] / count) << ",";
        out << JSON_FIELD_INT("cycles", it.second[1] / count);
        out << JSON_OBJ_END << ",";
    }

    out << JSON_FIELD_OBJ("any");
    out << JSON_FIELD_INT("instructions", total_instructions / count) << ",";
    out << JSON_FIELD_INT("cycles", total_instruction_cycles / count);
    out << JSON_OBJ_END << ",";

    out << JSON_FIELD_OBJ("not_none");
    out << JSON_FIELD_INT(
               "instructions", (total_instructions - typeCount[lane][CommandVPRO::NONE][0]) / count)
        << ",";
    out << JSON_FIELD_INT(
        "cycles", (total_instruction_cycles - typeCount[lane][CommandVPRO::NONE][1]) / count);
    out << JSON_OBJ_END;
    out << JSON_OBJ_END;
}

void StatisticVpro::reset() {
    StatisticBase::reset();

    totalL0LanesSrcStall = 0;
    totalL1LanesSrcStall = 0;
    totalLSLanesSrcStall = 0;

    totalL0LanesDstStall = 0;
    totalL1LanesDstStall = 0;
    totalLSLanesDstStall = 0;

    totalL0LanesActive = 0;
    totalL1LanesActive = 0;
    totalLSLanesActive = 0;

    totalL0LanesInActive = 0;
    totalL1LanesInActive = 0;
    totalLSLanesInActive = 0;

    anyL0LaneActive = 0;
    anyL1LaneActive = 0;
    anyLSLaneActive = 0;

    totalL0LanesBlocking = 0;
    totalL1LanesBlocking = 0;
    totalLSLanesBlocking = 0;

    typeCount.clear();
    typeCount = std::vector<std::map<CommandVPRO::TYPE, double[2]>>(VPRO_CFG::LANES + 1);
}
