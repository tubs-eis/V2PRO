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
#include "dataupdatefunctions.h"

QWidget* createunitwidget(CommandWindow* mainwindow,
    int unitqueue,
    int u,
    int lane_cnt,
    QVector<std::shared_ptr<CommandVPRO>> commands,
    QVector<QVector<std::shared_ptr<CommandVPRO>>> command_queue,
    bool paused,
    int* command,
    QVector<bool> isLoopparsing,
    QVector<bool> isLopppush,
    int c) {
    QVBoxLayout* unitlayout = new QVBoxLayout;
    QFrame* unit = new QFrame;
    unit->setFrameStyle(QFrame::Panel | QFrame::Plain);
    QLabel* unitlabel = new QLabel;
    QLabel* queuelabel = new QLabel;
    QPushButton* printbutton = new QPushButton;
    printbutton->setText("Print Commandqueue");
    auto command_queueforprint = command_queue[unitqueue];
    QObject::connect(printbutton, &QPushButton::clicked, [command_queueforprint, c, u] {
        printcommandqueue(command_queueforprint, c, u);
    });
    queuelabel->setText(QString::number(command_queue[unitqueue].size()) + " Commands in Queue");
    QString text = "Unit " + QString::number(u);
    if (isLoopparsing[unitqueue] == true) {
        text.append(" isLoopparsing");
    }
    if (isLopppush[unitqueue] == true) {
        text.append(" isLooppush");
    }
    unitlabel->setText(text);
    unitlayout->addWidget(unitlabel);
    unitlayout->addWidget(queuelabel);
    if (paused) {
        unitlayout->addWidget(createcommandqueue(command_queue, mainwindow, unitqueue));
    }
    unitlayout->addLayout(createlaneslayout(lane_cnt, command, commands, mainwindow));
    unitlayout->addWidget(printbutton);
    unitlayout->setSpacing(2);
    unitlayout->setContentsMargins(2, 2, 2, 2);
    unit->setLayout(unitlayout);
    return unit;
}

void createtablewindow(CommandWindow* mainwindow, std::shared_ptr<CommandVPRO> command) {
    emit mainwindow->closeotherwindows();
    commandstablewidget* window = new commandstablewidget(mainwindow, command);
    window->move(QCursor::pos());
    window->show();
    QObject::connect(mainwindow, SIGNAL(closeotherwindows()), window, SLOT(closewindow()));
}

QScrollArea* createcommandqueue(QVector<QVector<std::shared_ptr<CommandVPRO>>> command_queue,
    CommandWindow* mainwindow,
    int unitqueue) {
    QScrollArea* queuecommands = new QScrollArea;
    queuecommands->setWidgetResizable(true);
    QVBoxLayout* vprolayout = new QVBoxLayout;
    int i = 0;
    for (auto vprocommand : command_queue[unitqueue]) {
        QPushButton* item = new QPushButton;
        QFont font = item->font();
        font.setPointSize(6);
        item->setFont(font);
        item->setFixedHeight(10);
        item->setText(vprocommand->get_type() + " " + QString::number(i));
        QObject::connect(item, &QPushButton::clicked, [mainwindow, vprocommand]() {
            createtablewindow(mainwindow, vprocommand);
        });
        vprolayout->addWidget(item);
        i++;
        if (i > 5 || i > command_queue[unitqueue].size()) {
            break;
        }
    }
    vprolayout->setSpacing(0);
    vprolayout->setContentsMargins(0, 0, 0, 0);
    QWidget* inner = new QWidget;
    inner->setLayout(vprolayout);
    queuecommands->setWidget(inner);
    return (queuecommands);
}

QGridLayout* createlaneslayout(int lane_cnt,
    int* command,
    QVector<std::shared_ptr<CommandVPRO>> commands,
    CommandWindow* mainwindow) {
    QGridLayout* laneslayout = new QGridLayout;
    for (int l = 0; l < lane_cnt; l++) {
        QLabel* lanelabel = new QLabel;
        lanelabel->setText("Lane " + QString::number(l));
        laneslayout->addWidget(lanelabel, 0, l);
        QPushButton* item = new QPushButton;
        item->setFixedHeight(20);
        item->setText(commands[*command]->get_type());
        std::shared_ptr<CommandVPRO> cmd = commands[*command];
        QObject::connect(item, &QPushButton::clicked, [mainwindow, cmd]() {
            createtablewindow(mainwindow, cmd);
        });
        laneslayout->addWidget(item, 1, l);
        command++;
        if (*command > commands.size()) {
            break;
        }
    }
    return laneslayout;
}

QWidget* createcommandcluster(int cluster_cnt, QVector<bool> isDMAbusy, QVector<bool> isbusy) {
    QHBoxLayout* commandtablayout = new QHBoxLayout;
    QScrollArea* hscrollarea = new QScrollArea;
    for (int c = 0; c < cluster_cnt; c++) {
        QLabel* clusterlabel = new QLabel;
        QString text = "Cluster " + QString::number(c);
        if (isbusy[c]) {
            text.append(" isbusy");
        }
        if (isDMAbusy[c]) {
            text.append(" isDMAbusy");
        }
        clusterlabel->setText(text);
        QVBoxLayout* clusterlayout = new QVBoxLayout;
        clusterlayout->addWidget(clusterlabel);
        QFrame* cluster = new QFrame;
        cluster->setFrameStyle(QFrame::Panel | QFrame::Raised);
        clusterlayout->setSpacing(2);
        clusterlayout->setContentsMargins(0, 0, 0, 0);
        QScrollArea* clusterscrollarea = new QScrollArea;
        clusterscrollarea->setWidgetResizable(true);
        clusterscrollarea->setContentsMargins(0, 0, 0, 0);
        clusterlayout->addWidget(clusterscrollarea);
        cluster->setLayout(clusterlayout);
        commandtablayout->addWidget(cluster);
    }
    QWidget* hscrollwidget = new QWidget;
    hscrollwidget->setLayout(commandtablayout);
    return hscrollwidget;
}

void printcommandqueue(QVector<std::shared_ptr<CommandVPRO>> command_queue, int c, int u) {
    qDebug() << "\n";
    qDebug().noquote() << "Printing Commandqueue for Unit " + QString::number(u) + " in Cluster " +
                              QString::number(c);
    int i = 0;
    for (auto command : command_queue) {
        qDebug().noquote() << QString::number(i) + " " + command->get_type();
        QString Src1;
        QString Src2;
        if (command->src1.sel == SRC_SEL_ADDR) {
            Src1 = (QString::number(command->x * command->src1.alpha +
                                    command->y * command->src1.beta + command->src1.offset));
        } else if (command->src1.sel == SRC_SEL_IMM) {
            Src1 = ("Imm " + QString::number(command->src1.getImm()));
        } else if (command->src1.sel == SRC_SEL_IMM) {
            Src1 = ("chain left");
        } else if (command->src1.sel == SRC_SEL_IMM) {
            Src1 = ("chain right");
        } else {
            Src1 = ("undefined");
        }
        if (command->src2.sel == SRC_SEL_ADDR) {
            Src2 = (QString::number(command->x * command->src2.alpha +
                                    command->y * command->src2.beta + command->src2.offset));
        } else if (command->src2.sel == SRC_SEL_IMM) {
            Src2 = ("Imm " + QString::number(command->src2.getImm()));
        } else if (command->src2.sel == SRC_SEL_IMM) {
            Src2 = ("chain left");
        } else if (command->src2.sel == SRC_SEL_IMM) {
            Src2 = ("chain right");
        } else {
            Src2 = ("undefined");
        }
        if (command->blocking) {
            Src2.append(" is blocking");
        }
        if (command->is_chain) {
            Src2.append(" is_chain");
        }
        if (command->flag_update) {
            Src2.append(" is flagupdate");
        }
        if (command->type == CommandVPRO::WAIT_BUSY) {
            Src2.append(" Cluster: " + QString::number(command->cluster) + " is WAIT_BUS>");
        }
        qDebug().noquote() << "   Mask: " + QString::number(command->id_mask) +
                                  " Sourceadress 1: " + Src1 + " Sourceadress 2:" + Src2;
        i++;
    }
}
