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
#include "commandwindow.h"
#include <math.h>
#include <QCloseEvent>
#include <QComboBox>
#include <fstream>
#include <iostream>
#include "../../../core_wrapper.h"
#include "../../../simulator/helper/debugHelper.h"
#include "lmtableview.h"
#include "ui_commandwindow.h"

/**
 * Actions performed shortly after closing the window
 **/
void CommandWindow::closeEvent(QCloseEvent* event) {
    if (!sim_exited) {
        core_->quitSim();
        core_->sim_stop();
        sim_exited = true;
    }
    event->accept();
}

CommandWindow::CommandWindow(int clusters, int units, int lanes, QWidget* parent)
    : QMainWindow(parent), ui(new Ui::CommandWindow) {
    ui->setupUi(this);
    ui->lbl_hw->setText(QString("Hardware:\nCLUSTER %1\nUNITS   %2\nLANES   %3\n")
                            .arg(clusters)
                            .arg(units)
                            .arg(lanes));
    //QLocale german(QLocale::German);
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));

    paused = true;
    cluster_cnt = clusters;
    unit_cnt = units;
    lane_cnt = lanes;

    progresstotal.resize(cluster_cnt * unit_cnt * lane_cnt);

    RemoveLayout(ui->rfdump);
    RemoveLayout(ui->lmdump);
    RemoveLayout(ui->mainmembuttongroup);
    RemoveLayout(ui->Lanestab);
    RemoveLayout(ui->lmtabs);

    clusterhscroll->setWidgetResizable(true);
    ui->tabwidget->insertTab(
        4, clusterhscroll, "Commands (Lane+Unit's Q)");  //creates tab with commands

    ui->Lanestab->setLayout(createlaneprogressbars(cluster_cnt,
        unit_cnt,
        lane_cnt,
        ui->Labellastcycles,
        ui->radiobuttongroup));  //setlayout for lanestab creating the progressbars for each lane
    QList<QProgressBar*> progressbarslist =
        ui->Lanestab->findChildren<QProgressBar*>();  //finds progressbars of lanes
    progressbars = progressbarslist.toVector();

    simUpdate(0);

    QList<QRadioButton*> radiobuttons = ui->radiobuttongroup->findChildren<QRadioButton*>();
    for (
        int i = 0; i < radiobuttons.size();
        i++) {  //connects the radiobuttons of laneupdate with the signal getsimupdate so data is get updated instantly when another button is clicked
        connect(radiobuttons[i], &QRadioButton::clicked, [this]() {
            this->emit getSimupdate();
        });
    }

    for (int c = 0; c < cluster_cnt; c++) {  //creates the tables for local memory and registerfiles
        ui->lmtabs->addTab(creatememorytable(unit_cnt, lane_cnt, rfviews, lbl_mode),
            "Cluster " + QString::number(c));
    }
    QList<LmTableView*> lmlist =
        ui->lmtabs->findChildren<LmTableView*>();  //finds the localmemory tables
    QList<QComboBox*> lmboxeslist = ui->lmtabs->findChildren<QComboBox*>();
    lmboxes = lmboxeslist.toVector();
    lmviews = lmlist.toVector();

    for (
        int i = 0; i < log(float(DebugOptions::end)) / log(2.);
        i++) {  //loops trough debugoption creating corresponding checkboxes and connection to the debugoptions enum
        auto option = DebugOptions(1 << i);
        QString option_text = debugToText(option);
        if (option_text != "end") {
            QCheckBox* optionbox = new QCheckBox;
            optionbox->setText(option_text);
            connect(optionbox,
                QOverload<bool>::of(&QCheckBox::clicked),
                [optionbox, option](bool checked) {
                    if (checked) {
                        debug = debug | option;
                    } else if (!checked) {
                        debug = debug & ~option;
                    }
                });
            ui->scrollAreaWidgetContents_3->layout()->addWidget(optionbox);
        }
    }
    QPushButton* resetDebug = new QPushButton("Reset All Debug", this);
    ui->scrollAreaWidgetContents_3->layout()->addWidget(resetDebug);
    connect(resetDebug, &QPushButton::clicked, [&]() {
        for (QCheckBox* c_box : ui->scrollAreaWidgetContents_3->findChildren<QCheckBox*>()) {
            c_box->setCheckState(Qt::Unchecked);
        }
    });

    QGridLayout* mainbuttons = new QGridLayout;
    bool* pausedptr = &paused;  //creates pointer to the paused variable
    std::tie(offsets, sizes) = createmainmemorydumpbuttons(pausedptr,
        offsets,
        sizes,
        this,
        mainbuttons);  //creates mainmemorydumpbuttons and links them to offsets and sizes vectors
    ui->mainmembuttongroup->setLayout(mainbuttons);

    ui->rfdump->setLayout(createregisterdumpbuttons(this, cluster_cnt, unit_cnt, lane_cnt));
    ui->lmdump->setLayout(createlmdumpbuttons(this, cluster_cnt, unit_cnt));

    QList<QCheckBox*> checkboxes = ui->settingstab->findChildren<
        QCheckBox*>();  //findes checkboxes in settingstab to restoresettings after init
    restoresettings(offsets, sizes, checkboxes);
    QList<QCheckBox*> tabcheckboxes = ui->settingsbox->findChildren<
        QCheckBox*>();  //finds checkboxes in tabsettingsbox to enable tabs by looping trough the list
    for (int i = 0; i < tabcheckboxes.size(); i++) {
        ui->tabwidget->setTabEnabled(i, tabcheckboxes[i]->isChecked());
    }
    for (auto debugbox :
        ui->scrollAreaWidgetContents_3->findChildren<
            QCheckBox*>()) {  //loops trough all checked debugoptionboxes one time to emit signal if checked to activate this option
        if (debugbox->isChecked()) {
            emit debugbox->clicked(true);
        }
    }
    on_tablehex_clicked();

    auto accus = new QVBoxLayout(ui->accu_widget);
    accus->setContentsMargins(0, 0, 0, 0);
    accus->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
    accus->addWidget(new QLabel("Accumulation Register in Processing Lanes:"));
    accu_lcds.clear();
    for (int c = 0; c < cluster_cnt; c++) {
        accu_lcds.append(QVector<QVector<QLCDNumber*>>());
        for (int u = 0; u < unit_cnt; u++) {
            accu_lcds.last().append(QVector<QLCDNumber*>());
            for (int l = 0; l < lane_cnt; l++) {
                auto widget = new QWidget(ui->accu_widget);
                auto hlayout = new QHBoxLayout(widget);
                hlayout->setContentsMargins(0, 0, 0, 0);
                auto lcd = new QLCDNumber(20, widget);
                accu_lcds.last().last().append(lcd);
                hlayout->addWidget(lcd);
                hlayout->addWidget(
                    new QLabel("Cluster " + QString::number(c) + ", Unit " + QString::number(u) +
                                   ", Lane " + QString::number(l),
                        widget));
                widget->setLayout(hlayout);
                accus->addWidget(widget);
            }
            if (u + 1 < unit_cnt || c + 1 < cluster_cnt) {
                auto line2 = new QFrame();
                line2->setFrameShape(QFrame::HLine);
                line2->setFrameShadow(QFrame::Sunken);
                accus->addWidget(line2);
            }
        }
    }
    ui->accu_widget->setLayout(accus);

    reset_stats_model = new QStringListModel(this);
    ui->reset_log->setModel(reset_stats_model);
    ui->reset_log->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(ui->reset_log,
        SIGNAL(clicked(const QModelIndex)),
        this,
        SLOT(resetItemClicked(QModelIndex)));
}

Q_DECLARE_METATYPE(CommandBase*);

void CommandWindow::dataUpdate(Data data) {
    if (ui->command3check->isChecked()) {
        qDebug() << "Command List Tab disabled. Is on TODO!";

        //    overallDMA = overallDMA / (float) cluster_count;
        //    for (auto &stat : overalllane) {
        //        stat = stat / (float) (cluster_count * vector_unit_count);
        //    }
        //    overallUnit = overallUnit / (cluster_count * vector_unit_count);

        //        if (data.commands.size() > 0) {
        //            QString text;
        //            int cmdcount = 0;
        //            int *cmd = &cmdcount;
        //            int unitqueue = 0;
        //            clusterhscroll->setWidget(createcommandcluster(cluster_cnt, data.isDMAbusy,
        //                                                           data.isbusy)); //creates commandcluster in commandtab with label scrollarea etc
        //            QList<QScrollArea *> clusterscrollslist = ui->tabwidget->widget(
        //                    4)->findChildren<QScrollArea *>();//finds scrollareas of each cluster in the commandtabwidget and adds them to a list
        //            QVector<QScrollArea *> clusterscrollareas = clusterscrollslist.toVector();
        //            for (int c = 0; c < cluster_cnt; c++) {
        //                QWidget *clusterscrollwidget = new QWidget;
        //                QVBoxLayout *clustervlayout = new QVBoxLayout;
        //
        //                for (int u = 0; u < unit_cnt; u++) {
        //                    clustervlayout->addWidget(
        //                            createunitwidget(this, unitqueue, u, this->lane_cnt, data.commands, data.command_queue,
        //                                             paused, cmd, data.isLoppparsing, data.isLooppush,
        //                                             c)); //creates units for each cluster with command and commandqueue
        //                    unitqueue++;
        //                    if (unitqueue > data.command_queue.size()) {
        //                        break;
        //                    }
        //                }
        //                clusterscrollwidget->setLayout(clustervlayout);
        //                clusterscrollareas[c]->setWidget(clusterscrollwidget);
        //            }
        //        }
    }
    QString statistic_text;
    //data.simStat.print(data.clock, "\t", &statistic_text);

    Statistics::get().print(statistic_text);

    ui->statistic_textbrowser->setText(statistic_text);

    auto& architecture_state = vpro_special_registers.architecture_state;
    ui->conf_reg_acc_init->setCurrentIndex(architecture_state->MAC_ACCU_INIT_SOURCE);

    ui->conf_reg_acc_shift_mach->display(int(architecture_state->ACCU_MAC_HIGH_BIT_SHIFT));
    ui->conf_reg_acc_shift_mulh->display(int(architecture_state->ACCU_MUL_HIGH_BIT_SHIFT));
    ui->conf_reg_dma_pad_bottom->display(int(architecture_state->dma_pad_bottom));
    ui->conf_reg_dma_pad_left->display(int(architecture_state->dma_pad_left));
    ui->conf_reg_dma_pad_right->display(int(architecture_state->dma_pad_right));
    ui->conf_reg_dma_pad_top->display(int(architecture_state->dma_pad_top));
    ui->conf_reg_dma_pad_value->display(int(architecture_state->dma_pad_value));
    ui->conf_reg_mask_cluster->display(int(architecture_state->cluster_mask_global));
    ui->conf_reg_mask_unit->display(int(architecture_state->unit_mask_global));

    for (int c = 0; c < cluster_cnt; c++) {
        for (int u = 0; u < unit_cnt; u++) {
            for (int l = 0; l < lane_cnt; l++) {
                if (accu_lcds.length() > c) {
                    if (accu_lcds[c].length() > u) {
                        if (accu_lcds[c][u].length() > l) {
                            if (vpro_special_registers.accus.length() > c &&
                                vpro_special_registers.accus[c].length() > u &&
                                vpro_special_registers.accus[c][u].length() > l) {
                                accu_lcds[c][u][l]->display(QString::number(
                                    uint32_t(*vpro_special_registers.accus[c][u][l])));
                            } else {
                                qDebug() << "[Fail] Accu has not got enaugh references to display "
                                            "regsiters content!";
                                qDebug()
                                    << "[Fail] Accu C: " << vpro_special_registers.accus.length()
                                    << ", U: " << vpro_special_registers.accus.last().length()
                                    << ", L: "
                                    << vpro_special_registers.accus.last().last().length();
                                accu_lcds[c][u][l]->display(QString("dead"));
                            }
                        }
                    }
                }
            }
        }
    }
}

void CommandWindow::stats_reset(double clock, QString log) {
    reset_stat_data << "Statistic Reset, Time: " + QString::number(clock) + "ns";
    reset_stats_model->setStringList(reset_stat_data);
    reset_stats_log.append(log);
}

void CommandWindow::simUpdate(long clock) {
    if (ui->lmtabs->count() > 0 && ui->tabwidget->currentWidget() == ui->tab_localMemory) {
        LmTableView* current =
            ui->lmtabs->currentWidget()
                ->findChild<LmTableView*>();  //finds lmtableview in the current activated tab
        current->model()->dataChanged(QModelIndex(), QModelIndex());  //updates data of lm tableview
        QStackedWidget* stack =
            ui->lmtabs->currentWidget()
                ->findChild<QStackedWidget*>();  //finds Qstackedwidget in the current activated tab
        RfTableView* rf =
            stack->currentWidget()
                ->findChild<
                    RfTableView*>();  //finds rftableview on the current activated stack of the qstackedwidget
        rf->model()->dataChanged(QModelIndex(), QModelIndex());  //updates data of rf tableview
    }
    ui->cycle_label->setText("Time: " + QLocale().toString(qlonglong(clock)) + " ns");

    //    clockvalues.append(clock);//stores clock in a vector
    if (ui->lanecheck
            ->isChecked()) {  //shows percentual usage of lane, relation of NONE commands to total commands
        qDebug() << "Update of Lane Stats Tab is on TODO-List!";
    }
}

void CommandWindow::RemoveLayout(QWidget* widget) {
    QLayout* layout = widget->layout();
    if (layout != nullptr) {
        QLayoutItem* item;
        while ((item = layout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete layout;
    }
}

void CommandWindow::simIsPaused() {
    ui->sim_status_label->setText("Paused");
    ui->pauseButton->setText("Resume Simulation");
}

void CommandWindow::simIsResumed() {
    ui->sim_status_label->setText("Running");
    ui->pauseButton->setText("Pause Simulation");
}

CommandWindow::~CommandWindow() {
    delete ui;
}

void CommandWindow::on_cycle100button_clicked() {
    emit runSteps(100);
}

void CommandWindow::on_cycle1button_clicked() {
    emit runSteps(1);
}

void CommandWindow::on_pauseButton_clicked() {
    if (!paused) {
        paused = true;
    } else {
        paused = false;
    }
    emit pauseSim();
}

void CommandWindow::on_cycleButton_clicked() {
    emit runSteps(ui->cycleInput->value());
}

void CommandWindow::getmainmemory(uint8_t* adress) {
    main_memory = adress;
}

void CommandWindow::setLocalMemory(int unit, uint8_t* ref, int c) {
    lmviews[cluster_cnt - c - 1]->setLocalMemory(unit, ref);
    lmviews[cluster_cnt - c - 1]->selectLocalMemory(0);
}

void CommandWindow::setRegisterMemory(int lane, uint8_t* ref, int c, int u) {
    rfviews[c][u]->setRegisterFile(lane, ref);
    rfviews[c][u]->selectRegisterFile(0);
}

void CommandWindow::on_exitButton_clicked() {
    QList<QCheckBox*> checkedtorestore = ui->settingstab->findChildren<QCheckBox*>();
    savesettings(offsets, sizes, checkedtorestore);
    emit exit(0);
}

void CommandWindow::on_lanecheck_clicked() {
    ui->tabwidget->setTabEnabled(0, ui->lanecheck->isChecked());
}

void CommandWindow::on_consoledumpcheck_clicked() {
    ui->tabwidget->setTabEnabled(1, ui->consoledumpcheck->isChecked());
}

void CommandWindow::on_mainmemorycheck_clicked() {
    ui->tabwidget->setTabEnabled(2, ui->mainmemorycheck->isChecked());
}

void CommandWindow::on_lmcheck_clicked() {
    ui->tabwidget->setTabEnabled(3, ui->lmcheck->isChecked());
}

void CommandWindow::on_command3check_clicked() {
    ui->tabwidget->setTabEnabled(4, ui->command3check->isChecked());
}

void CommandWindow::on_tableint_clicked() {
    for (QLabel* l : lbl_mode) {
        l->setText("INT Mode");
    }
    for (int i = 0; i < lmviews.size();
         i++) {  //loop which changes lmviewdata to int and sets columnwidth
        lmviews[i]->modelData->basevalue = 10;
        for (int j = 0; j < lmviews[i]->modelData->COLS; j++) {
            lmviews[i]->setColumnWidth(j, 50);
        }
    }
    for (int j = 0; j < rfviews.size();
         j++) {  //loop which changes rfviewdata to int and sets columnwidth
        for (int k = 0; k < rfviews[j].size(); k++) {
            rfviews[j][k]->modelData->basevalue = 10;
            rfviews[j][k]->resizeColumnsToContents();
            for (int t = 0; t < rfviews[j][k]->modelData->COLS; t++) {
                rfviews[j][k]->setColumnWidth(t, 50);
            }
        }
    }
}

void CommandWindow::on_tablehex_clicked() {
    for (QLabel* l : lbl_mode) {
        l->setText("HEX Mode");
    }
    for (int i = 0; i < lmviews.size();
         i++) {  //loop which changes lmviewdata to hex and sets columnwidth
        lmviews[i]->modelData->basevalue = 16;
        lmviews[i]->resizeColumnsToContents();
        for (int j = 0; j < lmviews[i]->modelData->COLS; j++) {
            lmviews[i]->setColumnWidth(j, 75);
        }
    }
    for (int j = 0; j < rfviews.size();
         j++) {  //loop which changes rfviewdata to hex and sets columnwidth
        for (int k = 0; k < rfviews[j].size(); k++) {
            rfviews[j][k]->modelData->basevalue = 16;
            rfviews[j][k]->resizeColumnsToContents();
            for (int t = 0; t < rfviews[j][k]->modelData->COLS; t++) {
                rfviews[j][k]->setColumnWidth(t, 75);
            }
        }
    }
}

void CommandWindow::on_tablebit_clicked() {
    for (QLabel* l : lbl_mode) {
        l->setText("BIT Mode");
    }
    for (int i = 0; i < lmviews.size();
         i++) {  //loop which changes lmviewdata to bit and sets columnwidth
        lmviews[i]->modelData->basevalue = 2;
        lmviews[i]->resizeColumnsToContents();
        for (int j = 0; j < lmviews[i]->modelData->COLS; j++) {
            lmviews[i]->setColumnWidth(j, 100);
        }
    }
    for (int j = 0; j < rfviews.size();
         j++) {  //loop which changes rfviewdata to bit and sets columnwidth
        for (int k = 0; k < rfviews[j].size(); k++) {
            rfviews[j][k]->modelData->basevalue = 2;
            rfviews[j][k]->resizeColumnsToContents();
            for (int t = 0; t < rfviews[j][k]->modelData->COLS; t++) {
                rfviews[j][k]->setColumnWidth(t, 100);
            }
        }
    }
}

void CommandWindow::recieveguiwaitpress(bool* is_pressed) {
    emit pauseSim();
    //QMessageBox msgBox;
    //msgBox.setText("Sim_wait_step: press resume Simulation to continue");
    //msgBox.exec();
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(ui->pauseButton, &QPushButton::clicked, [is_pressed, conn]() {
        *is_pressed = true;
        disconnect(*conn);
    });  //connect pausebutton to is_pressed variable until clicked once for resuming
}

/**
 * Shall lock the resume and pause btns, as well the + btn, update Label
 **/
void CommandWindow::simIsFinished() {
    ui->cycle1button->setEnabled(false);
    ui->cycleButton->setEnabled(false);
    ui->cycle100button->setEnabled(false);
    ui->pauseButton->setEnabled(false);
    ui->sim_status_label->setText("Finished");
    sim_exited = true;
}

void CommandWindow::setVproSpecialRegisters(VproSpecialRegister regs) {
    vpro_special_registers = regs;
}

#include <stdio.h>
#include <time.h>

const std::string currentDateTime() {
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
    return buf;
}

void saveLogToFile(std::string log) {
    QDir dir("stats");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    std::string filename = "stats/" + currentDateTime() + ".log";
    std::ofstream f(filename);
    f << log;
    f.close();
    std::string info = "File was saved as " + filename + "\necho " + std::string(10, '=');
    std::string exe =
        "gnome-terminal -- /bin/bash -c 'echo " + info + "; cat " + filename + "; read'";
    int retCode = system(exe.c_str());
}

void CommandWindow::resetItemClicked(QModelIndex qmi) {
    static int lastSelectedIndex = -1;
    if (lastSelectedIndex == qmi.row()) {
        auto log = reset_stats_log.at(qmi.row());
        saveLogToFile(log.toStdString());
        lastSelectedIndex = -1;
    } else {
        lastSelectedIndex = qmi.row();
    }
}

void CommandWindow::updatePerformanceTime(float time) {
    ui->lbl_simtime->setText(QString("ns / sec: %1").arg(time));
}
