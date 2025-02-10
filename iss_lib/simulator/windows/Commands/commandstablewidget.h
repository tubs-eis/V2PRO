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
#ifndef COMMANDSTABLEWIDGET_H
#define COMMANDSTABLEWIDGET_H

#include <QLayout>
#include <QMainWindow>
#include <QObject>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVariant>
#include <QWidget>
#include "../../../model/commands/CommandVPRO.h"

namespace Ui {
class commandstablewidget;
}

class commandstablewidget : public QMainWindow {
    Q_OBJECT

   public:
    explicit commandstablewidget(
        QWidget* parent = nullptr, std::shared_ptr<CommandVPRO> cmd = nullptr);
    ~commandstablewidget();
    QVector<QVector<QTableWidgetItem*>> tabledata;
   public slots:
    void closewindow();

   private:
    Ui::commandstablewidget* ui;
    void createtable();
};

#endif  // COMMANDSTABLEWIDGET_H
