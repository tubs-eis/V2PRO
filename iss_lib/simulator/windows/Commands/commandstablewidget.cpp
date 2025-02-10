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
#include "commandstablewidget.h"
#include "ui_commandstablewidget.h"

commandstablewidget::commandstablewidget(QWidget* parent, std::shared_ptr<CommandVPRO> cmd)
    : QMainWindow(parent),
      ui(new Ui::
              commandstablewidget) {  //constructor creates matrix with the data of the command, for creating the table ->void createtable
    ui->setupUi(this);
    tabledata.resize(2);
    QTableWidgetItem* classtype = new QTableWidgetItem(tr("Classtype"));
    tabledata[0].append(classtype);
    QTableWidgetItem* classtypevalue = new QTableWidgetItem(cmd->get_class_type());
    tabledata[1].append(classtypevalue);
    QTableWidgetItem* Function = new QTableWidgetItem("Function");
    tabledata[0].append(Function);
    QTableWidgetItem* Functionvalue = new QTableWidgetItem(cmd->get_type());
    tabledata[1].append(Functionvalue);
    QTableWidgetItem* Mask = new QTableWidgetItem("Mask");
    tabledata[0].append(Mask);
    QTableWidgetItem* maskvalue = new QTableWidgetItem(QString::number(cmd->id_mask));
    tabledata[1].append(maskvalue);
    QTableWidgetItem* Destinationadress = new QTableWidgetItem("Destinationadress");
    tabledata[0].append(Destinationadress);
    QTableWidgetItem* Destinationadressvalue = new QTableWidgetItem(
        QString::number(cmd->x * cmd->dst.alpha + cmd->y * cmd->dst.beta + cmd->dst.offset));
    tabledata[1].append(Destinationadressvalue);
    QTableWidgetItem* Sourceadress1 = new QTableWidgetItem("Sourceadress 1");
    tabledata[0].append(Sourceadress1);
    QTableWidgetItem* Sourceadress1value = new QTableWidgetItem;
    if (cmd->src1.sel == SRC_SEL_ADDR) {
        Sourceadress1value->setText(
            QString::number(cmd->x * cmd->src1.alpha + cmd->y * cmd->src1.beta + cmd->src1.offset));
        tabledata[1].append(Sourceadress1value);
    } else if (cmd->src1.sel == SRC_SEL_IMM) {
        Sourceadress1value->setText("Imm: " + QString::number(cmd->src1.getImm()));
        tabledata[1].append(Sourceadress1value);
    } else if (cmd->src1.sel == SRC_SEL_IMM) {
        Sourceadress1value->setText("chain left");
        tabledata[1].append(Sourceadress1value);
    } else if (cmd->src1.sel == SRC_SEL_IMM) {
        Sourceadress1value->setText("chain right");
        tabledata[1].append(Sourceadress1value);
    } else {
        Sourceadress1value->setText("undefined");
    }
    QTableWidgetItem* Sourceadress2 = new QTableWidgetItem("Sourceadress 2");
    tabledata[0].append(Sourceadress2);
    QTableWidgetItem* Sourceadress2value = new QTableWidgetItem;
    if (cmd->src2.sel == SRC_SEL_ADDR) {
        Sourceadress2value->setText(
            QString::number(cmd->x * cmd->src2.alpha + cmd->y * cmd->src2.beta + cmd->src2.offset));
        tabledata[1].append(Sourceadress2value);
    } else if (cmd->src2.sel == SRC_SEL_IMM) {
        Sourceadress2value->setText("Imm: " + QString::number(cmd->src2.getImm()));
        tabledata[1].append(Sourceadress2value);
    } else if (cmd->src2.sel == SRC_SEL_IMM) {
        Sourceadress2value->setText("chain left");
        tabledata[1].append(Sourceadress2value);
    } else if (cmd->src2.sel == SRC_SEL_IMM) {
        Sourceadress2value->setText("chain right");
        tabledata[1].append(Sourceadress2value);
    } else {
        Sourceadress2value->setText("undefined");
        tabledata[1].append(Sourceadress2value);
    }
    if (cmd->blocking) {
        QTableWidgetItem* isblocking = new QTableWidgetItem("Is Blocking");
        tabledata[0].append(isblocking);
        QTableWidgetItem* isblockingvalue = new QTableWidgetItem(cmd->blocking);
        tabledata[1].append(isblockingvalue);
    }
    if (cmd->is_chain) {
        QTableWidgetItem* ischain = new QTableWidgetItem("Is Chain");
        tabledata[0].append(ischain);
        QTableWidgetItem* isblockingvalue = new QTableWidgetItem(cmd->is_chain);
        tabledata[1].append(isblockingvalue);
    }
    if (cmd->flag_update) {
        QTableWidgetItem* flagupdate = new QTableWidgetItem("Flagupdate");
        tabledata[0].append(flagupdate);
        QTableWidgetItem* flagupdatevalue = new QTableWidgetItem(cmd->flag_update);
        tabledata[1].append(flagupdatevalue);
    }
    if (cmd->type == CommandVPRO::WAIT_BUSY) {
        QTableWidgetItem* cluster = new QTableWidgetItem("Cluster");
        QTableWidgetItem* clusternumber = new QTableWidgetItem(QString::number(cmd->cluster));
        tabledata[0].append(cluster);
        tabledata[1].append(clusternumber);
    }
    createtable();
}

commandstablewidget::~commandstablewidget() {
    delete ui;
}

void commandstablewidget::createtable() {
    QTableWidget* commandstable = new QTableWidget;
    this->setCentralWidget(commandstable);
    commandstable->setColumnCount(tabledata.size());
    commandstable->setRowCount(tabledata[0].size());
    for (int i = 0; i < tabledata.size(); i++) {
        for (int u = 0; u < tabledata[i].size(); u++) {
            commandstable->setItem(u, i, tabledata[i][u]);
        }
    }
    this->setFixedSize((2 * commandstable->columnWidth(0) * 2) - 110,
        commandstable->rowHeight(0) * commandstable->rowCount() + 21);
}

void commandstablewidget::closewindow() {
    QMainWindow::close();
}
