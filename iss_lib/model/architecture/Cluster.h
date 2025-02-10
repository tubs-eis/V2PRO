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
// ########################################################
// # VPRO instruction & system simulation library         #
// # Sven Gesper, IMS, Uni Hannover, 2019                 #
// ########################################################
// # Cluster class                                        #
// ########################################################

#ifndef VPRO_CPP_CLUSTER_H
#define VPRO_CPP_CLUSTER_H

// C std libraries
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include <vector>

#include <QMutex>
#include <QThread>

#include <vpro/vpro_special_register_enums.h>
#include "ArchitectureState.h"
#include "DCMA.h"
#include "DMA.h"
#include "unit/VectorUnit.h"

class ISS;

class Cluster : public QThread {
    Q_OBJECT
   public:
    int cluster_id;
    std::shared_ptr<ArchitectureState> architecture_state;

    DMA* dma;
    ISS* core;

#ifdef THREAD_CLUSTER
    QMutex tick_en;
    QMutex tick_done;
#endif

    Cluster(ISS* core,
        int id,
        std::shared_ptr<ArchitectureState> architecture_state,
        DCMA* dcma,
        double& time);

#ifdef THREAD_CLUSTER
    [[noreturn]] void run() override {
        while (true) {
            tick_en.lock();
            tick();
            tick_done.unlock();
        }
    }
#endif

    void tick();

#ifndef ISS_STANDALONE
    // FK: Callback for accessing core methods
    void memory_access_callback(
        bool read_write, uint64_t addr, uint32_t& data, uint32_t cluster_id);
#endif

    /**
     * send commands into unit queue or dma queue
     * @param cmd vpro command to be processed (id sensitive for cluster/unit?!)
     */
    bool sendCMD(std::shared_ptr<CommandBase> cmd);

    void dumpLocalMemory(uint32_t unit);

    void dumpQueue(uint32_t unit);

    void dumpRegisterFile(uint32_t unit, uint32_t lane);

    bool isBusy();

    bool isReadyForCommand();

    bool isWaitingForDMA() const;

    void setWaitingForDMA(bool dma);

    bool isWaitingForVPRO() const;

    void setWaitingForVPRO(bool vpro);

    [[nodiscard]] bool isWaitBusy() const {
        return waitBusy;
    };

    [[nodiscard]] bool isWaitDMABusy() const {
        return waitDMABusy;
    };

    std::vector<std::shared_ptr<Unit::IVectorUnit>>& getUnits() {
        return units;
    }

   private:
    // if receive of command wait busy caused waiting
    bool waitBusy;
    bool waitDMABusy;

    double& time;  // sim time
    // time of vpro (required to determine clock tick occurence)
    double vpro_time, dma_time;

    // all the VectorUnits in this cluster are stored here
    std::vector<std::shared_ptr<Unit::IVectorUnit>> units;
};

#endif  //VPRO_CPP_CLUSTER_H
