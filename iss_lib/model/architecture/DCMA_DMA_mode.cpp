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

#include "DCMA_DMA_mode.h"

DCMA_DMA_mode::DCMA_DMA_mode(NonBlockingBusSlaveInterface* bus, uint32_t number_cluster) {
    this->bus = bus;
    this->dmaRequests = std::vector<Request>(number_cluster);
    cur_req.is_done = true;
}

DCMA_DMA_mode::DCMA_DMA_mode() {
    this->bus = NULL;
    cur_req.is_done = true;
}

void DCMA_DMA_mode::requestDmaReadTransfer(
    intptr_t byte_addr, uint32_t burst_length, uint32_t initiator_id) {
    Request cur_request;
    cur_request.id = initiator_id;
    cur_request.byte_addr = byte_addr;
    cur_request.burst_length = burst_length;
    cur_request.is_read_transfer = true;

    dmaRequests[initiator_id] = cur_request;

    // TODO
    dmaRequestFifo.push(cur_request);
}

void DCMA_DMA_mode::requestDmaWriteTransfer(
    intptr_t byte_addr, uint32_t burst_length, uint32_t initiator_id) {
    Request cur_request;
    cur_request.id = initiator_id;
    cur_request.byte_addr = byte_addr;
    cur_request.burst_length = burst_length;
    cur_request.is_read_transfer = false;
    cur_request.is_ready_for_dma = true;

    dmaRequests[initiator_id] = cur_request;

    // TODO
    dmaRequestFifo.push(cur_request);
}

uint32_t DCMA_DMA_mode::getNextBusAlignedAddr(uint32_t addr) {
    return (uint32_t(addr / bus_dataword_length_byte) + 1) * bus_dataword_length_byte;
}

void DCMA_DMA_mode::readData(uint32_t initiator_id, uint8_t* read_data) {
    if (initiator_id != cur_req.id) {
        printf_error("ERROR: VPRO Main Memory Not Ready for Read Data of this ID\n");
        return;
    }

    if (!cur_req.is_ready_for_dma) {
        printf_error("ERROR: VPRO Main Memory Not Ready for Read Data\n");
        return;
    }

    // get 16bit word aligned data
    uint32_t align_addr = cur_req.current_burst_iter * dma_dataword_length_byte +
                          cur_req.byte_addr % bus_dataword_length_byte;
#ifdef ISS_STANDALONE
    if (cur_req.byte_addr >= reinterpret_cast<NonBlockingMainMemory*>(bus)->getMemByteSize())
        align_addr = cur_req.current_burst_iter * dma_dataword_length_byte;
#endif
    for (int i = 0; i < dma_dataword_length_byte; ++i) {
        read_data[i] = buffer[align_addr + i];
    }

    cur_req.current_burst_iter++;

    // check if complete read command is finished
    if (cur_req.current_burst_iter == cur_req.burst_length) {
        cur_req.is_done = true;
        cur_req.is_ready_for_dma = false;
    }
}

void DCMA_DMA_mode::writeData(const uint32_t initiator_id, const uint8_t* data) {
    if (initiator_id != cur_req.id) {
        printf_error("ERROR: VPRO Main Memory Not Ready for Write Data of this ID\n");
        return;
    }

    if (!cur_req.is_ready_for_dma) {
        printf_error("ERROR: VPRO Main Memory Not Ready for Write Data\n");
        return;
    }

    // write 16bit word aligned data to buffer
    uint32_t align_addr = cur_req.current_burst_iter * dma_dataword_length_byte +
                          cur_req.byte_addr % bus_dataword_length_byte;
#ifdef ISS_STANDALONE
    if (cur_req.byte_addr >= reinterpret_cast<NonBlockingMainMemory*>(bus)->getMemByteSize())
        align_addr = cur_req.current_burst_iter * dma_dataword_length_byte;
#endif
    for (int i = 0; i < dma_dataword_length_byte; ++i) {
        buffer[align_addr + i] = data[i];
    }

    cur_req.current_burst_iter++;

    // check if all write data is transfered
    if (cur_req.current_burst_iter == cur_req.burst_length) {
        // data can be written to bus in the next tick
        cur_req.is_ready_for_dma = false;
        waiting_for_bus = true;
    }
}

bool DCMA_DMA_mode::isReadDataAvailable(uint32_t initiator_id) {
    if (cur_req.id != initiator_id) return false;
    return cur_req.is_ready_for_dma;
}

bool DCMA_DMA_mode::isWriteDataReady(uint32_t initiator_id) {
    if (cur_req.id != initiator_id) return false;
    return cur_req.is_ready_for_dma;
}

bool DCMA_DMA_mode::isWaitingForBus() {
    return waiting_for_bus;
}

bool DCMA_DMA_mode::isBusy() {
    return waiting_for_bus || !dmaRequestFifo.empty();
}

void DCMA_DMA_mode::tick() {
    // iterate through every dma
    for (int i = 0; i < dmaRequests.size(); ++i) {}

    if (cur_req.is_done && !dmaRequestFifo.empty()) {
        //get new request from fifo
        cur_req = dmaRequestFifo.front();
        dmaRequestFifo.pop();

        // allocate buffer for read/write data
        uint32_t bus_burst_length = calcBusBurstLength(cur_req.byte_addr, cur_req.burst_length);
        buffer = std::vector<uint8_t>(bus_burst_length * bus_dataword_length_byte, 0);
        if (cur_req.is_read_transfer)
            waiting_for_bus = true;
        else {
            // workaround for not having byte enable, not necessary with cache
            // TODO: remove when using cache
            for (int i = 0; i < bus_burst_length * bus_dataword_length_byte; ++i) {
#ifdef ISS_STANDALONE
                if (cur_req.byte_addr <
                    reinterpret_cast<NonBlockingMainMemory*>(bus)->getMemByteSize()) {
                    bus->dbgRead(intptr_t(cur_req.byte_addr / bus_dataword_length_byte) *
                                         bus_dataword_length_byte +
                                     i,
                        &buffer[i]);
                }
#else
                bus->dbgRead(intptr_t(cur_req.byte_addr / bus_dataword_length_byte) *
                                     bus_dataword_length_byte +
                                 i,
                    &buffer[i]);
#endif
            }
        }
    }
    if (!cur_req.is_done) {
        if (cur_req.is_waiting_for_bus) {
            if (cur_req.is_read_transfer) {
                if (bus->isReadDataAvailable(cur_req.id)) {
                    bus->readData(&buffer[0], cur_req.id);
                    cur_req.is_waiting_for_bus = false;
                    waiting_for_bus = false;
                    cur_req.is_ready_for_dma = true;
                }
            } else {
                if (bus->isWriteDataReady(cur_req.id)) {
                    cur_req.is_waiting_for_bus = false;
                    waiting_for_bus = false;
                    cur_req.is_done = true;
                }
            }
        }
        // check if req is to be loaded from bus (read) or ready to write to bus (write)
        else if (!cur_req.is_ready_for_dma) {
            uint32_t bus_burst_length = calcBusBurstLength(cur_req.byte_addr, cur_req.burst_length);
            intptr_t bus_addr =
                intptr_t(cur_req.byte_addr / bus_dataword_length_byte) * bus_dataword_length_byte;
#ifdef ISS_STANDALONE
            if (cur_req.byte_addr >=
                reinterpret_cast<NonBlockingMainMemory*>(bus)->getMemByteSize()) {
                bus_addr = cur_req.byte_addr;
                bus_burst_length = cur_req.burst_length;
            }
#endif
            bool tlm_accepted;
            if (cur_req.is_read_transfer) {
                // start read request to bus
                tlm_accepted = bus->requestReadTransfer(bus_addr, bus_burst_length, cur_req.id);
            } else {
                // start write request to bus
                tlm_accepted =
                    bus->requestWriteTransfer(bus_addr, &buffer[0], bus_burst_length, cur_req.id);
            }
            cur_req.is_waiting_for_bus = tlm_accepted;
            waiting_for_bus = true;
        }
    }
}

uint32_t DCMA_DMA_mode::calcBusBurstLength(intptr_t start_addr, uint32_t dma_burst_length) {
    intptr_t base_addr_bus =
        bus_dataword_length_byte * intptr_t(start_addr / bus_dataword_length_byte);
    intptr_t end_addr_bus = bus_dataword_length_byte *
                            intptr_t((start_addr + dma_burst_length * dma_dataword_length_byte) /
                                     bus_dataword_length_byte);
    return uint32_t((end_addr_bus - base_addr_bus) / bus_dataword_length_byte) + 1;
}
