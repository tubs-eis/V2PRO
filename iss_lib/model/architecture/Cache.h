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
// Created by Gia Bao Thieu on 04.04.2022.
//

#ifndef TEMPLATE_CACHE_H
#define TEMPLATE_CACHE_H

// C std libraries
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bitset>
#include <cmath>
#include <list>
#include <random>
#include <vector>

// ISS libraries
//#include "DCMA.h"
#include "NonBlockingBusSlaveInterface.h"
#include "bram.h"
#include "stats/Statistics.h"

#ifdef ISS_STANDALONE

#include "NonBlockingMainMemory.h"

#endif

class Cache {
   public:
    Cache(ISS* core,
        uint32_t line_size,
        uint32_t associativity,
        uint32_t nr_brams,
        uint32_t bram_size_byte,
        NonBlockingBusSlaveInterface* bus);

    void dmaReadDataHit(uint32_t addr, uint8_t* data_ptr);

    void dmaWriteDataHit(uint32_t addr, uint8_t* data_ptr);

    void dmaRequestDownloadCacheLine(uint32_t byte_addr, uint32_t initiator_id);

    void flush();

    void reset();

    bool isBusy();

    bool isAccessReady(uint32_t addr_in);

    bool isHit(intptr_t addr_in);

    void tick();

   private:
    enum ReplacementPolicy {
        FIFO,  // First In First Out
        LRU,   // Least Recently Used
        LFU,   // Least Frequently Used
        Random
    };

    // constants
    constexpr static int dma_dataword_length_byte = 16 / 8;
    constexpr static int dcma_dataword_length_byte = 128 / 8;
    constexpr static int bus_dataword_length_byte = 512 / 8;
    constexpr static uint32_t initiator_id = 0;
    ReplacementPolicy replacement_policy = FIFO;

    struct Config {
        uint32_t nr_lines;   // cache size = nr_lines * lines_size
        uint32_t line_size;  // in bytes
        uint32_t associativity;
        uint32_t total_cache_size;  // in bytes
        uint32_t nr_sets;
        uint32_t nr_brams;
        uint32_t bram_size;
        uint32_t addr_word_bitwidth;
        uint32_t addr_word_select_bitwidth;
        uint32_t addr_set_bitwidth;
    } config;

    struct Request {
        uint32_t byte_addr;
        uint32_t tag, set, word;
        uint32_t cache_line_data_ptr;  // pointer within a cache line
        uint8_t* dma_data_ptr;         // pointer to read/write data of dma
        uint32_t cache_line;           // resulting cache line calculated from byte_addr
        bool is_read;
        bool is_waiting_for_bus = false;
        bool is_waiting_for_wdata;
        bool is_done = true;
    } cur_bus_request;

    // object pointers// object pointers
    NonBlockingBusSlaveInterface* bus;
    std::vector<Bram> brams{};
    ISS* core;

    // cache memory
    //    std::vector<std::vector<uint8_t>> data_memory; // 2d: nr_lines x line_size
    std::vector<uint32_t> tag_memory;
    std::vector<bool> dirty_flags;
    std::vector<bool> valid_flags;

    // memory for cache replacement policies
    std::vector<uint32_t> replacement_memory;  // stores information for replacement algorithms

    // virtual buffer for cache line, this will not be in hardware
    std::vector<uint8_t> bus_buffer;
    uint32_t
        burst_wait_counter;  // memory gives/takes complete burst word, with this counter we wait the burst delay

    // flush
    bool flush_flag = false;
    bool flush_last = false;
    uint32_t flush_counter;

    // functions
    void splitAddr(uint32_t addr_in, uint32_t* tag, uint32_t* line, uint32_t* word);

    uint32_t mergeAddr(uint32_t cache_line, uint32_t word);

    void uploadCacheLine();

    void downloadCacheLine();

    uint32_t calcLineAlignedAddr(uint32_t addr);

    uint32_t getReplaceLine(uint32_t addr);

    uint32_t getBramIdx(uint32_t addr);

    uint32_t getBramAddr(uint32_t addr);
};

#endif  //TEMPLATE_CACHE_H
