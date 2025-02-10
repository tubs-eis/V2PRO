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

#include "Statistics.h"
#include <simulator/helper/debugHelper.h>
#include <QDataStream>
#include <QFile>
#include <QTextStream>
#include "../../../simulator/ISS.h"

#include "JSONHelpers.h"

void Statistics::initialize(ISS* c) {
    core = c;
    stats[AXI] = new StatisticAxi(c);
    stats[DCMA] = new StatisticDcma(c);
    stats[DMA] = new StatisticDma(c);
    stats[VPRO] = new StatisticVpro(c);
    stats[RISC] = new StatisticRisc(c);
}

/**
 * process the tick in given clock domain
 * @param clock domain
 */
void Statistics::tick(clock_domains clock) {
    switch (clock) {
        case AXI:
            dynamic_cast<StatisticAxi*>(stats[clock])->tick();
            break;
        case DCMA:
            dynamic_cast<StatisticDcma*>(stats[clock])->tick();
            break;
        case DMA:
            dynamic_cast<StatisticDma*>(stats[clock])->tick();
            break;
        case VPRO:
            dynamic_cast<StatisticVpro*>(stats[clock])->tick();
            break;
        case RISC:
            dynamic_cast<StatisticRisc*>(stats[clock])->tick();
            break;
        default:
            stats[clock]->tick();
    }
}

void Statistics::print() {
    QString output;
    print(output);
    printf("%s", output.toStdString().c_str());
}

void Statistics::print(QString& output) {
    for (uint8_t i = 0; i < clock_domains::end; ++i) {
        print(static_cast<clock_domains>(i), output);
    }
}

inline void Statistics::print(clock_domains clock, QString& output) {
    switch (clock) {
        case AXI:
            dynamic_cast<StatisticAxi*>(stats[clock])->print(output);
            break;
        case DCMA:
            dynamic_cast<StatisticDcma*>(stats[clock])->print(output);
            break;
        case DMA:
            dynamic_cast<StatisticDma*>(stats[clock])->print(output);
            break;
        case VPRO:
            dynamic_cast<StatisticVpro*>(stats[clock])->print(output);
            break;
        case RISC:
            dynamic_cast<StatisticRisc*>(stats[clock])->print(output);
            break;
        default:
            stats[clock]->print(output);
    }
}

void Statistics::print_json() {
    QString output;
    print_json(output);
    printf("%s", output.toStdString().c_str());
}

void Statistics::print_json(QString& output) {
    QTextStream out(&output);
    out << JSON_OBJ_BEGIN;
    for (uint8_t i = 0; i < clock_domains::end; ++i) {
        print_json(static_cast<clock_domains>(i), output);
        if (i != clock_domains::end - 1) {
            out << ",";
        }
    }
    out << JSON_OBJ_END;
}

void Statistics::print_json(clock_domains clock, QString& output) {
    QTextStream out(&output);
    switch (clock) {
        case AXI:
            out << JSON_ID("axi");
            dynamic_cast<StatisticAxi*>(stats[clock])->print_json(output);
            break;
        case DCMA:
            out << JSON_ID("dcma");
            dynamic_cast<StatisticDcma*>(stats[clock])->print_json(output);
            break;
        case DMA:
            out << JSON_ID("dma");
            dynamic_cast<StatisticDma*>(stats[clock])->print_json(output);
            break;
        case VPRO:
            out << JSON_ID("vpro");
            dynamic_cast<StatisticVpro*>(stats[clock])->print_json(output);
            break;
        case RISC:
            out << JSON_ID("risc");
            dynamic_cast<StatisticRisc*>(stats[clock])->print_json(output);
            break;
        default:
            stats[clock]->print(output);
    }
}

void Statistics::dumpToFile(const QString& filename) {
    QString stat;
    print(stat);
    QFile outFile(filename);
    if (outFile.open(QIODevice::Append)) {
        QTextStream out(&outFile);
        out << stat;
        outFile.close();
    } else {
        printf_warning(
            "[Statistics] Output File for Statistic Dump could not be opened for append (check if "
            "path exists! [%s]). -> Skipped!\n",
            filename.toStdString().c_str());
    }
}

void Statistics::dumpToJSONFile(const QString& filename) {
    QString stat;
    print_json(stat);
    QFile outFile(filename);
    if (outFile.open(QIODevice::WriteOnly)) {
        QTextStream out(&outFile);
        out << stat;
        outFile.close();
    } else {
        printf_warning(
            "[Statistics] Output File for JSON Statistic Dump could not be opened (check if path "
            "exists! [%s]). -> Skipped!\n",
            filename.toStdString().c_str());
    }
}

void Statistics::reset() {
    for (uint8_t i = 0; i < clock_domains::end; ++i) {
        reset(static_cast<clock_domains>(i));
    }
    printf_info("[Statistic] reset every statistic counter to 0 @Time %.0lf ns\n", core->getTime());
}

void Statistics::reset(Statistics::clock_domains clock) {
    switch (clock) {
        case AXI:
            dynamic_cast<StatisticAxi*>(stats[clock])->reset();
            break;
        case DCMA:
            dynamic_cast<StatisticDcma*>(stats[clock])->reset();
            break;
        case DMA:
            dynamic_cast<StatisticDma*>(stats[clock])->reset();
            break;
        case VPRO:
            dynamic_cast<StatisticVpro*>(stats[clock])->reset();
            break;
        case RISC:
            dynamic_cast<StatisticRisc*>(stats[clock])->reset();
            break;
        default:
            stats[clock]->reset();
    }
}
