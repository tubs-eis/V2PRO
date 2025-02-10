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
#ifndef SIMINITFUNCTIONS_H
#define SIMINITFUNCTIONS_H
#include <math.h>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QObject>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QTabWidget>
#include <QVector>
#include <QWidget>
#include <tuple>
#include <vector>
#include "../../../simulator/helper/debugHelper.h"

#include "commandwindow.h"
#include "savemain.h"

QVBoxLayout* createlaneprogressbars(
    int cluster_cnt, int unit_cnt, int lane_cnt, QLabel* label, QGroupBox* radiobuttongroup);

QVBoxLayout* createlmdumpbuttons(CommandWindow* Window, int cluster_cnt, int unit_cnt);

QVBoxLayout* createregisterdumpbuttons(
    CommandWindow* Window, int cluster_cnt, int unit_cnt, int lane_cnt);
/**
 * creates lm table which also contains register table
 **/
QWidget* creatememorytable(int unit_cnt,
    int lane_cnt,
    std::vector<std::vector<RfTableView*>>& rfviews,
    std::vector<QLabel*>& v_lbl);

std::tuple<QVector<QLineEdit*>, QVector<QLineEdit*>> createmainmemorydumpbuttons(bool* paused,
    QVector<QLineEdit*> offsets,
    QVector<QLineEdit*> sizes,
    CommandWindow* Window,
    QGridLayout* mainbuttons);

void createdebugoptions(QWidget* parent);

QWidget* createregistertable(
    int lane_cnt, QStackedWidget* dropdown, std::vector<RfTableView*>& rfviews);

#endif  // SIMINITFUNCTIONS_H
