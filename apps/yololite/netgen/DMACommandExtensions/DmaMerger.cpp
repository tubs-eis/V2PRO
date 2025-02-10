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
// Created by gesper on 13.04.23.
//

#include "DmaMerger.h"

void DmaMerger::merge_same_commands(){
    auto cmd_lst = new_list.begin();
    for (auto cmd_it = ++new_list.begin(); cmd_it != new_list.end(); cmd_it++) {
        bool equal = true;
        equal &= (cmd_lst->dma.direction == cmd_it->dma.direction);
        equal &= (cmd_lst->dma.isBiasOffset == cmd_it->dma.isBiasOffset);
        equal &= (cmd_lst->dma.isKernelOffset == cmd_it->dma.isKernelOffset);
        equal &= (cmd_lst->dma.cluster == cmd_it->dma.cluster);
        equal &= (cmd_lst->dma.unit_mask == cmd_it->dma.unit_mask);
        equal &= (cmd_lst->dma.mm_addr == cmd_it->dma.mm_addr);
        equal &= (cmd_lst->dma.lm_addr == cmd_it->dma.lm_addr);
        equal &= (cmd_lst->dma.y_leap == cmd_it->dma.y_leap);
        equal &= (cmd_lst->dma.x_size == cmd_it->dma.x_size);
        equal &= (cmd_lst->dma.y_size == cmd_it->dma.y_size);
        equal &= (cmd_lst->dma.padding == cmd_it->dma.padding);

        if (equal) { // skip this cmd
            printf("\e[41mSKIPPED DMA Cmd (twice the same!)... \e[0m\n");
            printf("CMD: %s\n", cmd_it->dma.to_char());
            cmd_it = new_list.erase(cmd_it); // remove from list (erase will update iterator to next element)
            --cmd_it; // correct iterator
        }
        cmd_lst = cmd_it;
    }
}

void DmaMerger::transform_2d_to_1d(){
    int merge_2d_to_1d = 0;
    for (auto &cmd: new_list) {
        if (cmd.type.type == DMA_CMD) {  // DMA
            if (cmd.dma.direction == e2l2D || cmd.dma.direction == l2e2D) {
                if (cmd.dma.y_size > 1 && cmd.dma.y_leap == 1) {    // 2D transfer, merge into 1D transfer possible (stride == 1)
                    if (!cmd.dma.padding) { // no padding
                        uint32_t dma_elements = cmd.dma.x_size * cmd.dma.y_size;
                        if (dma_elements <= 8192) {
                            //                    printf("2D->1D Merge: %s \n", cmd.dma.to_char());
                            cmd.dma.y_size = 1;
                            cmd.dma.x_size = dma_elements;
                            if (cmd.dma.direction == e2l2D)
                                cmd.dma.direction = e2l1D;
                            else
                                cmd.dma.direction = l2e1D;
                            merge_2d_to_1d++;
                            //                    printf("=>            %s\n", cmd.dma.to_char());
                        }
                    }
                } else if (cmd.dma.y_size == 1) {   // this is already a 1D transfer but marked as 2D...
                    if (cmd.dma.direction == e2l2D)
                        cmd.dma.direction = e2l1D;
                    else
                        cmd.dma.direction = l2e1D;
                }
            }
        }
    }
    if (merge_2d_to_1d > 0 && debug) {
        printf("   [DMA Merge] \e[96mMerged 2D -> 1D: %d Commands\e[0m\n", merge_2d_to_1d);
    }
}

void DmaMerger::reduce_1d_by_overcalc(){
    for (auto & cmd : new_list) {
        if (cmd.type.type == DMA_CMD) { // DMA
            if (cmd.dma.direction == e2l1D || cmd.dma.direction == l2e1D) { // 1D
                cmd.dma.x_size -= cmd.dma.skipped_elements_at_end;
                cmd.dma.skipped_elements_at_end = 0;      // just once!
            } else {  // 2D
                if (cmd.dma.skipped_elements_at_end != 0) {
                    printf("2D Command with skipped elements at end!!!!!!!\n");
                    assert(false);
                }
            }
        }
    }
}

void DmaMerger::merge_sequential_1ds(){
    auto cmd_lst = new_list.begin();
    for (auto cmd_it = ++new_list.begin(); cmd_it != new_list.end(); cmd_it++) {
        // only 1D transfers
        if (cmd_it->dma.direction == e2l1D || cmd_it->dma.direction == l2e1D) {
            bool equal = true;
            equal &= (cmd_lst->dma.direction == cmd_it->dma.direction);
            equal &= (cmd_lst->dma.isBiasOffset == cmd_it->dma.isBiasOffset);
            equal &= (cmd_lst->dma.isKernelOffset == cmd_it->dma.isKernelOffset);
            equal &= (cmd_lst->dma.cluster == cmd_it->dma.cluster);
            equal &= (cmd_lst->dma.unit_mask == cmd_it->dma.unit_mask);

            if (equal) {
                // 1D concat memory regions?
                // it follows lst
                int size = cmd_lst->dma.x_size;
                if (cmd_lst->dma.mm_addr + size * 2 == cmd_it->dma.mm_addr) {
                    if (cmd_lst->dma.lm_addr + size == cmd_it->dma.lm_addr) {
//                        printf("\e[96m 1D MERGEABLE Commands!\e[0m\n");
//                        printf("\e[96m     %s \e[0m\n", cmd_it->dma.to_char());
//                        printf("\e[96m     %s \e[0m\n", cmd_lst->dma.to_char());
                        cmd_lst->dma.x_size += cmd_it->dma.x_size;
                        cmd_it = new_list.erase(cmd_it); // remove from list (erase will update iterator to next element)
                        --cmd_it; // correct iterator
                        dma_1d_merges++;
                        continue;
                    }
                }
                // lst follows it
                size = cmd_it->dma.x_size;
                if (cmd_lst->dma.mm_addr == cmd_it->dma.mm_addr + size * 2) {
                    if (cmd_lst->dma.lm_addr == cmd_it->dma.lm_addr + size) {
//                        printf("\e[96m 1D MERGEABLE Commands!\e[0m\n");
//                        printf("\e[96m     %s \e[0m\n", cmd_it->dma.to_char());
//                        printf("\e[96m     %s \e[0m\n", cmd_lst->dma.to_char());
                        cmd_lst->dma.x_size += cmd_it->dma.x_size;
                        cmd_lst->dma.mm_addr = cmd_it->dma.mm_addr;
                        cmd_lst->dma.lm_addr = cmd_it->dma.lm_addr;
                        cmd_it = new_list.erase(cmd_it); // remove from list (erase will update iterator to next element)
                        --cmd_it; // correct iterator
                        dma_1d_merges++;
                        continue;
                    }
                }
            }
        }
        cmd_lst = cmd_it;
    }
}

void DmaMerger::merge_sequential_2ds() {
    auto cmd_lst = new_list.begin();
    for (auto cmd_it = ++new_list.begin(); cmd_it != new_list.end(); cmd_it++) {
        if (cmd_it->dma.direction == e2l2D || cmd_it->dma.direction == l2e2D) {
            bool equal = true;
            equal &= (cmd_lst->dma.direction == cmd_it->dma.direction);
            equal &= (cmd_lst->dma.isBiasOffset == cmd_it->dma.isBiasOffset);
            equal &= (cmd_lst->dma.isKernelOffset == cmd_it->dma.isKernelOffset);
            equal &= (cmd_lst->dma.cluster == cmd_it->dma.cluster);
            equal &= (cmd_lst->dma.unit_mask == cmd_it->dma.unit_mask);
            equal &= (cmd_lst->dma.y_leap == cmd_it->dma.y_leap);
            equal &= (cmd_lst->dma.x_size == cmd_it->dma.x_size);

            if (equal) {
                // 2D concat memory regions?
                int mm_size = (cmd_lst->dma.x_size + cmd_lst->dma.y_leap - 1) * cmd_lst->dma.y_size;
                int lm_size = cmd_lst->dma.x_size * cmd_lst->dma.y_size;
                if (cmd_lst->dma.mm_addr + mm_size * 2 == cmd_it->dma.mm_addr) {
                    if (cmd_lst->dma.lm_addr + lm_size == cmd_it->dma.lm_addr) {
//                        printf("\e[96m 2D MERGEABLE Commands!\e[0m\n");
//                        printf("\e[96m     %s \e[0m\n", cmd_it->dma.to_char());
//                        printf("\e[96m     %s \e[0m\n", cmd_lst->dma.to_char());
                        cmd_lst->dma.y_size += cmd_it->dma.y_size;
                        cmd_it = new_list.erase(cmd_it); // remove from list (erase will update iterator to next element)
                        --cmd_it; // correct iterator
                        dma_2d_merges++;
                        continue;
                    }
                }
                mm_size = (cmd_it->dma.x_size + cmd_it->dma.y_leap - 1) * cmd_it->dma.y_size;
                lm_size = cmd_it->dma.x_size * cmd_it->dma.y_size;
                if (cmd_lst->dma.mm_addr == cmd_it->dma.mm_addr + mm_size * 2) {
                    if (cmd_lst->dma.lm_addr == cmd_it->dma.lm_addr + lm_size) {
//                        printf("\e[96m 2D MERGEABLE Commands!\e[0m\n");
//                        printf("\e[96m     %s \e[0m\n", cmd_it->dma.to_char());
//                        printf("\e[96m     %s \e[0m\n", cmd_lst->dma.to_char());
                        cmd_lst->dma.y_size += cmd_it->dma.y_size;
                        cmd_lst->dma.mm_addr = cmd_it->dma.mm_addr;
                        cmd_lst->dma.lm_addr = cmd_it->dma.lm_addr;
                        cmd_it = new_list.erase(cmd_it); // remove from list (erase will update iterator to next element)
                        --cmd_it; // correct iterator
                        dma_2d_merges++;
                        continue;
                    }
                }
            }
        }
        cmd_lst = cmd_it;
    }
}

uint64_t DmaMerger::count_dma_transfers(){
    uint64_t count = 0;
    for (const auto &cmd: new_list) {
        if (cmd.type.type == COMMAND_SEGMENT_TYPE::DMA_CMD){
            auto clusters = std::bitset<32>(cmd.dma.cluster).count();
            auto units = std::bitset<32>(cmd.dma.unit_mask).count();
            count += cmd.dma.x_size * cmd.dma.y_size * clusters * units;
        }
    }
    return count;
}

std::vector<BIF::COMMAND_SEGMENT> DmaMerger::generate(bool debug) {
    this->debug = debug;

    new_list = std::list<BIF::COMMAND_SEGMENT>(cmd_list.begin(), cmd_list.end());

    auto initial_transfers = count_dma_transfers();

    auto sortfunction = [](const BIF::COMMAND_SEGMENT &a, const BIF::COMMAND_SEGMENT &b) -> bool{
        // direction hold 4 states (but '1 bit is ~ dir)
        assert((a.dma.direction & 0b10) == (b.dma.direction & 0b10));
        // type identical
        assert(a.type.type == b.type.type);

        if (a.dma.cluster != b.dma.cluster) return a.dma.cluster < b.dma.cluster; // sort by cluster
        if (a.dma.unit_mask != b.dma.unit_mask) return a.dma.unit_mask < b.dma.unit_mask; // sort by local memory address
        if (a.dma.mm_addr != b.dma.mm_addr) return a.dma.mm_addr < b.dma.mm_addr; // sort by main memory address
        if (a.dma.lm_addr != b.dma.lm_addr) return a.dma.lm_addr < b.dma.lm_addr; // sort by local memory address
        return false; // sort by cluster
    };
    reorderDMAs(sortfunction);

    merge_same_commands();
    transform_2d_to_1d();
    reduce_1d_by_overcalc();
    merge_sequential_1ds();
    merge_sequential_2ds();

    auto sortfunction2 = [](const BIF::COMMAND_SEGMENT &a, const BIF::COMMAND_SEGMENT &b) -> bool{
        // direction hold 4 states (but '1 bit is ~ dir)
        assert((a.dma.direction & 0b10) == (b.dma.direction & 0b10));
        // type identical
        assert(a.type.type == b.type.type);

        if (a.dma.cluster != b.dma.cluster) return a.dma.cluster < b.dma.cluster; // sort by cluster
        if (a.dma.unit_mask != b.dma.unit_mask) return a.dma.unit_mask < b.dma.unit_mask; // sort by local memory address
        if (a.dma.lm_addr != b.dma.lm_addr) return a.dma.lm_addr < b.dma.lm_addr; // sort by local memory address
        if (a.dma.mm_addr != b.dma.mm_addr) return a.dma.mm_addr < b.dma.mm_addr; // sort by main memory address
        return false; // sort by cluster
    };
    reorderDMAs(sortfunction2);

    merge_same_commands();
    transform_2d_to_1d();
    merge_sequential_1ds();
    merge_sequential_2ds();

    if (dma_1d_merges > 0 || dma_2d_merges > 0)
        printf("   [DMA Merge] \e[96mConcat Regions in sequential DMA Transfers: Merged %i (1D), %i (2D) | -%.2f%% Commands\e[0m\n", dma_1d_merges, dma_2d_merges,
               100*float(dma_1d_merges+dma_2d_merges)/(new_list.size()+dma_1d_merges+dma_2d_merges));

    auto final_transfers = count_dma_transfers();
    if (final_transfers != initial_transfers){
        printf("   [DMA Merge] \e[91mError: Total count of dma transfers differ! Initial %lu elements, Final %lu elements\e[0m\n", initial_transfers, final_transfers);
    }

    auto sortfunction3 = [](const BIF::COMMAND_SEGMENT &a, const BIF::COMMAND_SEGMENT &b) -> bool{
        // direction hold 4 states (but '1 bit is ~ dir)
        assert((a.dma.direction & 0b10) == (b.dma.direction & 0b10));
        // type identical
        assert(a.type.type == b.type.type);

//        if (a.dma.direction == l2e1D || a.dma.direction == l2e2D){
//            // to loop l2e store commands ;) -> slower!?!! -> better to start cmds in all clusters at the same time instead all '1 first
//            if (a.dma.cluster != b.dma.cluster) return a.dma.cluster < b.dma.cluster; // sort by cluster
//            if (a.dma.unit_mask != b.dma.unit_mask) return a.dma.unit_mask < b.dma.unit_mask; // sort by local memory address
//            if (a.dma.mm_addr != b.dma.mm_addr) return a.dma.mm_addr < b.dma.mm_addr; // sort by main memory address
//            if (a.dma.lm_addr != b.dma.lm_addr) return a.dma.lm_addr < b.dma.lm_addr; // sort by local memory address
//        }

        if (a.dma.mm_addr != b.dma.mm_addr) return a.dma.mm_addr < b.dma.mm_addr; // sort by main memory address
        if (a.dma.lm_addr != b.dma.lm_addr) return a.dma.lm_addr < b.dma.lm_addr; // sort by local memory address
        if (a.dma.unit_mask != b.dma.unit_mask) return a.dma.unit_mask < b.dma.unit_mask; // sort by local memory address
        if (a.dma.cluster != b.dma.cluster) return a.dma.cluster < b.dma.cluster; // sort by cluster
        return false; // sort by cluster
    };
    reorderDMAs(sortfunction3); // for looper

    return std::vector<BIF::COMMAND_SEGMENT>(new_list.begin(), new_list.end());
}

void DmaMerger::reorderDMAs(const std::function<bool(const BIF::COMMAND_SEGMENT &, const BIF::COMMAND_SEGMENT &)>& sortfunction) {
    // sub list to be sorted
    auto start = new_list.begin();
    auto end = new_list.begin();

    bool has_commands = false;
    DMA_DIRECTION dir;
    bool isFinish = false;

    for (auto list_iterator = new_list.begin(); list_iterator != new_list.end(); ++list_iterator){
        if (list_iterator->type.type == COMMAND_SEGMENT_TYPE::DMA_CMD){
            if (!has_commands){ // First command in block
                dir = list_iterator->dma.direction;
                start = list_iterator;
                end = list_iterator;
            } else {    // filled block
                if (dir != list_iterator->dma.direction){   // direction change
                    isFinish = true;
                } else {    // continue to fill block
                    end = list_iterator;
                }
            }
            has_commands = true;
        } else {    // no DMA command
            if (has_commands) isFinish = true;
        }

        if (isFinish){
            std::list<BIF::COMMAND_SEGMENT> temp;
            temp.splice(temp.end(), new_list, start, end);  // move to temp list
            temp.sort(sortfunction);
            --list_iterator;    // start again with this command
            new_list.splice(list_iterator, temp, temp.begin(), temp.end()); // push before iterator

            isFinish = false;
            has_commands = false;
        }
    }

    // final block should be sync... no sort required
    assert(has_commands == false);
    auto dma_block_list = std::list<BIF::COMMAND_SEGMENT>(new_list.begin(), new_list.end());
}
