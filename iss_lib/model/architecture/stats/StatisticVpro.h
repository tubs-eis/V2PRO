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

#ifndef CONV2DADD_STATISTICVPRO_H
#define CONV2DADD_STATISTICVPRO_H

#include <map>
#include "../../commands/CommandVPRO.h"
#include "StatisticBase.h"

#include <QTextStream>

class StatisticVpro : public StatisticBase {
   private:
    unsigned long totalL0LanesSrcStall = 0;
    unsigned long totalL1LanesSrcStall = 0;
    unsigned long totalLSLanesSrcStall = 0;

    unsigned long totalL0LanesDstStall = 0;
    unsigned long totalL1LanesDstStall = 0;
    unsigned long totalLSLanesDstStall = 0;

    unsigned long totalL0LanesActive = 0;
    unsigned long totalL1LanesActive = 0;
    unsigned long totalLSLanesActive = 0;

    unsigned long totalL0LanesInActive = 0;
    unsigned long totalL1LanesInActive = 0;
    unsigned long totalLSLanesInActive = 0;

    unsigned long anyL0LaneActive = 0;
    unsigned long anyL1LaneActive = 0;
    unsigned long anyLSLaneActive = 0;

    // isBlocking
    unsigned long totalL0LanesBlocking = 0;
    unsigned long totalL1LanesBlocking = 0;
    unsigned long totalLSLanesBlocking = 0;  // never?

    std::vector<std::map<CommandVPRO::TYPE, double[2]>> typeCount;  // queue + clockticks

    double getCyclesNotNONE(int lane);
    double getCyclesNONE(int lane);

    void json_print_lane_stats(QTextStream& out, int lane);
    void json_print_lane_instruction_stats(QTextStream& out, int lane);

   public:
    explicit StatisticVpro(ISS* core);

    void tick() override;

    void addExecutedCmdTick(CommandVPRO* cmd, int vector_lane_id);
    void addExecutedCmdQueue(CommandVPRO* cmd, int vector_lane_id);

    void print(QString& output) override;
    void print_json(QString& output) override;

    void reset() override;
};

#endif  //CONV2DADD_STATISTICVPRO_H
