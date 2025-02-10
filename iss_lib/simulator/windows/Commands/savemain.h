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
#ifndef SAVEMAIN_H
#define SAVEMAIN_H

#include <QDebug>
#include <QFile>
#include <QFileDevice>
#include <QFileDialog>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QObject>
#include <QVector>
#include <QWidget>
#pragma once
class CommandWindow;
#include "commandwindow.h"

void savemaintofile(int i,
    CommandWindow* Widget,
    QVector<QLineEdit*> offsets,
    QVector<QLineEdit*> sizes,
    bool* paused);
#endif  // SAVEMAIN_H
