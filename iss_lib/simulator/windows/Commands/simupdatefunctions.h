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
#ifndef SIMUPDATEFUNCTIONS_H
#define SIMUPDATEFUNCTIONS_H
#include <QDebug>
#include <QProgressBar>
#include <QRadioButton>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QVector>

int updatelanes(QVector<QRadioButton*> radiobuttons,
    QVector<QProgressBar*> progressbars,
    long clock,
    QVector<long> clockvalues,
    QVector<double> progresstotal);  //update for the last x cycles

int updatelanestotal(QVector<QProgressBar*> progressbars, long clock);  //update for all cycles

#endif  // SIMUPDATEFUNCTIONS_H
