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
#ifndef COMMANDWINDOW_H
#define COMMANDWINDOW_H

#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <QDebug>
#include <QFile>
#include <QFileDevice>
#include <QFileDialog>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QMetaEnum>
#include <QMetaType>
#include <QProgressBar>
#include <QRadioButton>
#include <QSettings>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QVector>
#include <fstream>
#include <tuple>
#include "../../../model/architecture/ArchitectureState.h"
#include "../../../model/commands/CommandDMA.h"
#include "../../../model/commands/CommandSim.h"
#include "../../../model/commands/CommandVPRO.h"
#include "QStackedLayout"
#include "lmtableview.h"
#include "rftableview.h"
#include "savemain.h"
#include "savesettings.h"
#include "siminitfunctions.h"
#include "simupdatefunctions.h"

#include <vpro/vpro_special_register_enums.h>
#include <vector>
#include "dataupdatefunctions.h"
#include "savemain.h"
#include "siminitfunctions.h"
#include "simupdatefunctions.h"

class QComboBox;
class QLCDNumber;
namespace Ui {
class CommandWindow;
}

class CommandWindow : public QMainWindow {
    Q_OBJECT

   public:
    struct Data {
        QVector<QVector<std::shared_ptr<CommandVPRO>>>
            command_queue;  // [cluster][unit] list of cmds in queue
        QVector<std::shared_ptr<CommandVPRO>>
            commands;             // command in all cluster*units*lanes lane's ALU
        QVector<bool> isbusy;     // [cluster]
        QVector<bool> isDMAbusy;  // [cluster]

        long clock;
        Data(){};
    };

    struct VproSpecialRegister {
        std::shared_ptr<ArchitectureState> architecture_state;
        QVector<QVector<QVector<uint64_t*>>> accus{};
    };

    explicit CommandWindow(int clusters, int units, int lanes = 2, QWidget* parent = nullptr);
    ~CommandWindow();

    QVector<QComboBox*> lmboxes;
    uint8_t* main_memory;
    QScrollArea* clusterhscroll = new QScrollArea;
    QVector<QProgressBar*> progressbars;  //vector of progressbars for each lane
    QVector<LmTableView*> lmviews;
    std::vector<std::vector<RfTableView*>> rfviews;
    QVector<long>
        clockvalues;  //saves the clockvalue to the corresponding lanestats saved in the vector alllanestats
    QVector<double> progresstotal;
    QVector<QLineEdit*> offsets;
    QVector<QLineEdit*> sizes;
    QVector<QStackedLayout*> qstacks;
    std::vector<QLabel*> lbl_mode;
    bool sim_exited = false;
    void RemoveLayout(QWidget*);
    void closeEvent(QCloseEvent* event);

   public slots:
    void dataUpdate(CommandWindow::Data data);
    void simUpdate(long clock);
    void simIsPaused();
    void simIsResumed();
    void getmainmemory(uint8_t*);
    void on_pauseButton_clicked();
    void setLocalMemory(int unit, uint8_t*, int c);
    void setRegisterMemory(int lane, uint8_t* ref, int c, int u);
    void recieveguiwaitpress(bool*);
    void simIsFinished();
    void setVproSpecialRegisters(VproSpecialRegister regs);
    void stats_reset(double clock, QString log);
    void resetItemClicked(QModelIndex qmi);
    void updatePerformanceTime(float time);

   private slots:
    void on_cycle100button_clicked();
    void on_cycle1button_clicked();
    void on_cycleButton_clicked();
    void on_exitButton_clicked();
    void on_lanecheck_clicked();
    void on_consoledumpcheck_clicked();
    void on_mainmemorycheck_clicked();
    void on_lmcheck_clicked();
    void on_command3check_clicked();
    void on_tableint_clicked();
    void on_tablehex_clicked();
    void on_tablebit_clicked();

   signals:
    void pauseSim();
    void runSteps(int);
    void dumpRegisterFile(int, int, int);
    void dumpLocalMemory(int, int);
    void laneupdate(int);
    void savemainclicked(int);
    void getSimupdate();
    void closeotherwindows();

   private:
    Ui::CommandWindow* ui;
    Data data;
    int cluster_cnt, unit_cnt, lane_cnt, startclock;
    bool paused;
    LmTableView* lm_view;
    VproSpecialRegister vpro_special_registers;
    QVector<QVector<QVector<QLCDNumber*>>> accu_lcds;

    //Reset Statistik
    QStringList reset_stat_data;
    QStringListModel* reset_stats_model;
    QVector<QString> reset_stats_log;
};

Q_DECLARE_METATYPE(CommandWindow::Data)
#endif  // COMMANDWINDOW_H
