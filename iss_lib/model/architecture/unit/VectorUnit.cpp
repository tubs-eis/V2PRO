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
//
// Created by gesper on 06.03.19.
//
#include <QString>

#include "../../../simulator/ISS.h"
#include "../../../simulator/helper/debugHelper.h"
#include "../../../simulator/helper/typeConversion.h"
#include "VectorUnit.h"

namespace Unit {

VectorUnit::VectorUnit(int id,
    int cluster_id,
    double& time,
    std::shared_ptr<ArchitectureState> architecture_state)
    : cluster_id(cluster_id),
      time(time) {
    vector_unit_id = id;

    // LM interpretated in 24-bit segments
    local_memory = new uint8_t[VPRO_CFG::LM_SIZE * (LOCAL_MEMORY_DATA_WIDTH / 8)]();

    // create regular Lanes
    for (int i = 0; i < VPRO_CFG::LANES; i++) {
        lanes.push_back(std::make_shared<VectorLane>(
            i, this, architecture_state));
    }
    // create LS Lane
    ls_lane = std::make_shared<VectorLaneLS>(this, architecture_state);
    lanes.push_back(ls_lane);

    // set neighbor info to all Lanes
    lanes[0]->setNeighbors(lanes[VPRO_CFG::LANES - 1], lanes[1], lanes.back());
    for (int i = 1; i < VPRO_CFG::LANES - 1; i++) {
        lanes[i]->setNeighbors(lanes[i - 1], lanes[i + 1], ls_lane);
    }
    lanes[VPRO_CFG::LANES - 1]->setNeighbors(lanes[VPRO_CFG::LANES - 2], lanes[0], ls_lane);
    lanes.back()->setNeighbors(lanes[0], lanes[VPRO_CFG::LANES - 1], ls_lane);  // this is LS lane

    // init commands
    cmd_queue = std::deque<std::shared_ptr<CommandVPRO>>();
    clearCommands();
    noneCmd = std::make_shared<CommandVPRO>();

    cmdQueueFetchedCmd = false;
};

bool VectorUnit::isLaneSelected(CommandVPRO const* cmd, long lane_id) const {
    if (long(lane_id) > VPRO_CFG::LANES) {  // this is L/S lane check
        return cmd->isLS();
    }

    return ((uint(cmd->id_mask) >> uint(lane_id)) & 1u) == 1;
}

bool VectorUnit::isCmdQueueFull() {
    return (cmd_queue.size() > UNITS_COMMAND_QUEUE_MAX_SIZE);
}
// cmds from cluster/sim push into units queue
bool VectorUnit::trySendCMD(const std::shared_ptr<CommandVPRO>& cmd) {
    //*********************************************
    // Push to Units Queue
    //*********************************************
    if (cmd->type == CommandVPRO::NONE) return true;

    if (isCmdQueueFull()) {
        printf_error(
            "UNIT received CMD but Q is full! should wait (by cluster) until space available...\n");
        return false;
    }

    cmd_queue.push_back(cmd);

    return true;
}

void VectorUnit::clearCommands() {
    cmd_queue.clear();
}

void VectorUnit::tick() {
    // cascade
    for (auto lane : lanes) {
        lane->tick();
    }
}

void VectorUnit::update() {
    cmdQueueFetchedCmd = false;  // enable fetch of one new command in this cycle
    for (auto lane : lanes) {
        lane->update();
    }
}

/**
 * gets command for the lane to process. all lane command queues should be at the same state for x/y/is_done ...
 * @param id Lane
 * @return
 */
std::shared_ptr<CommandVPRO> VectorUnit::getNextCommandForLane(int id) {
    if (cmdQueueFetchedCmd) {  // FIFO can only fetch front cmd
        return noneCmd;
    }

    if (cmd_queue.empty()) {
        return noneCmd;
    }

    for (auto lane : lanes) {
        if (lane->isBlocking()) return noneCmd;
    }

    auto cmd = cmd_queue.front();
    if (isLaneSelected(cmd.get(), id)) {
        // Check minimal vector length + print warning
        if (checkVectorPipelineLength && cmd->fu_sel != CLASS_OTHER && cmd->x == cmd->x_end &&
            cmd->y == cmd->y_end) {
            uint32_t vector_length = (cmd->x_end + 1) * (cmd->y_end + 1);
            if ((vector_length < 10) && (cmd->fu_sel != CLASS_MEM)) {
                cmd->print_type();
                printf_warning(
                    "Vector length(%d) smaller than pipeline depth(%d)! User should add wait "
                    "states!\n",
                    vector_length,
                    10);
            }
        }

        cmd_queue.front()->id_mask &= ~(1u << uint(id));
        // unset the mask bit for this unit so this command dont get assigned to it again
        if (cmd_queue.front()->id_mask == 0) {
            // if assigned to all lanes it should be poped from queue
            cmd_queue.pop_front();
            cmdQueueFetchedCmd = true;
            cmd->id_mask |= (1u << uint(id));
            return cmd;
        } else {
            return std::make_shared<CommandVPRO>(cmd.get());
        }
    }

    return noneCmd;
}

uint32_t VectorUnit::getLocalMemoryData(const uint32_t addr, const int size) {
    if (addr * (LOCAL_MEMORY_DATA_WIDTH / 8) + size >
        VPRO_CFG::LM_SIZE * (LOCAL_MEMORY_DATA_WIDTH / 8)) {
        printf_error("Read from Local Memory out of Range! (addr: %d, size: %d)\n", addr, size);
        return 0;
    }
    if (size != 2) {
        printf_error("LM Read for size != 2 [NOT IMPLEMENTED!]\n");
    }
    return uint32_t(*((uint16_t*)&local_memory[addr * 2]));
}

void VectorUnit::writeLocalMemoryData(const uint32_t addr, const uint32_t data, const int size) {
    if (addr + (LOCAL_MEMORY_DATA_WIDTH / 8) > VPRO_CFG::LM_SIZE * (LOCAL_MEMORY_DATA_WIDTH / 8)) {
        printf_error("Write to Local Memory out of Range! (addr: %d)\n", addr);
        return;
    }
    if (size > (LOCAL_MEMORY_DATA_WIDTH / 8)) {
        printf_error(
            "Write to Local Memory more than LM DATA WIDTH (%i)\n", (LOCAL_MEMORY_DATA_WIDTH / 8));
    }

    for (uint i = 0; i < size; i++) {
        local_memory[addr * (LOCAL_MEMORY_DATA_WIDTH / 8) + i] = uint8_t(data >> (i * 8));
    }
}

void VectorUnit::writeLocalMemoryData(const uint32_t& addr, const uint8_t* data, const int size) {
    if (addr * (LOCAL_MEMORY_DATA_WIDTH / 8) + (LOCAL_MEMORY_DATA_WIDTH / 8) >
        VPRO_CFG::LM_SIZE * (LOCAL_MEMORY_DATA_WIDTH / 8)) {
        printf_error(
            "Write to Local Memory out of Range! (addr: %d) Max: %i\n", addr, VPRO_CFG::LM_SIZE);
        return;
    }
    if (size > (LOCAL_MEMORY_DATA_WIDTH / 8)) {
        printf_error(
            "Write to Local Memory more than LM DATA WIDTH (%i)\n", (LOCAL_MEMORY_DATA_WIDTH / 8));
    }

    for (uint i = 0; i < size; i++) {
        local_memory[addr * (LOCAL_MEMORY_DATA_WIDTH / 8) + i] = data[i];
    }
}

bool VectorUnit::isBusy() {
    bool lanes_busy = !cmd_queue.empty();
    for (auto lane : lanes) {
        lanes_busy = lanes_busy || lane->isBusy();
    }
    return lanes_busy;
}

// ***********************************************************************
// Dump local memory to console
// ***********************************************************************
void VectorUnit::dumpLocalMemory(const std::string& prefix) {
    printf("%s", prefix.c_str());
    printf("DUMP Local Memory (Vector Unit %i):\n", vector_unit_id);
    printf(LGREEN);
    int printLast = 0;
    for (int l = 0; l < VPRO_CFG::LM_SIZE; l += 32) {
        bool hasData = false;
        for (int m = 0; m <= 31; m++) {
            uint32_t data = getLocalMemoryData(l + m);
            hasData |= (data != 0);
        }
        if (hasData) {
            printLast = std::min(printLast + 1, 1);
            printf(LGREEN);
            printf("%s", prefix.c_str());
            printf("$%04x:  ", l & (~(32 - 1)));
            for (int m = 0; m <= 31; m++) {  // print a line with 32 entries
                if (m == 16)                 // for line break...
                    printf("\n\t\t");
                uint32_t data = getLocalMemoryData(l + m);
                if (data == 0) {
                    printf(LIGHT);
                    printf("0x%04x, ", (uint32_t)data);  // data & 0xffff); // in gray
                    printf(NORMAL_);
                } else {
                    printf(LGREEN);
                    printf("0x%04x, ", data);  //data & 0xffff);
                }
            }
            printf("\n");
        } else {
            if (printLast == 0) {  // if no data, print ...
                printf("%s", prefix.c_str());
                printf("...\n");
            }
            printLast = std::max(printLast - 1, -1);
        }
    }
    printf(RESET_COLOR);
}

void VectorUnit::dumpRegisterFile(uint32_t lane) {
    lanes[lane]->dumpRegisterFile();
}

void VectorUnit::dumpQueue() {
    printf(LBLUE);
    printf("Command Queue (Cluster %i, Unit %i):\n", cluster_id, vector_unit_id);
    int i = 0;
    for (std::shared_ptr<CommandVPRO>& cmd : cmd_queue) {
        printf("%i: \t", i);
        cmd->print();
        i++;
    }
    printf("######### Size: %lu\n", cmd_queue.size());
    printf(RESET_COLOR);
}

}  // namespace Unit
