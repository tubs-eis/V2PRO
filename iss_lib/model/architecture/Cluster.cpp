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
/**
 * @file Cluster.cpp
 * @author Sven Gesper, IMS, Uni Hannover, 2019
 * @date 06.03.19
 *
 * Cluster
 */

#include <bitset>
#include <limits>  // std::numeric_limits

#include "../../simulator/ISS.h"
#include "../../simulator/helper/debugHelper.h"
#include "../../simulator/helper/typeConversion.h"
#include "Cluster.h"
#include "stats/Statistics.h"

Cluster::Cluster(ISS* core,
    int id,
    std::shared_ptr<ArchitectureState> architecture_state,
    DCMA* dcma,
    double& time)
    : core(core),
      cluster_id(id),
      architecture_state(architecture_state),
      time(time),
      waitBusy(false),
      waitDMABusy(false),
      vpro_time(0),
      dma_time(0) {
    auto unitsTemp = std::vector<std::shared_ptr<Unit::VectorUnit>>();
    for (int i = 0; i < VPRO_CFG::UNITS; i++) {
        unitsTemp.push_back(std::make_shared<Unit::VectorUnit>(i,
            cluster_id,
            time,
            architecture_state));
    }

    if (VPRO_CFG::UNITS > 1) {
        unitsTemp[0]->setNeighbors(unitsTemp[VPRO_CFG::UNITS - 1], unitsTemp[1]);
        for (int i = 1; i < VPRO_CFG::UNITS - 1; i++) {
            unitsTemp[i]->setNeighbors(unitsTemp[i - 1], unitsTemp[i + 1]);
        }
        unitsTemp[VPRO_CFG::UNITS - 1]->setNeighbors(
            unitsTemp[VPRO_CFG::UNITS - 2], unitsTemp[0]);
    } else {
        unitsTemp[0]->setNeighbors(unitsTemp[0], unitsTemp[0]);
    }

    units = {unitsTemp.begin(), unitsTemp.end()};

    dma = new DMA(this, units, time, architecture_state, dcma);
}

#ifndef ISS_STANDALONE
// FK Needed to access Core methods
void Cluster::memory_access_callback(
    bool read_write, uint64_t addr, uint32_t& data, uint32_t cluster_id) {
    this->core->m_cb(read_write, addr, data, cluster_id);
};
#endif

bool Cluster::isBusy() {
    for (auto unit : units) {
        if (unit->isBusy()) return true;
    }

    return dma->isBusy();
}

/**
 * Clock tick
 */
void Cluster::tick() {
    // cascade to DMAs
    if (dma_time <= time) {
        if (debug & DEBUG_TICK)
            printf("DMA Clock Cycle %.2lf (DMA Time: %.2lf ns) in Cluster %i\n",
                dma_time / core->getDMAClockPeriod(),
                dma_time,
                cluster_id);
        dma_time += core->getDMAClockPeriod();
        dma->tick();
        if (cluster_id == VPRO_CFG::CLUSTERS - 1)
            Statistics::get().tick(Statistics::clock_domains::DMA);
    }

    // check if time for clock tick is up
    if (vpro_time <= time) {
        if (debug & DEBUG_TICK)
            printf("VPRO Clock Cycle %.2lf (VPRO Time: %.2lf ns) in Cluster %i\n",
                vpro_time / core->getVPROClockPeriod(),
                vpro_time,
                cluster_id);
        vpro_time += core->getVPROClockPeriod();
        // cascade tick to units -> lanes

        for (auto unit : units) {
            unit->tick();
        }
        for (auto unit : units) {
            unit->update();
        }
        if (cluster_id == VPRO_CFG::CLUSTERS - 1)
            Statistics::get().tick(Statistics::clock_domains::VPRO);
    }
}

bool Cluster::isReadyForCommand() {
    for (auto unit : units) {
        if (unit->isCmdQueueFull()) return false;
    }
    if (waitBusy) {
        bool busy = false;
        for (auto unit : units) {
            busy |= unit->isBusy();
        }
        if (!busy) waitBusy = false;
        return !waitBusy;
    }
    if (waitDMABusy) {
        if (!dma->isBusy()) waitDMABusy = false;
        return !waitDMABusy;
    }
    return true;
}

bool Cluster::isWaitingForDMA() const {
    return waitDMABusy;
}

void Cluster::setWaitingForDMA(bool dma_id) {
    waitDMABusy = dma_id;
}

bool Cluster::isWaitingForVPRO() const {
    return waitBusy;
}

void Cluster::setWaitingForVPRO(bool vpro) {
    waitBusy = vpro;
}

/**
 * command from sim to this cluster. push to units or to dma
 * @param cmd
 */
bool Cluster::sendCMD(std::shared_ptr<CommandBase> cmd) {
    if (cmd->class_type == CommandBase::VPRO) {
        auto vprocmd = std::dynamic_pointer_cast<CommandVPRO>(cmd);
        if (debug & DEBUG_INSTRUCTION_SCHEDULING || debug & DEBUG_INSTRUCTION_SCHEDULING_BASIC) {
            printf("Cluster %i Got a new VPRO command (@time = %.2lf):", cluster_id, time);
            printf("\n\t");
            print_cmd(vprocmd.get());
        }
        uint32_t ret = 0;
        for (auto unit : units) {
            //*********************************************
            // Check global mask (for this unit)
            //*********************************************
            uint32_t unit_mask = 1u << unit->id();
            if (unit_mask & architecture_state->unit_mask_global) {
                if (!unit->trySendCMD(std::make_shared<CommandVPRO>(vprocmd.get()))) {
                    ret |= unit_mask;
                }
            }
        }
        if (ret != 0) {
            printf_error(
                "Command push to units failed! units queue full! -> TODO check before + repeat\n");
            printf_error("unit error count is: %i\n", ret);
        }
        return (ret == 0);
    } else if (cmd->class_type == CommandBase::DMA) {
        auto dmacmd = std::dynamic_pointer_cast<CommandDMA>(cmd);
        if (debug & DEBUG_INSTRUCTION_SCHEDULING) {
            printf("Cluster %i Got a new DMA command (@time = %.2lf):\n", cluster_id, time);
            printf("\t");
            print_cmd(dmacmd.get());
        }
        dma->execute_cmd(dmacmd);
        return true;
    } else {
        printf_warning(
            "Unknown Command type in Cluster %i received (@time = %.2lf)!\n", cluster_id, time);
        return true;
    }
}

void Cluster::dumpQueue(uint32_t unit) {
    this->units[unit]->dumpQueue();
}

void Cluster::dumpLocalMemory(uint32_t unit) {
    this->units[unit]->dumpLocalMemory();
}

void Cluster::dumpRegisterFile(uint32_t unit, uint32_t lane) {
    this->units[unit]->dumpRegisterFile(lane);
}
