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

#ifndef CONV2DADD_STATISTICBASE_H
#define CONV2DADD_STATISTICBASE_H

#include <QString>

class ISS;

class StatisticBase {
   public:
    StatisticBase();
    explicit StatisticBase(ISS* core);

    virtual void tick();

    virtual void print(QString& output);
    virtual void print_json(QString& output){};

    virtual void reset();

   protected:
    ISS* core;

    long total_ticks{0};
};

#endif  //CONV2DADD_STATISTICBASE_H
