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
// # DMA class                                            #
// ########################################################

#ifndef VPRO_CPP_DMA_H
#define VPRO_CPP_DMA_H

// C std libraries
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <list>
#include <memory>
#include <vector>

#include <QMap>

#include "../../simulator/helper/debugHelper.h"
#include "../commands/CommandDMA.h"
#include "ArchitectureState.h"
#include "DCMA.h"
#include "NonBlockingBusSlaveInterface.h"
#include "unit/VectorUnit.h"

class Cluster;

class DMA {
   public:
    DMA(Cluster* cluster,
        std::vector<std::shared_ptr<Unit::IVectorUnit>> units,
        double& time,
        std::shared_ptr<ArchitectureState> architecture_state,
        DCMA* dcma);

    ~DMA();

    /**
     * check if queue is processed
     * @return
     */
    bool isBusy();

    /**
     * push to DMA's cmd queue
     * @param cmd
     */
    void execute_cmd(const std::shared_ptr<CommandDMA>& cmd);

    /**
     * clock tick to process queues front command
     */
    void tick();

    std::shared_ptr<CommandDMA> getCmd();

    void setExternalVariableInfo(QMap<uint64_t, QMap<int, QString>>* map) {
        this->map = map;
    };

    uint32_t io_read(uint32_t addr);

    void io_write(uint32_t addr, uint32_t value);

   private:
    // sim time
    double& time;

    // delay simulates axi delay for each read command
    int32_t delay_cnt = 0;
    bool delay_active = false;

    struct IO_register {
        uint32_t loc_base;
        uint32_t ext_base;
        uint32_t x_size;
        uint32_t y_size;
        uint32_t x_stride;
        uint64_t unit_mask;
        bool pad_left, pad_right, pad_top, pad_bottom;
    } io_register;

    struct Iteration {
        uint32_t x;
        uint32_t y;
        uint32_t loc_addr;
        intptr_t ext_addr;
        uint32_t remaining_req_elements = 0;  // from DCMA
        uint32_t total_remaining_elements;
        uint32_t ext_addr_index = 0;  // ext_addr + index = burst element (for debug)
    } cur_iteration;

    // to detect new access to an external memory segment.
    // Could cause memory overflow (access to extern instead of main memory if address too large)
    uint64_t ext_addr_base_lst;

    static const uint64_t THIS_IS_LOAD = 42
                                      << 11;  // random numbers to distinguish between LOC -> EXT
    static const uint64_t THIS_IS_STORE = 42 << 12;

    const int DMA_DATA_WIDTH = 16;  // TODO: to VPRO_CFG!
    const int DCMA_DATA_WIDTH = 128; // TODO: to VPRO_CFG!

    // referenced units with LM instances
    std::vector<std::shared_ptr<Unit::IVectorUnit>> units;
    // superior cluster (containing the units)
    Cluster* cluster;
    // connection to the main memory (extern; reference)
    //    uint8_t *main_memory;
    DCMA* dcma;

    // reference to global architecture state
    std::shared_ptr<ArchitectureState> architecture_state;

    QMap<uint64_t, QMap<int, QString>>* map;

    // current DMA command with all its informations...
    std::shared_ptr<CommandDMA> command;

    std::shared_ptr<CommandDMA> noneCmd;
    std::list<std::shared_ptr<CommandDMA>> cmd_queue;

    bool DMA_gen_trace{false};

    std::ofstream trace;
    std::ofstream cmd_trace;

    int id_counter = 0;

    void createWriteRequest(uint32_t y_iteration);

    void createReadRequest(uint32_t y_iteration);

    void increment_iteration(uint32_t burst_length = 1, bool is_padding = true);

    /**
     * Will write to LM.
     * Byte Count = 2 (TODO: add parameter here, needed by LM)
     * @param element_addr in LMs of all command->units
     * @param byte_data
     */
    void write_to_LM(const uint32_t& element_addr, const uint8_t* byte_data);

    bool is_padding_region(const CommandDMA& dma_command, const Iteration& iteration) const;

    bool is_read_transfer(const CommandDMA& dma_command) const;

    void openTraceFile(const char* tracefilename);
};

#endif  //VPRO_CPP_DMA_H
