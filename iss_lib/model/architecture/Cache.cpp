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

#include "Cache.h"

Cache::Cache(ISS* core,
    uint32_t line_size,
    uint32_t associativity,
    uint32_t nr_brams,
    uint32_t bram_size_byte,
    NonBlockingBusSlaveInterface* bus)
    : core(core) {
    this->bus = bus;

    config.line_size = line_size;  // in bytes
    config.associativity = associativity;
    config.bram_size = bram_size_byte;
    config.nr_brams = nr_brams;

    config.nr_lines = bram_size_byte * nr_brams / line_size;

    tag_memory = std::vector<uint32_t>(config.nr_lines, 0);
    dirty_flags = std::vector<bool>(config.nr_lines, false);
    valid_flags = std::vector<bool>(config.nr_lines, false);

    bus_buffer = std::vector<uint8_t>(line_size, 0);

    config.total_cache_size = bram_size_byte * nr_brams;  // in bytes
    config.nr_sets = config.total_cache_size / (line_size * associativity);

    // addr widths
    config.addr_word_bitwidth = uint32_t(std::ceil(log2(dma_dataword_length_byte)));
    config.addr_word_select_bitwidth =
        uint32_t(std::ceil(log2(config.line_size / dma_dataword_length_byte)));
    config.addr_set_bitwidth = uint32_t(std::ceil(log2(config.nr_sets)));

    // replacement policy memory init
    if (replacement_policy == ReplacementPolicy::FIFO) {
        replacement_memory = std::vector<uint32_t>(config.nr_sets, 0);
    }
    if (replacement_policy == ReplacementPolicy::LRU) {
        replacement_memory = std::vector<uint32_t>(config.nr_lines, 0);
    }
    if (replacement_policy == ReplacementPolicy::LFU) {
        replacement_memory = std::vector<uint32_t>(config.nr_lines, 0);
    }

    // init brams
    for (int i = 0; i < nr_brams; ++i) {
        brams.emplace_back(bram_size_byte, dma_dataword_length_byte);
    }
}

/**
 *
 * @param addr_in byte addr of data
 * @param tag pointer to result tag addr
 * @param set pointer to result set addr
 * @param word pointer to result word addr
 */
void Cache::splitAddr(uint32_t addr_in, uint32_t* tag, uint32_t* set, uint32_t* word) {
    // structure: ||-TAG-|-SET-|-WORD-||  {addr_in}
    // word
    uint32_t mask;
    mask = ((1 << config.addr_word_select_bitwidth) - 1) << config.addr_word_bitwidth;
    *word = (addr_in & mask) >> config.addr_word_bitwidth;

    // line
    mask = ((1 << config.addr_set_bitwidth) - 1)
        << (config.addr_word_select_bitwidth + config.addr_word_bitwidth);
    *set = (addr_in & mask) >> (config.addr_word_select_bitwidth + config.addr_word_bitwidth);

    // tag
    mask = -1 << (config.addr_set_bitwidth + config.addr_word_select_bitwidth +
                  config.addr_word_bitwidth);
    *tag = (addr_in & mask) >> (config.addr_set_bitwidth + config.addr_word_select_bitwidth +
                                   config.addr_word_bitwidth);
}

/**
 * merges cache_line and word nr for bram access
 * @param cache_line
 * @param word
 * @return byte addr for bram access
 */
uint32_t Cache::mergeAddr(uint32_t cache_line, uint32_t word) {
    return ((cache_line << config.addr_word_select_bitwidth) + word) << config.addr_word_bitwidth;
}

/**
 * check if addr is in cache or not
 * @param addr_in byte addr of requested addr
 * @return true if hit, false if miss
 */
bool Cache::isHit(intptr_t addr_in) {
    uint32_t tag, set, word;
    splitAddr(addr_in, &tag, &set, &word);

    for (int i = 0; i < config.associativity; ++i) {
        if (tag_memory[i + set * config.associativity] == tag &&
            valid_flags[i + set * config.associativity])
            return true;
    }

    return false;
}

/**
 * reads dma_dataword_length bytes of data, only possible if it is a hit
 * @param addr_in byte addr of read data
 * @param data_ptr pointer to read data variable
 */
void Cache::dmaReadDataHit(uint32_t addr_in, uint8_t* data_ptr) {
    uint32_t tag, set, word;
    splitAddr(addr_in, &tag, &set, &word);
    uint32_t dma_word_in_line = addr_in % config.line_size;
    uint32_t set_offset = set * config.associativity;

    int line;  // line within a set
    for (line = 0; line < config.associativity; ++line) {
        if (tag_memory[line + set_offset] == tag && valid_flags[line + set_offset]) break;
    }

    uint32_t cache_addr = mergeAddr(line + set_offset, word);
    uint32_t bram_idx = getBramIdx(cache_addr);
    uint32_t bram_addr = getBramAddr(cache_addr);
    brams[bram_idx].read(bram_addr, data_ptr);
    brams[bram_idx].set_accessed_this_cycle(true);

    if (replacement_policy == ReplacementPolicy::LRU) {
        // increment replacement memory of other lines in a set
        for (int i = 0; i < config.associativity; ++i) {
            if (i == line)
                replacement_memory[set_offset + line] = 0;
            else
                replacement_memory[set_offset + i]++;
        }
    } else if (replacement_policy == ReplacementPolicy::LFU) {
        replacement_memory[set_offset + line]++;
    }
}

/**
 * writes dma_dataword_length bytes of data, only possible if it is a hit
 * @param addr_in byte addr of write data
 * @param data_ptr pointer to write data variable
 */
void Cache::dmaWriteDataHit(uint32_t addr_in, uint8_t* data_ptr) {
    uint32_t tag, set, word;
    splitAddr(addr_in, &tag, &set, &word);
    uint32_t dma_word_in_line = addr_in % config.line_size;
    uint32_t set_offset = set * config.associativity;

    int line;  // line within a set
    for (line = 0; line < config.associativity; ++line) {
        if (tag_memory[line + set_offset] == tag && valid_flags[line + set_offset]) break;
    }

    uint32_t cache_addr = mergeAddr(line + set_offset, word);
    uint32_t bram_idx = getBramIdx(cache_addr);
    uint32_t bram_addr = getBramAddr(cache_addr);
    brams[bram_idx].write(bram_addr, data_ptr);
    // update bram accessed this cycle flag
    brams[bram_idx].set_accessed_this_cycle(true);

    dirty_flags[line + set_offset] = true;

    if (replacement_policy == ReplacementPolicy::LRU) {
        // increment replacement memory of other lines in a set
        for (int i = 0; i < config.associativity; ++i) {
            if (i == line)
                replacement_memory[set_offset + line] = 0;
            else
                replacement_memory[set_offset + i]++;
        }
    } else if (replacement_policy == ReplacementPolicy::LFU) {
        replacement_memory[set_offset + line]++;
    }
}

/**
 * starts a write request to the bus for uploading a cache line
 */
void Cache::uploadCacheLine() {
    uint32_t addr = uint32_t(cur_bus_request.cache_line / config.associativity) * config.line_size;
    uint32_t tag = tag_memory[cur_bus_request.cache_line];
    addr += tag << (config.addr_word_bitwidth + config.addr_word_select_bitwidth +
                    config.addr_set_bitwidth);
    uint32_t burst_length = config.line_size / bus_dataword_length_byte;

    //    dcma->requestDmaWriteTransfer(addr, burst_length, initiator_id);
    // get upload data from cache/brams
    uint32_t bram_idx;
    uint32_t bram_addr;
    uint32_t cache_addr = mergeAddr(cur_bus_request.cache_line, 0);
    for (int i = 0; i < config.line_size; i = i + dma_dataword_length_byte) {
        bram_idx = getBramIdx(cache_addr + i);
        bram_addr = getBramAddr(cache_addr + i);
        brams[bram_idx].read(bram_addr, &bus_buffer[i]);
    }

    bus->requestWriteTransfer(addr, &bus_buffer[0], burst_length, initiator_id);

    cur_bus_request.is_waiting_for_wdata = true;
    cur_bus_request.is_waiting_for_bus = true;
}

/**
 * starts a read request to the bus for downloading a cache line
 */
void Cache::downloadCacheLine() {
    intptr_t addr = calcLineAlignedAddr(cur_bus_request.byte_addr);
    uint32_t burst_length = config.line_size / bus_dataword_length_byte;

    //    dcma->requestDmaReadTransfer(addr, burst_length, initiator_id);
    bus->requestReadTransfer(addr, burst_length, initiator_id);
    cur_bus_request.is_waiting_for_bus = true;
}

/**
 *
 * @param addr byte addr of data
 * @return cache line aligned byte addr
 */
uint32_t Cache::calcLineAlignedAddr(uint32_t addr) {
    uint32_t line_size_width = uint32_t(std::ceil(log2(config.line_size)));
    uint32_t mask;
    mask = -1 << line_size_width;
    return addr & mask;
}

/**
 * start a cache line download request, only possible if cache is not busy (loading another cache line)
 * @param byte_addr byte addr of requested data
 * @param initiator_id cluster id
 */
void Cache::dmaRequestDownloadCacheLine(uint32_t byte_addr, uint32_t initiator_id) {
    cur_bus_request.byte_addr = calcLineAlignedAddr(byte_addr);
    //    cur_bus_request.is_read = false;
    cur_bus_request.is_waiting_for_bus = false;
    cur_bus_request.is_waiting_for_wdata = false;
    cur_bus_request.is_done = false;
    cur_bus_request.cache_line_data_ptr = 0;
}

/**
 * checks if cache is busy down/uploading data
 * @return bool busy
 */
bool Cache::isBusy() {
    return !cur_bus_request.is_done;
}

/**
 * checks if is hit and if bram was not accessed in this cycle
 * @param addr_in byte addr of data
 * @return bool ready
 */
bool Cache::isAccessReady(uint32_t addr_in) {
    if (isHit(addr_in)) {
        uint32_t bram_idx = getBramIdx(addr_in);
        return !brams[bram_idx].get_accessed_this_cycle();
    }
    return false;
}

/**
 * simulates a cache cycle
 */
void Cache::tick() {
    // reset bram accessed this cycle flag
    for (Bram& bram : brams)
        bram.set_accessed_this_cycle(false);

    if (flush_flag) {
        if (!cur_bus_request.is_waiting_for_bus) {
            uint32_t cache_ext_addr;
            uint32_t cache_bram_addr;
            bool found_dirty = false;
            while (!found_dirty && !flush_last) {
                if (dirty_flags[flush_counter]) {
                    found_dirty = true;
                    cache_ext_addr =
                        (uint32_t(flush_counter / config.associativity) * config.associativity) *
                        config.line_size;
                    cache_bram_addr = flush_counter * config.line_size;
                }

                flush_counter++;
                if (flush_counter == config.nr_lines) {
                    flush_last = true;
                    if (!found_dirty) {
                        cur_bus_request.is_done = true;
                        flush_flag = false;
                    }
                }
            }
            if (found_dirty) {
                uint32_t burst_length = config.line_size / bus_dataword_length_byte;

                // get upload data from cache/brams
                uint32_t bram_idx;
                uint32_t bram_addr;
                for (int i = 0; i < config.line_size; i = i + dma_dataword_length_byte) {
                    bram_idx = getBramIdx(cache_bram_addr + i);
                    bram_addr = getBramAddr(cache_bram_addr + i);
                    brams[bram_idx].read(bram_addr, &bus_buffer[i]);
                }

                uint32_t tag = tag_memory[flush_counter - 1];
                uint32_t ext_addr =
                    cache_ext_addr / config.associativity +
                    (tag << (config.addr_word_bitwidth + config.addr_word_select_bitwidth +
                             config.addr_set_bitwidth));
                bus->requestWriteTransfer(ext_addr, &bus_buffer[0], burst_length, initiator_id);

                cur_bus_request.is_waiting_for_wdata = true;
                cur_bus_request.is_waiting_for_bus = true;
            }

        } else {
            if (burst_wait_counter > 0) {
                burst_wait_counter--;
                if (burst_wait_counter == 0) {
                    cur_bus_request.is_waiting_for_bus = false;
                    if (flush_last) {
                        flush_flag = false;
                        cur_bus_request.is_done = true;
                    }
                }
            } else if (bus->isWriteDataReady(initiator_id)) {
                burst_wait_counter = config.line_size / bus_dataword_length_byte;
                Statistics::get().getDCMAStat()->counters.bus_write_cycles += burst_wait_counter;
            }
        }
    }
    // if cur request is not processed yet, start new bus transfer
    else if (!cur_bus_request.is_done && !cur_bus_request.is_waiting_for_bus &&
             !cur_bus_request.is_waiting_for_wdata) {
        // get corresponding cache line according to addr
        cur_bus_request.cache_line = getReplaceLine(cur_bus_request.byte_addr);
        splitAddr(cur_bus_request.byte_addr,
            &cur_bus_request.tag,
            &cur_bus_request.set,
            &cur_bus_request.word);
        valid_flags[cur_bus_request.cache_line] = false;

        // check if overwritten block is dirty
        if (dirty_flags[cur_bus_request.cache_line]) {
            uploadCacheLine();
        } else {
            downloadCacheLine();
        }

        dirty_flags[cur_bus_request.cache_line] = false;
    }
    // if cur request is ongoing, check if read/write data can be read/written
    else if (cur_bus_request.is_waiting_for_bus) {
        if (burst_wait_counter > 0) {
            burst_wait_counter--;
            if (burst_wait_counter == 0) {
                if (!cur_bus_request.is_waiting_for_wdata) {
                    // download finished
                    cur_bus_request.is_done = true;
                    cur_bus_request.is_waiting_for_bus = false;
                    valid_flags[cur_bus_request.cache_line] = true;
                    tag_memory[cur_bus_request.cache_line] = cur_bus_request.tag;
                } else {
                    // upload of dirty cache line finished
                    cur_bus_request.is_waiting_for_wdata = false;
                    dirty_flags[cur_bus_request.cache_line] = false;
                    // download new cache line
                    downloadCacheLine();
                }
            }
        } else {
            Statistics::get().getDCMAStat()->counters.bus_wait_cycles++;
            if (!cur_bus_request.is_waiting_for_wdata) {
                if (bus->isReadDataAvailable(initiator_id)) {
                    // read complete cache line from bus to buffer
                    bus->readData(&bus_buffer[0], initiator_id);

                    // write bus data from buffer to cache
                    uint32_t bram_idx;
                    uint32_t bram_addr;
                    for (int i = 0; i < config.line_size; i = i + dma_dataword_length_byte) {
                        bram_idx = getBramIdx(
                            mergeAddr(cur_bus_request.cache_line, i / dma_dataword_length_byte));
                        bram_addr = getBramAddr(
                            mergeAddr(cur_bus_request.cache_line, i / dma_dataword_length_byte));
                        brams[bram_idx].write(bram_addr, &bus_buffer[i]);
                    }

                    burst_wait_counter = config.line_size / bus_dataword_length_byte;

                    Statistics::get().getDCMAStat()->counters.bus_read_cycles += burst_wait_counter;
                    Statistics::get().getDCMAStat()->counters.bus_wait_cycles--;
                }
            } else {
                if (bus->isWriteDataReady(initiator_id)) {
                    burst_wait_counter = config.line_size / bus_dataword_length_byte;
                    Statistics::get().getDCMAStat()->counters.bus_write_cycles +=
                        burst_wait_counter;
                    Statistics::get().getDCMAStat()->counters.bus_wait_cycles--;
                }
            }
        }
    }
}

/**
 *
 * @param addr byte addr of accessed data
 * @return cache line nr
 */
uint32_t Cache::getReplaceLine(uint32_t addr) {
    uint32_t tag, set, word;
    splitAddr(addr, &tag, &set, &word);

    uint32_t set_offset = set * config.associativity;

    // if there is an unused block, return that
    for (int i = 0; i < config.associativity; ++i) {
        if (!valid_flags[set_offset + i]) return set_offset + i;
    }

    // else use replacement policy
    if (replacement_policy == ReplacementPolicy::FIFO) {
        uint32_t cur_pointer = replacement_memory[set];

        if (cur_pointer == config.associativity - 1)
            replacement_memory[set] = 0;
        else
            replacement_memory[set]++;

        return set_offset + cur_pointer;
    } else if (replacement_policy == ReplacementPolicy::LRU) {
        // get least recently used cache line in a set
        uint32_t lru_line = 0;
        for (int i = 1; i < config.associativity; ++i) {
            if (replacement_memory[set_offset + i] > replacement_memory[set_offset + lru_line])
                lru_line = i;
        }

        // increment replacement memory of other lines in a set
        for (int i = 0; i < config.associativity; ++i) {
            if (i == lru_line)
                replacement_memory[set_offset + lru_line] = 0;
            else
                replacement_memory[set_offset + i]++;
        }

        return set_offset + lru_line;
    } else if (replacement_policy == ReplacementPolicy::LFU) {
        // get least frequently used cache line in a set
        uint32_t lfu_line = 0;
        for (int i = 1; i < config.associativity; ++i) {
            if (replacement_memory[set_offset + i] < replacement_memory[set_offset + lfu_line])
                lfu_line = i;
        }

        replacement_memory[set_offset + lfu_line] = 0;

        return set_offset + lfu_line;
    } else if (replacement_policy == ReplacementPolicy::Random) {
        std::random_device rd;   // obtain a random number from hardware
        std::mt19937 gen(rd());  // seed the generator
        std::uniform_int_distribution<> distr(0, config.associativity - 1);  // define the range
        return set_offset + distr(gen);
    }
    return -1;
}

/**
 * get number of bram from addr
 * @param addr byte addr of data
 * @return bram index
 */
uint32_t Cache::getBramIdx(uint32_t addr) {
    return (addr / dcma_dataword_length_byte) % config.nr_brams;
}

/**
 * get addr within the accessed bram from byte addr
 * @param addr byte addr of data
 * @return bram addr
 */
uint32_t Cache::getBramAddr(uint32_t addr) {
    return (addr / dcma_dataword_length_byte) / config.nr_brams * dcma_dataword_length_byte /
               dma_dataword_length_byte +
           (addr / dma_dataword_length_byte) %
               (dcma_dataword_length_byte / dma_dataword_length_byte);
}

/**
 * resets cache by reseting all valid flags to false
 */
void Cache::reset() {
    valid_flags = std::vector<bool>(config.nr_lines, false);
    dirty_flags = std::vector<bool>(config.nr_lines, false);
    tag_memory = std::vector<uint32_t>(config.nr_lines, 0);

    // replacement policy memory init
    if (replacement_policy == ReplacementPolicy::FIFO) {
        replacement_memory = std::vector<uint32_t>(config.nr_sets, 0);
    }
    if (replacement_policy == ReplacementPolicy::LRU) {
        replacement_memory = std::vector<uint32_t>(config.nr_lines, 0);
    }
    if (replacement_policy == ReplacementPolicy::LFU) {
        replacement_memory = std::vector<uint32_t>(config.nr_lines, 0);
    }
}

/**
 * flushes cache
 */
void Cache::flush() {
    flush_counter = 0;
    flush_flag = true;
    flush_last = false;
    cur_bus_request.is_done = false;
}
