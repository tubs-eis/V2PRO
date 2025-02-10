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

#ifndef CONV2DADD_STATISTICS_H
#define CONV2DADD_STATISTICS_H

#include "StatisticAxi.h"
#include "StatisticBase.h"
#include "StatisticDcma.h"
#include "StatisticDma.h"
#include "StatisticRisc.h"
#include "StatisticVpro.h"

#include <cinttypes>

// forward definition
class ISS;

class Statistics {
   public:
    enum clock_domains : uint8_t {
        AXI = 0,
        DCMA = 1,
        DMA = 2,
        VPRO = 3,
        RISC = 4,
        end
    };

    explicit Statistics() = default;

    void initialize(ISS* c);

    static Statistics& get() {
        static Statistics instance;
        return instance;
    }

    Statistics(Statistics const&) = delete;
    void operator=(Statistics const&) = delete;

    StatisticDcma* getDCMAStat() {
        return dynamic_cast<StatisticDcma*>(stats[clock_domains::DCMA]);
    }

    StatisticVpro* getVPROStat() {
        return dynamic_cast<StatisticVpro*>(stats[clock_domains::VPRO]);
    }

    StatisticDma* getDMAStat() {
        return dynamic_cast<StatisticDma*>(stats[clock_domains::DMA]);
    }

    void tick(clock_domains clock);

    void print();
    void print(QString& output);
    void print(clock_domains clock, QString& output);

    void print_json();
    void print_json(QString& output);
    void print_json(clock_domains clock, QString& output);

    void dumpToFile(const QString& filename);
    void dumpToJSONFile(const QString& filename);

    void reset();
    void reset(clock_domains clock);

   private:
    StatisticBase* stats[clock_domains::end];
    ISS* core;
};

#endif  //CONV2DADD_STATISTICS_H
