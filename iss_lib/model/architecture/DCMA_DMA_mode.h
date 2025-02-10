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
// Created by thieu on 08.06.22.
//

#ifndef DCMA_DMA_MODE_H
#define DCMA_DMA_MODE_H

#include <iostream>
#include <map>
#include <queue>
#include "../../simulator/helper/debugHelper.h"
#include "NonBlockingBusSlaveInterface.h"

#ifdef ISS_STANDALONE
#include "NonBlockingMainMemory.h"
#endif

#ifndef ISS_STANDALONE
#include "NonBlockingBusSlaveImpl.h"
#endif

class DCMA_DMA_mode {
   public:
    DCMA_DMA_mode(NonBlockingBusSlaveInterface* bus, uint32_t number_cluster);

    DCMA_DMA_mode();

    void tick();

    // DMA
    void requestDmaReadTransfer(intptr_t byte_addr,
        uint32_t burst_length,
        uint32_t initiator_id);  // burst_length in bus_word_length

    void requestDmaWriteTransfer(intptr_t byte_addr,
        uint32_t burst_length,
        uint32_t initiator_id);  // burst_length in bus_word_length

    bool isReadDataAvailable(const uint32_t initiator_id);

    bool isWriteDataReady(const uint32_t initiator_id);

    void readData(const uint32_t initiator_id, uint8_t* read_data);

    void writeData(const uint32_t initiator_id, const uint8_t* data);

    // bus interface
    bool isWaitingForBus();

    bool isBusy();

   private:
    // definitions
    struct Request {
        uint32_t id;
        uint32_t burst_length;            // in dma datawords
        uint32_t current_burst_iter = 0;  // counts number of words already transfered to/from dma
        intptr_t byte_addr;
        bool is_read_transfer;
        bool is_ready_for_dma = false;  // ready to be accessed by DMA
        bool is_waiting_for_bus = false;
        bool is_done = false;
    };

    // constants
    const static int dma_dataword_length_byte = 16 / 8;
    const static int bus_dataword_length_byte = 512 / 8;

    bool waiting_for_bus = false;

    // object pointers
    NonBlockingBusSlaveInterface* bus;

    // local register/memory
    std::vector<uint8_t> buffer;

    // DMA
    std::queue<Request> dmaRequestFifo;
    std::vector<Request> dmaRequests;
    Request cur_req;

    // functions
    uint32_t getNextBusAlignedAddr(uint32_t addr);

    uint32_t calcBusBurstLength(intptr_t start_addr, uint32_t dma_burst_length);
};

#endif  //DCMA_DMA_MODE_H
