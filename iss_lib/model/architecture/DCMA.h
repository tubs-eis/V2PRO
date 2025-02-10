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
// Created by thieu on 23.03.22.
//

#ifndef TEMPLATE_DCMA_H
#define TEMPLATE_DCMA_H

#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include "../../simulator/helper/debugHelper.h"
#include "Cache.h"
#include "DCMA_DMA_mode.h"
#include "NonBlockingBusSlaveInterface.h"
#include "stats/Statistics.h"

#ifdef ISS_STANDALONE
#include "NonBlockingMainMemory.h"
#endif

#ifndef ISS_STANDALONE
#include "NonBlockingBusSlaveImpl.h"
#endif

class DCMA {
   public:
    // constants
    static constexpr int dma_dataword_length_byte = 16 / 8;
    static constexpr int dcma_dataword_length_byte = 128 / 8;
    static constexpr int bus_dataword_length_byte = 512 / 8;

    DCMA(ISS* core,
        NonBlockingBusSlaveInterface* bus,
        uint32_t number_cluster,
        uint32_t line_size,
        uint32_t associativity,
        uint32_t nr_brams,
        uint32_t bram_size);

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

    bool isBusy(uint32_t initiator_id);

    void reset();

    void flush();

    bool isBusy();

    bool getDCMAOff();

    uint32_t getNrRams();

    uint32_t getRamSize();

    uint32_t getLineSize();

    uint32_t getAssociativity();

   private:
    // definitions
    enum DCMA_Mode {
        REALISTIC,
        IDEAL,  // 100% hitrate
        DMA  // no cache, all DMA accesses are serviced sequentially as 512-bit burst transfers, no parallel DMA access
    };

    struct Request {
        uint32_t id{};
        uint32_t burst_length{};          // in dma datawords
        uint32_t current_burst_iter = 0;  // counts number of words already transfered to/from dma
        intptr_t byte_addr{};
        uint32_t latency_wait_counter{};
        bool is_new_access = true;  // for hit/miss counter
        bool is_done = true;
    };

    struct Params {
        bool DCMA_OFF{};
        uint32_t nr_brams{};
        uint32_t bram_size{};
        uint32_t line_size{};
        uint32_t associativity{};
    } params;

    DCMA_Mode dcma_mode = REALISTIC;

    // object pointers
    ISS* core;
    NonBlockingBusSlaveInterface* bus;
    Cache cache;
    DCMA_DMA_mode dcma_dma_mode;

    // local register/memory
    std::vector<uint8_t> buffer;
    uint32_t pointer_nxt_dma_miss = 0;
    uint32_t number_cluster;

    // DMA
    std::vector<Request> dmaRequests;
};

#endif  //TEMPLATE_DCMA_H
