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
#ifndef DATAUPDATEFUNCTIONS_H
#define DATAUPDATEFUNCTIONS_H
#include <QLabel>
#include <QLayout>
#include <QObject>
#include <QPushButton>
#include <QScrollArea>
#include <QWidget>
#pragma once
#include "commandstablewidget.h"
#include "commandwindow.h"

QWidget* createunitwidget(CommandWindow* mainwindow,
    int unitqueue,
    int u,
    int lane_cnt,
    QVector<std::shared_ptr<CommandVPRO>> commands,
    QVector<QVector<std::shared_ptr<CommandVPRO>>> command_queue,
    bool paused,
    int* cmd,
    QVector<bool> isLoopparsing,
    QVector<bool> isLopppush,
    int c);

void createtablewindow(CommandWindow* mainwindow, std::shared_ptr<CommandVPRO> cmd);

QScrollArea* createcommandqueue(QVector<QVector<std::shared_ptr<CommandVPRO>>> command_queue,
    CommandWindow* mainwindow,
    int unitqueue);

QGridLayout* createlaneslayout(int lane_cnt,
    int* cmd,
    QVector<std::shared_ptr<CommandVPRO>> commands,
    CommandWindow* mainwindow);

void printcommandqueue(QVector<std::shared_ptr<CommandVPRO>> command_queue, int c, int u);

QWidget* createcommandcluster(int cluster_cnt,
    QVector<bool> isDMAbusy,
    QVector<bool>
        isbusy);  //creates a widget for each cluster, in which are later the unitwidgets of the dataupdate are added
#endif  // DATAUPDATEFUNCTIONS_H
