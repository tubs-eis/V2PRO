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
// # Unit class                                           #
// ########################################################

#ifndef VPRO_CPP_VECTORUNIT_H
#define VPRO_CPP_VECTORUNIT_H

// C std libraries
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <QVector>
#include <list>
#include <memory>
#include <vector>

#include "../../../simulator/setting.h"
#include "IVectorUnit.h"
#include "VectorLane.h"
#include "VectorLaneLS.h"

// to be included in cpp
class Cluster;

namespace Unit {

class VectorUnit : public IVectorUnit {
   public:
    const static int LOCAL_MEMORY_DATA_WIDTH = 16;  // TODO: Put into VPRO_CFG!

    int vector_unit_id;
    int cluster_id;

    VectorUnit(int id,
        int cluster_id,
        double& time,
        std::shared_ptr<ArchitectureState> architecture_state);

    void update_stall_conditions();
    void check_stall_conditions();
    void tick();
    void update();

    int id() {
        return vector_unit_id;
    }

    std::shared_ptr<IVectorUnit> getLeftNeighbor() {
        return left_unit;
    }

    std::shared_ptr<IVectorUnit> getRightNeighbor() {
        return right_unit;
    }

    void setNeighbors(std::shared_ptr<IVectorUnit> left, std::shared_ptr<IVectorUnit> right) {
        left_unit = left;
        right_unit = right;
    };

    VectorLaneLS& getLSLane() {
        return *ls_lane;
    };

    uint32_t getLocalMemoryData(uint32_t addr, int size = 2);
    void writeLocalMemoryData(uint32_t addr, uint32_t data, int size = 2);
    void writeLocalMemoryData(const uint32_t& addr, const uint8_t* data, int size = 2);

    bool trySendCMD(const std::shared_ptr<CommandVPRO>& cmd);

    void clearCommands();

    void dumpLocalMemory(const std::string& prefix = "");
    void dumpRegisterFile(uint32_t lane);
    void dumpQueue();

    bool isBusy();

    std::shared_ptr<CommandVPRO> getNextCommandForLane(int id);

    std::vector<std::shared_ptr<VectorLane>>& getLanes() {
        return lanes;
    }

    bool isLaneSelected(CommandVPRO const* cmd, long lane_id) const;

    bool isCmdQueueFull();
    uint8_t* getLocalMemoryPtr() {
        return local_memory;
    }

    std::deque<std::shared_ptr<CommandVPRO>> getCopyOfCommandQueue() {
        return cmd_queue;
    }

   private:
    // all the VectorLanes in this unit are stored here
    std::vector<std::shared_ptr<VectorLane>> lanes;
    // neighbors to access (e.g. from LS chain)
    std::shared_ptr<IVectorUnit> left_unit;
    std::shared_ptr<IVectorUnit> right_unit;

    std::shared_ptr<VectorLaneLS> ls_lane;

    // cmd-queue from this unit.
    std::deque<std::shared_ptr<CommandVPRO>> cmd_queue;

    // reference to time from sim
    double& time;

    uint8_t* local_memory;  // each unit has a local_memory, the lanes can access

    // returned if cmd queue is empty (or similar...)
    std::shared_ptr<CommandVPRO> noneCmd;

    //flag to indicate a command was pulled from cmd queue. in hw only once per cycle a cmd is received from queue...
    bool cmdQueueFetchedCmd;
};

}  // namespace Unit

#endif  //VPRO_CPP_VECTORUNIT_H
