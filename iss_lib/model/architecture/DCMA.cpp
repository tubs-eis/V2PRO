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

#include "DCMA.h"

DCMA::DCMA(ISS* core,
    NonBlockingBusSlaveInterface* bus,
    uint32_t number_cluster,
    uint32_t line_size,
    uint32_t associativity,
    uint32_t nr_brams,
    uint32_t bram_size)
    : core(core),
      cache(core, line_size, associativity, nr_brams, bram_size, bus),
      dcma_dma_mode(bus, number_cluster) {
    this->bus = bus;
    this->dmaRequests = std::vector<Request>(number_cluster);
    this->number_cluster = number_cluster;
    this->params.DCMA_OFF = false;
    if (dcma_mode == DCMA_Mode::DMA) this->params.DCMA_OFF = true;
    this->params.nr_brams = nr_brams;
    this->params.bram_size = bram_size;
    this->params.line_size = line_size;
    this->params.associativity = associativity;
}

/**
 * interface function for dma to request a dma read transfer
 * @param byte_addr byte addr
 * @param burst_length burst_length in dma words
 * @param initiator_id cluster id
 */
void DCMA::requestDmaReadTransfer(
    intptr_t byte_addr, uint32_t burst_length, uint32_t initiator_id) {
    if (dcma_mode == DMA)
        return dcma_dma_mode.requestDmaReadTransfer(byte_addr, burst_length, initiator_id);

    Request cur_request;
    cur_request.id = initiator_id;
    cur_request.byte_addr = byte_addr;
    cur_request.burst_length = burst_length;
    cur_request.is_done = false;
    cur_request.latency_wait_counter = 6 * dcma_dataword_length_byte / dma_dataword_length_byte;

    dmaRequests[initiator_id] = cur_request;
}

/**
 * interface function for dma to request a dma write transfer
 * @param byte_addr byte addr
 * @param burst_length burst_length in dma words
 * @param initiator_id cluster id
 */
void DCMA::requestDmaWriteTransfer(
    intptr_t byte_addr, uint32_t burst_length, uint32_t initiator_id) {
    if (dcma_mode == DMA)
        return dcma_dma_mode.requestDmaWriteTransfer(byte_addr, burst_length, initiator_id);

    Request cur_request;
    cur_request.id = initiator_id;
    cur_request.byte_addr = byte_addr;
    cur_request.burst_length = burst_length;
    cur_request.is_done = false;
    cur_request.latency_wait_counter = 9 * dcma_dataword_length_byte / dma_dataword_length_byte;

    dmaRequests[initiator_id] = cur_request;
}

/**
 * interface function for dma to read data if the data is ready
 * @param initiator_id cluster id
 * @return uint8_t* read data pointer
 */
void DCMA::readData(uint32_t initiator_id, uint8_t* read_data) {
    Request* cur_req = &dmaRequests[initiator_id];

    if (dcma_mode == DMA) return dcma_dma_mode.readData(initiator_id, read_data);
    if (dcma_mode == IDEAL) {
#ifdef ISS_STANDALONE
        if (cur_req->byte_addr >= reinterpret_cast<NonBlockingMainMemory*>(bus)->getMemByteSize())
            memcpy(&read_data[0],
                (uint8_t*)(cur_req->byte_addr +
                           cur_req->current_burst_iter * dma_dataword_length_byte),
                dma_dataword_length_byte);
        else {
            bus->dbgRead(
                cur_req->byte_addr + dma_dataword_length_byte * cur_req->current_burst_iter,
                &read_data[0]);
            bus->dbgRead(
                cur_req->byte_addr + dma_dataword_length_byte * cur_req->current_burst_iter + 1,
                &read_data[1]);
        }
#else
        bus->dbgRead(cur_req->byte_addr + dma_dataword_length_byte * cur_req->current_burst_iter,
            &read_data[0]);
        bus->dbgRead(
            cur_req->byte_addr + dma_dataword_length_byte * cur_req->current_burst_iter + 1,
            &read_data[1]);
#endif
    } else {
#ifdef ISS_STANDALONE
        if (cur_req->byte_addr >= reinterpret_cast<NonBlockingMainMemory*>(bus)->getMemByteSize())
            memcpy(&read_data[0],
                (uint8_t*)(cur_req->byte_addr +
                           cur_req->current_burst_iter * dma_dataword_length_byte),
                dma_dataword_length_byte);
        else
            cache.dmaReadDataHit(
                cur_req->byte_addr + dma_dataword_length_byte * cur_req->current_burst_iter,
                &read_data[0]);
#else
        cache.dmaReadDataHit(
            cur_req->byte_addr + dma_dataword_length_byte * cur_req->current_burst_iter,
            &read_data[0]);
#endif
    }
    cur_req->current_burst_iter++;

    // check if complete read command is finished
    if (cur_req->current_burst_iter == cur_req->burst_length) {
        cur_req->is_done = true;
    }

    cur_req->is_new_access = true;
}

/**
 * interface function for dma to write data if the dcma is ready
 * @param initiator_id cluster id
 * @param data write data pointer
 */
void DCMA::writeData(const uint32_t initiator_id, const uint8_t* data) {
    Request* cur_req = &dmaRequests[initiator_id];

    if (dcma_mode == DMA) return dcma_dma_mode.writeData(initiator_id, data);
    if (dcma_mode == IDEAL) {
#ifdef ISS_STANDALONE
        if (cur_req->byte_addr >= reinterpret_cast<NonBlockingMainMemory*>(bus)->getMemByteSize())
            memcpy((uint8_t*)(cur_req->byte_addr +
                              cur_req->current_burst_iter * dma_dataword_length_byte),
                data,
                dma_dataword_length_byte);
        else {
            bus->dbgWrite(
                cur_req->byte_addr + dma_dataword_length_byte * cur_req->current_burst_iter,
                const_cast<uint8_t*>(&data[0]));
            bus->dbgWrite(
                cur_req->byte_addr + dma_dataword_length_byte * cur_req->current_burst_iter + 1,
                const_cast<uint8_t*>(&data[1]));
        }
#else
        bus->dbgWrite(cur_req->byte_addr + dma_dataword_length_byte * cur_req->current_burst_iter,
            const_cast<uint8_t*>(&data[0]));
        bus->dbgWrite(
            cur_req->byte_addr + dma_dataword_length_byte * cur_req->current_burst_iter + 1,
            const_cast<uint8_t*>(&data[1]));
#endif
    } else {
#ifdef ISS_STANDALONE
        if (cur_req->byte_addr >= reinterpret_cast<NonBlockingMainMemory*>(bus)->getMemByteSize())
            memcpy((uint8_t*)(cur_req->byte_addr +
                              cur_req->current_burst_iter * dma_dataword_length_byte),
                data,
                dma_dataword_length_byte);
        else
            cache.dmaWriteDataHit(
                cur_req->byte_addr + dma_dataword_length_byte * cur_req->current_burst_iter,
                const_cast<uint8_t*>(data));
#else
        cache.dmaWriteDataHit(
            cur_req->byte_addr + dma_dataword_length_byte * cur_req->current_burst_iter,
            const_cast<uint8_t*>(data));
#endif
    }
    cur_req->current_burst_iter++;

    // check if all write data is transfered
    if (cur_req->current_burst_iter == cur_req->burst_length) {
        // data can be written to bus in the next tick
        cur_req->is_done = true;
    }

    cur_req->is_new_access = true;
}

/**
 * interface function for dma to check if read data is ready
 * @param initiator_id cluster id
 * @return bool
 */
bool DCMA::isReadDataAvailable(uint32_t initiator_id) {
    if (dcma_mode == DMA) return dcma_dma_mode.isReadDataAvailable(initiator_id);
    if (dcma_mode == IDEAL) return true;
    Request* cur_req = &dmaRequests[initiator_id];

    if (cur_req->latency_wait_counter > 0) {
        cur_req->latency_wait_counter--;
        return false;
    }

#ifdef ISS_STANDALONE
    if (cur_req->byte_addr >= reinterpret_cast<NonBlockingMainMemory*>(bus)->getMemByteSize())
        return true;
#endif
    uint32_t addr = cur_req->byte_addr + dma_dataword_length_byte * cur_req->current_burst_iter;

    // metric counters
    auto stat = Statistics::get().getDCMAStat();
    if (!cache.isHit(addr)) {
        stat->cycle_counters.did_dma_read_miss[initiator_id] = 1;
    } else if (cache.isAccessReady(addr)) {
        stat->cycle_counters.did_dma_read_hit[initiator_id] = 1;
    } else {
        stat->cycle_counters.did_dma_read_hit_but_busy[initiator_id] = 1;
    }

    bool result = cache.isAccessReady(addr);

    if (cur_req->is_new_access) {
        cur_req->is_new_access = false;
        if (result)
            stat->counters.read_hit_access_counter++;
        else
            stat->counters.read_miss_access_counter++;
    }

    return result;
}

/**
 * interface function for dma to check if write data is ready
 * @param initiator_id cluster id
 * @return bool
 */
bool DCMA::isWriteDataReady(uint32_t initiator_id) {
    if (dcma_mode == DMA) return dcma_dma_mode.isWriteDataReady(initiator_id);
    if (dcma_mode == IDEAL) return true;
    Request* cur_req = &dmaRequests[initiator_id];
#ifdef ISS_STANDALONE
    if (cur_req->byte_addr >= reinterpret_cast<NonBlockingMainMemory*>(bus)->getMemByteSize())
        return true;
#endif

    if (cur_req->latency_wait_counter > 0) {
        cur_req->latency_wait_counter--;
        return false;
    }

    uint32_t addr = cur_req->byte_addr + dma_dataword_length_byte * cur_req->current_burst_iter;

    // metric counters
    auto stat = Statistics::get().getDCMAStat();
    if (!cache.isHit(addr)) {
        stat->cycle_counters.did_dma_write_miss[initiator_id] = 1;
    } else if (cache.isAccessReady(addr)) {
        stat->cycle_counters.did_dma_write_hit[initiator_id] = 1;
    } else {
        stat->cycle_counters.did_dma_write_hit_but_busy[initiator_id] = 1;
    }

    bool result = cache.isAccessReady(addr);

    if (cur_req->is_new_access) {
        cur_req->is_new_access = false;
        if (result)
            stat->counters.write_hit_access_counter++;
        else
            stat->counters.write_miss_access_counter++;
    }

    return result;
}

/**
 * interface function for dma to check if last dma request is done and new request can be started
 * @param initiator_id cluster id
 * @return bool
 */
bool DCMA::isBusy(uint32_t initiator_id) {
    if (dcma_mode == DMA) return dcma_dma_mode.isBusy();
    return !dmaRequests[initiator_id].is_done;
}

/**
 * calculates a clock cycle of DCMA
 */
void DCMA::tick() {
    if (dcma_mode == DMA) return dcma_dma_mode.tick();
    if (!cache.isBusy()) {
        // check if there are outstanding cache misses and request cache download
        uint32_t cur_dma_id = pointer_nxt_dma_miss;
        Request* cur_req = &dmaRequests[cur_dma_id];
        intptr_t cur_addr =
            cur_req->byte_addr + cur_req->current_burst_iter * dma_dataword_length_byte;
        if (!cur_req->is_done && !cache.isHit(cur_addr)) {
#ifdef ISS_STANDALONE
            if (cur_req->byte_addr <
                reinterpret_cast<NonBlockingMainMemory*>(bus)->getMemByteSize())
                cache.dmaRequestDownloadCacheLine(cur_addr, cur_dma_id);
#else
            cache.dmaRequestDownloadCacheLine(cur_addr, cur_dma_id);
#endif
        }

        pointer_nxt_dma_miss++;
        if (pointer_nxt_dma_miss >= number_cluster) pointer_nxt_dma_miss = 0;
    }

    cache.tick();
}

/**
 * resets cache by reseting all valid flags to false
 */
void DCMA::reset() {
    cache.reset();

    Statistics::get().getDCMAStat()->reset();
}

/**
 * flushes cache, make sure to call dma_wait_finish() before
 */
void DCMA::flush() {
    cache.flush();
}

/**
 * checks if DCMA is busy
 */
bool DCMA::isBusy() {
    if (dcma_mode == DMA) return dcma_dma_mode.isBusy();
    return cache.isBusy();
}

bool DCMA::getDCMAOff() {
    if (this->params.DCMA_OFF)
        printf("DCMA is OFF\n");
    else
        printf("DCMA is ON\n");
    return this->params.DCMA_OFF;
}

uint32_t DCMA::getNrRams() {
    return this->params.nr_brams;
}

uint32_t DCMA::getRamSize() {
    return this->params.bram_size;
}

uint32_t DCMA::getLineSize() {
    return this->params.line_size;
}

uint32_t DCMA::getAssociativity() {
    return this->params.associativity;
}
