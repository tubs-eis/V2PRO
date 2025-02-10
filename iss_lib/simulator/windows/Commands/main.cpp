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
#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QMetaType>
#include "../../../model/commands/CommandDMA.h"
#include "../../../model/commands/CommandSim.h"
#include "../../../model/commands/CommandVPRO.h"
#include "commandwindow.h"

#include <memory>

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);

    QVector<QVector<std::shared_ptr<CommandVPRO>>> command_queue;
    QVector<std::shared_ptr<CommandVPRO>> commands;

    for (int x = 0; x < 4; x++) {
        QVector<std::shared_ptr<CommandVPRO>> cmd_queuevector;
        for (int i = 0; i < 4; i++) {
            cmd_queuevector.push_back(std::make_shared<CommandVPRO>());
        }
        command_queue.push_back(cmd_queuevector);
    }
    for (int y = 0; y < 8; y++) {
        commands.push_back(std::make_shared<CommandVPRO>());
        commands.push_back(std::make_shared<CommandVPRO>());
    }

    CommandWindow::Data data;

    CommandWindow w(4, 8);

    uint32_t ACCU_MAC_HIGH_BIT_SHIFT{1};
    uint32_t ACCU_MUL_HIGH_BIT_SHIFT{2};
    VPRO::MAC_INIT_SOURCE MAC_ACCU_INIT_SOURCE{VPRO::MAC_INIT_SOURCE::ADDR};
    uint32_t dma_pad_top{3};
    uint32_t dma_pad_right{4};
    uint32_t dma_pad_bottom{5};
    uint32_t dma_pad_left{6};
    uint32_t dma_pad_value{7};
    uint32_t unit_mask_global{8};
    uint32_t cluster_mask_global{9};

    CommandWindow::VproSpecialRegister v;
    v.ACCU_MAC_HIGH_BIT_SHIFT = &ACCU_MAC_HIGH_BIT_SHIFT;
    v.ACCU_MUL_HIGH_BIT_SHIFT = &ACCU_MUL_HIGH_BIT_SHIFT;
    v.MAC_ACCU_INIT_SOURCE = &MAC_ACCU_INIT_SOURCE;
    v.dma_pad_bottom = &dma_pad_bottom;
    v.dma_pad_left = &dma_pad_left;
    v.dma_pad_right = &dma_pad_right;
    v.dma_pad_top = &dma_pad_top;
    v.dma_pad_value = &dma_pad_value;
    v.cluster_mask = &cluster_mask_global;
    v.unit_mask = &unit_mask_global;

    uint64_t* accu;
    int ci = 0;
    for (auto c : {0}) {
        v.accus.append(QVector<QVector<uint64_t*>>());
        int ui = 0;
        for (auto u : {0, 1}) {
            v.accus[ci].append(QVector<uint64_t*>());
            for (auto l : {0, 1}) {
                accu = new uint64_t;
                *accu = ci * 100 + ui * 10 + l * 1;
                v.accus[ci][ui].append(accu);
            }
            ui++;
        }
        ci++;
    }

    w.setVproSpecialRegisters(v);
    w.dataUpdate(data);
    *accu = 15615;
    w.dataUpdate(data);

    w.simUpdate(5);
    uint8_t* lm = new uint8_t[8192 * 2 * 2]();

    lm[0] = 0xff;
    lm[1] = 0xff;
    lm[3] = 0xff;

    w.setLocalMemory(0, &(lm[0]), 0);
    w.setLocalMemory(1, &(lm[8192 * 2]), 0);

    w.show();

    return a.exec();
}
