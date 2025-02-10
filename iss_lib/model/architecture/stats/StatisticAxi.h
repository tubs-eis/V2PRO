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

#ifndef CONV2DADD_STATISTICaxi_H
#define CONV2DADD_STATISTICaxi_H

#include "StatisticBase.h"

class StatisticAxi : public StatisticBase {
   public:
    explicit StatisticAxi(ISS* core);

    void tick() override;

    void print(QString& output) override;
    void print_json(QString& output) override;

    void reset() override;
};

#endif  //CONV2DADD_STATISTICaxi_H
