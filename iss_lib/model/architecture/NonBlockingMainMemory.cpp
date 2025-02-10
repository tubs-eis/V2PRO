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

#include "NonBlockingMainMemory.h"
#include <errno.h>
#include <sys/mman.h>
#include "../../simulator/helper/debugHelper.h"

NonBlockingMainMemory::NonBlockingMainMemory(uint64_t memory_byte_size) {
    printf("# [NonBlockingMainMemory] Allocating memory... (Size: %lu Bytes)\n", memory_byte_size);
    this->memory = (uint8_t*)(mmap(
        NULL, memory_byte_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));

    if (this->memory == ((uint8_t*)MAP_FAILED)) {
        printf_error("# [NonBlockingMainMemory] Allocating memory failure ... MMAP");
        perror("NonBlockingMainMemory");
    }
    this->memory_byte_size = memory_byte_size;

    if (gen_mem_trace) {
        std::stringstream ss;
        ss << "main_mem.trace";
        mem_trace = std::ofstream(ss.str().c_str());
    }
}

void NonBlockingMainMemory::tick() {
    for (auto& kr : incomingReadTransfers) {
        if (kr.second.cycle_counter == cycles_read_latency) kr.second.is_done = true;
        kr.second.cycle_counter++;
    }

    for (auto& kr : incomingWriteTransfers) {
        if (kr.second.cycle_counter == cycles_write_latency) kr.second.is_done = true;
        kr.second.cycle_counter++;
    }

    tick_counter++;
}

bool NonBlockingMainMemory::isReadDataAvailable(uint32_t initiator_id) {
    return incomingReadTransfers[initiator_id].is_done;
}

bool NonBlockingMainMemory::isWriteDataReady(uint32_t initiator_id) {
    bool result = incomingWriteTransfers[initiator_id].is_done;
    if (result) incomingWriteTransfers.erase(initiator_id);
    return result;
}

bool NonBlockingMainMemory::readData(uint8_t* data_ptr, const uint32_t initiator_id) {
    auto& cur_req = incomingReadTransfers[initiator_id];

    if (!cur_req.is_done) {
        printf_error("ERROR: VPRO Main Memory Not Ready for Read Data\n");
        return false;
    }

    if (cur_req.byte_addr < memory_byte_size) {
        if (debug & DEBUG_EXT_MEM)
            printf_info(
                "[MM] read from ext to loc (addr = 0x%08X) burst_length in 512bit-words: %i\n",
                cur_req.byte_addr,
                cur_req.burst_length);
        for (int i = 0; i < dataword_length_byte * cur_req.burst_length; ++i) {
            data_ptr[i] = memory[cur_req.byte_addr + i];
            if (debug & DEBUG_EXT_MEM && i % 2 == 0)
                printf_info("\tread data at ext addr (0x%08X): %i = 0x%04X \n",
                    cur_req.byte_addr + i,
                    ((int16_t*)data_ptr)[i / 2],
                    ((int16_t*)data_ptr)[i / 2]);
        }
    } else {
        for (int i = 0; i < dma_length_byte * cur_req.burst_length; ++i)
            memcpy(&data_ptr[i], (uint8_t*)(cur_req.byte_addr + i), 1);
    }

    incomingReadTransfers.erase(initiator_id);
    return true;
}

bool NonBlockingMainMemory::requestReadTransfer(
    intptr_t dst_addr_ptr, const uint32_t burst_length, const uint32_t initiator_id) {
    Request cur_request;
    cur_request.byte_addr = dst_addr_ptr;
    cur_request.burst_length = burst_length;
    cur_request.is_done = false;
    incomingReadTransfers[initiator_id] = cur_request;

    if (gen_mem_trace) {
        mem_trace << tick_counter << "," << dst_addr_ptr << "," << burst_length << ",r\n";
        mem_trace.flush();
    }

    return true;
}

bool NonBlockingMainMemory::requestWriteTransfer(intptr_t dst_addr_ptr,
    const uint8_t* data_ptr,
    const uint32_t burst_length,
    const uint32_t initiator_id) {
    Request cur_request;
    cur_request.byte_addr = dst_addr_ptr;
    cur_request.data_ptr = const_cast<uint8_t*>(data_ptr);
    cur_request.burst_length = burst_length;
    cur_request.is_done = false;
    incomingWriteTransfers[initiator_id] = cur_request;

    if (dst_addr_ptr < memory_byte_size) {
        if (debug & DEBUG_EXT_MEM)
            printf_info(
                "[MM] write from loc to ext (addr = 0x%08X) burst_length in 512bit-words: %i\n",
                cur_request.byte_addr,
                cur_request.burst_length);
        for (int i = 0; i < dataword_length_byte * cur_request.burst_length; ++i) {
            memory[cur_request.byte_addr + i] = data_ptr[i];
            if (debug & DEBUG_EXT_MEM && i % 2 == 0)
                printf_info("\t write data at ext addr (0x%08X): %i = 0x%04X \n",
                    cur_request.byte_addr + i,
                    *((int16_t*)&data_ptr[i]),
                    *((int16_t*)&data_ptr[i]));
        }
    } else {
        for (int i = 0; i < dma_length_byte * cur_request.burst_length; ++i)
            memcpy((uint8_t*)(cur_request.byte_addr + i), &data_ptr[i], 1);
    }

    if (gen_mem_trace) {
        mem_trace << tick_counter << "," << dst_addr_ptr << "," << burst_length << ",w\n";
        mem_trace.flush();
    }

    return true;
}

void NonBlockingMainMemory::dbgWrite(intptr_t dst_addr, uint8_t* data_ptr) {
    memory[dst_addr] = data_ptr[0];
}

void NonBlockingMainMemory::dbgRead(intptr_t dst_addr, uint8_t* data_ptr) {
    data_ptr[0] = memory[dst_addr];
}

uint64_t NonBlockingMainMemory::getMemByteSize() const {
    return memory_byte_size;
}
