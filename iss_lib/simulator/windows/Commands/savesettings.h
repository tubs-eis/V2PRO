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
#ifndef SAVESETTINGS_H
#define SAVESETTINGS_H

#include <QCheckBox>
#include <QDebug>
#include <QDir>
#include <QLineEdit>
#include <QList>
#include <QSettings>
#include <QVector>
#include <QWidget>

void savesettings(
    QVector<QLineEdit*> offsets, QVector<QLineEdit*> sizes, QList<QCheckBox*> checkedtorestore);

void restoresettings(
    QVector<QLineEdit*> offsets, QVector<QLineEdit*> sizes, QList<QCheckBox*> checkedtorestore);

#endif  // SAVESETTINGS_H
