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

#ifndef TEMPLATE_NONBLOCKINGMAINMEMORY_H
#define TEMPLATE_NONBLOCKINGMAINMEMORY_H

#include <stdio.h>
#include <fstream>
#include <map>
#include <sstream>
#include "NonBlockingBusSlaveInterface.h"

class NonBlockingMainMemory : public NonBlockingBusSlaveInterface {
   public:
    explicit NonBlockingMainMemory(uint64_t memory_byte_size);

    uint8_t* getMemory() {
        return memory;
    }

    void tick();

    bool requestReadTransfer(
        intptr_t dst_addr_ptr, uint32_t burst_length, uint32_t initiator_id) override;

    bool isReadDataAvailable(uint32_t initiator_id) override;

    bool readData(uint8_t* data_ptr, uint32_t initiator_id) override;

    bool requestWriteTransfer(intptr_t dst_addr_ptr,
        const uint8_t* data_ptr,
        uint32_t burst_length,
        uint32_t initiator_id) override;

    bool isWriteDataReady(uint32_t initiator_id) override;

    void dbgWrite(intptr_t dst_addr, uint8_t* data_ptr) override;

    void dbgRead(intptr_t dst_addr, uint8_t* data_ptr) override;

    [[nodiscard]] uint64_t getMemByteSize() const;

   private:
    uint8_t* memory;
    uint64_t memory_byte_size;
    uint32_t cycles_read_latency = 36;
    uint32_t cycles_write_latency = 6;

    struct Request {
        uint32_t burst_length{};
        intptr_t byte_addr{};
        uint8_t* data_ptr{};
        uint32_t cycle_counter = 0;
        bool is_done = true;
    };

    std::map<uint32_t, Request> incomingReadTransfers, incomingWriteTransfers;

    bool gen_mem_trace{false};

    std::ofstream mem_trace;

    uint32_t tick_counter = 0;
};

#endif  //TEMPLATE_NONBLOCKINGMAINMEMORY_H
