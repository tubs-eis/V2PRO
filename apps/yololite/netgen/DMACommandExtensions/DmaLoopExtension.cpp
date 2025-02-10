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
// Created by gesper on 03.04.23.
//

#include "DmaLoopExtension.h"

std::vector<BIF::COMMAND_SEGMENT> DmaLoopExtension::generate(bool debug) {
    total_loop_encoded_dmas = 0;
    total_dmas = 0;
    total_loops = 0;

    auto input_list_size = cmd_list.size();

    std::list<BIF::COMMAND_SEGMENT> new_list;

    auto begin = cmd_list.begin();
    auto end = begin;
    end++;

    auto cmp_func = [](const BIF::COMMAND_SEGMENT &d1, const BIF::COMMAND_SEGMENT &d2) -> bool {
        if (d2.type.type != DMA_CMD) return false;
        if (d1.type.type != DMA_CMD) return false;

//        printf("     [DMA LOOP gen|cmp] \n");
//        printf("           D1: %s\n", d1.to_char());
//        printf("           D2: %s\n", d2.to_char());

        return (d1.dma.direction == d2.dma.direction &&
                d1.dma.isBiasOffset == d2.dma.isBiasOffset &&
                d1.dma.isKernelOffset == d2.dma.isKernelOffset &&
                d1.dma.x_size == d2.dma.x_size &&
                d1.dma.y_leap == d2.dma.y_leap &&
                d1.dma.y_size == d2.dma.y_size &&
                d1.dma.padding == d2.dma.padding);
    };
    bool is_dma_cmd_loop = false;
    while (end != cmd_list.end()) {
        if (!is_dma_cmd_loop) {  // begin + end are not part of a loop
            if (cmp_func(*begin, *end)) {    // begin + end are loopable
                if (debug){
                    printf("  [DMA LOOP gen] Loop begin\n");
                    printf("         Base: %s\n", begin->to_char());
                    printf("         +1  : %s\n", end->to_char());
                }
                is_dma_cmd_loop = true;
            } else {    // begin + end are not loopable
                if (debug)
                    printf("  [DMA LOOP gen] Skip (shortcut to list) \e[2m %s \e[0m\n", begin->to_char());
                total_dmas += 1;
                new_list.push_back(*begin);
                begin = end;
            }
        } else { // begin + previous end are loopable
            if (cmp_func(*begin, *end)) { // loop can continue
                if (debug) {
                    printf("  [DMA LOOP gen] Continue\n");
                    printf("         +1  : %s\n", end->to_char());
                }
                // nothing, increment end at end
            } else {    // loop finish. extract parameters + push
                if (debug)
                    printf("  [DMA LOOP gen] Finish (new begin: %s)\n", end->to_char());
                new_list.splice(new_list.end(), extractLoopCommand(std::list<BIF::COMMAND_SEGMENT>(begin, end), debug));
                begin = end;
                is_dma_cmd_loop = false;
            }
        }
        end++;
    }
    if (is_dma_cmd_loop) {
        new_list.splice(new_list.end(), extractLoopCommand(std::list<BIF::COMMAND_SEGMENT>(begin, --end), debug));
    } else {
        total_dmas += 1;
        new_list.push_back(*begin);
    }

    auto &newer_list = new_list;
//    auto sortfunction = [](const BIF::COMMAND_SEGMENT &a, const BIF::COMMAND_SEGMENT &b) -> bool{
//        // direction hold 4 states (but '1 bit is ~ dir)
//        assert((a.dma.direction & 0b10) == (b.dma.direction & 0b10));
//        // type identical
//        assert(a.type.type == b.type.type);
//
//        if (a.dma.cluster != b.dma.cluster) return a.dma.cluster < b.dma.cluster; // sort by cluster
//        if (a.dma.unit_mask != b.dma.unit_mask) return a.dma.unit_mask < b.dma.unit_mask; // sort by local memory address
//        if (a.dma.mm_addr != b.dma.mm_addr) return a.dma.mm_addr < b.dma.mm_addr; // sort by main memory address
//        if (a.dma.lm_addr != b.dma.lm_addr) return a.dma.lm_addr < b.dma.lm_addr; // sort by local memory address
//        return false; // sort by cluster
//    };
//
//    // sub list to be sorted
//    auto sort_start = new_list.begin();
//    auto sort_end = new_list.begin();
//    bool has_commands = false;
//    DMA_DIRECTION dir;
//    bool isFinish = false;
//    int skip_next = 0;
//    for (auto list_iterator = new_list.begin(); list_iterator != new_list.end(); ++list_iterator){
//        if(skip_next > 0){
//            skip_next--;
//            continue;
//        }
//        if (list_iterator->type.type == COMMAND_SEGMENT_TYPE::DMA_CMD && list_iterator->dma.direction != loop){
//            if (!has_commands){ // First command in block
//                dir = list_iterator->dma.direction;
//                sort_start = list_iterator;
//                sort_end = list_iterator;
//            } else {    // filled block
//                if (dir != list_iterator->dma.direction){   // direction change
//                    isFinish = true;
//                } else {    // continue to fill block
//                    sort_end = list_iterator;
//                }
//            }
//            has_commands = true;
//        } else {    // no DMA command
//            if (list_iterator->type.type == COMMAND_SEGMENT_TYPE::DMA_CMD && list_iterator->dma.direction == loop){
//                skip_next = 2;
//            }
//            if (has_commands) isFinish = true;
//        }
//
//        if (isFinish){
//            std::list<BIF::COMMAND_SEGMENT> temp;
//            temp.splice(temp.end(), new_list, sort_start, sort_end);  // move to temp list
//            temp.sort(sortfunction);
//            --list_iterator;    // start again with this command
//            new_list.splice(list_iterator, temp, temp.begin(), temp.end()); // push before iterator
//
//            isFinish = false;
//            has_commands = false;
//        }
//    }
//    if (has_commands){
//        std::list<BIF::COMMAND_SEGMENT> temp;
//        temp.splice(temp.end(), new_list, sort_start, sort_end);  // move to temp list
//        temp.sort(sortfunction);
//        new_list.splice(--new_list.end(), temp, temp.begin(), temp.end()); // push before iterator
//        isFinish = false;
//        has_commands = false;
//    }
//
//    std::list<BIF::COMMAND_SEGMENT> newer_list;
//    auto begin2 = new_list.begin();
//    auto end2 = begin2;
//    end2++;
//    is_dma_cmd_loop = false;
//    while (end2 != new_list.end()) {
//        if (!is_dma_cmd_loop) {  // begin + end are not part of a loop
//            if (cmp_func(*begin2, *end2) && end2->dma.direction != loop) {    // begin + end are loopable
//                if (debug){
//                    printf("  [DMA LOOP gen] Loop begin\n");
//                    printf("         Base: %s\n", begin2->to_char());
//                    printf("         +1  : %s\n", end2->to_char());
//                }
//                is_dma_cmd_loop = true;
//            } else {    // begin + end are not loopable
//                if (debug)
//                    printf("  [DMA LOOP gen] Skip (shortcut to list) \e[2m %s \e[0m\n", begin2->to_char());
//                total_dmas += 1;
//                newer_list.push_back(*begin2);
//                begin2 = end2;
//                if (end2->dma.direction == loop){
//                    newer_list.push_back(*end2);
//                    end2++;
//                    newer_list.push_back(*end2);
//                    end2++;
//                    begin2 = end2;
//                }
//            }
//        } else { // begin + previous end are loopable
//            if (cmp_func(*begin2, *end2) && end2->dma.direction != loop) { // loop can continue
//                if (debug) {
//                    printf("  [DMA LOOP gen] Continue\n");
//                    printf("         +1  : %s\n", end->to_char());
//                }
//                // nothing, increment end at end
//            } else {    // loop finish. extract parameters + push
//                if (debug)
//                    printf("  [DMA LOOP gen] Finish (new begin: %s)\n", end2->to_char());
//                newer_list.splice(newer_list.end(), extractLoopCommand(std::list<BIF::COMMAND_SEGMENT>(begin2, end2), debug));
//                begin2 = end2;
//                is_dma_cmd_loop = false;
//                if (end2->dma.direction == loop){
//                    newer_list.push_back(*end2);
//                    end2++;
//                    newer_list.push_back(*end2);
//                    end2++;
//                    begin2 = end2;
//                }
//            }
//        }
//        end2++;
//    }
//    if (is_dma_cmd_loop) {
//        newer_list.splice(newer_list.end(), extractLoopCommand(std::list<BIF::COMMAND_SEGMENT>(begin2, --end2), debug));
//    } else {
//        total_dmas += 1;
//        newer_list.push_back(*begin2);
//    }

    uint32_t original_byte = 32*cmd_list.size();
    uint32_t new_byte = 32*newer_list.size();
    uint32_t reduced_byte = original_byte - new_byte;

    printf("  [DMA CMD LOOP] Direct DMA commands remaining (no loop): %i, Loop encoded DMA commands: %i, number of DMA Loops: %i (+VPRO/Syncs)\n", total_dmas, total_loop_encoded_dmas, total_loops);
    printf("      %lu Commands in total (-%u Commands by Loop; -%2.2f%%)\n", newer_list.size(), reduced_byte/32, 100-float(new_byte)/float(original_byte)*100);
    if (debug){
        printf("      Size reduced from %i B to %i B (- %i B)\n", original_byte, new_byte, reduced_byte);
    }

    /**
     * The DMA Blocks still have sizes of the plain DMA commands (without loop)
     * correct this now
     */
    BIF::COMMAND_SEGMENT *block = NULL;
    bool counting_block_cmds = false;
    uint loop_encoded_dmas = 0;
    uint loop_count = 0;
    for(auto &cmd : newer_list){
        if (cmd.type.type == DMA_BLOCK){
            block = &cmd;
            counting_block_cmds = true;
            block->dma.unit_mask = 0;
            continue;
        }
        if (counting_block_cmds){
            if (cmd.type.type == DMA_CMD){
                block->dma.unit_mask++;
                if (cmd.dma.direction == loop){
                    loop_encoded_dmas += cmd.dma_loop.dma_cmd_count;
                    loop_count++;
                }
            } else {
                counting_block_cmds = false;
            }
        }
    }

    auto output_list_size_unrolled = newer_list.size() - 2 * loop_count + loop_encoded_dmas;
    if (input_list_size != output_list_size_unrolled){
        printf("\e[91m[Error] Loop List Total Length differs when unrolling all Loops!\e[0m\n");
        printf("    [DMA LOOP] Loops: %u, Loop Encoded DMAs: %u\n", loop_count, loop_encoded_dmas);
        printf("    [DMA LOOP] Output List Total: %lu\n", newer_list.size());
        printf("    [DMA LOOP] Output List Total (old count with individual DMAs): %lu\n", newer_list.size() - 2 * loop_count + loop_encoded_dmas);
        printf("    [DMA LOOP] Input List Total: %lu\n", input_list_size);
        exit(1);
    }
    return std::vector<BIF::COMMAND_SEGMENT>(newer_list.begin(), newer_list.end());
}

std::list<BIF::COMMAND_SEGMENT> DmaLoopExtension::extractLoopCommand(std::list<BIF::COMMAND_SEGMENT> same_dim_dma_list, bool debug) {

    if (debug) {
        printf("    [DMA LOOP Extract] In List Size: %lu \n", same_dim_dma_list.size());
//        int cnt = 0;
        for (auto i: same_dim_dma_list) {
            printf("       %s\n", i.to_char());
//            cnt++;
//            if (cnt > 3) break;
        }
    }

    uint input_size = same_dim_dma_list.size();
    std::list<BIF::COMMAND_SEGMENT> new_list;
    std::list<BIF::COMMAND_SEGMENT> loop_list;
    uint8_t cluster_shift = 0;
    bool cluster_shift_set = false;
    uint8_t unit_shift = 0;
    bool unit_shift_set = false;
    int cluster_len = 0;
    int unit_len = 0;
    int inter_unit_len = 0;
    int mm_incr = 0;
    int lm_incr = 0;

    bool is_looping = false;

    auto base = same_dim_dma_list.front();
    same_dim_dma_list.pop_front();
    auto second = same_dim_dma_list.front();
    same_dim_dma_list.pop_front();

    auto previous = second;

    auto cluster_mask_check = is_modified_base_mask(base.dma.cluster, second.dma.cluster);
    auto unit_mask_check = is_modified_base_mask(base.dma.unit_mask, second.dma.unit_mask);

    enum UPDATE_TARGET_LOOP{
        INTER_UNIT,
        UNIT,
        CLUSTER
    } loop_update_target = INTER_UNIT;
    auto UPDATE_TARGET_LOOP_to_string = [](UPDATE_TARGET_LOOP e){
        switch (e) {
            case INTER_UNIT:
                return "INTER_UNIT";
            case UNIT:
                return "UNIT";
            case CLUSTER:
                return "CLUSTER";
        }
        return "unreachable";
    };
    uint8_t cluster_mask_shift_last = 0, unit_mask_shift_last = 0;

    while (true) {
        bool loop = true;

        /**
         * extract cluster mask shift factor
         */
        cluster_mask_check = is_modified_base_mask(base.dma.cluster, second.dma.cluster);
        if (!cluster_mask_check.first) {
            loop = false;
        }

        /**
         * extract unit mask shift factor
         */
        unit_mask_check = is_modified_base_mask(base.dma.unit_mask, second.dma.unit_mask);
        if (!unit_mask_check.first) {
            loop = false;
        }

        if (cluster_mask_check.first){
            if (cluster_mask_check.second == 0 && unit_mask_check.second == 0) {
                // inter unit ++
            } else if (cluster_mask_check.second == 0) {
                // unit ++
            } else if (!cluster_shift_set) {
                cluster_shift = cluster_mask_check.second;
                cluster_shift_set = true;
                // cluster ++
            }
        }

        if (unit_mask_check.first){
            if (cluster_mask_check.second == 0 && unit_mask_check.second == 0) {
                // inter unit ++
            } else if (cluster_mask_check.second == 0 && !unit_shift_set) {
                unit_shift = unit_mask_check.second;
                unit_shift_set = true;
                // unit ++
            } else {
                // cluster ++
            }
        }

        /**
         * extract mm addr, lm addr increments + looping count of clusters, units, inter units
         * verify them
         */
        if (loop) {
            if (!is_looping) {    // first
                // extract mm_inc, lm_incr
                mm_incr = second.dma.mm_addr - base.dma.mm_addr;
                lm_incr = second.dma.lm_addr - base.dma.lm_addr;
                cluster_len = 0;
                unit_len = 0;
                inter_unit_len = 0;
                unit_mask_shift_last = unit_mask_check.second;
                cluster_mask_shift_last = cluster_mask_check.second;
                if (cluster_mask_check.second == 0 && unit_mask_check.second == 0) {
                    inter_unit_len++;
                    loop_update_target = INTER_UNIT;
                } else if (cluster_mask_check.second == 0) {
                    unit_len++;
                    loop_update_target = UNIT;
                } else {
                    cluster_len++;
                    loop_update_target = CLUSTER;
                }
            } else {
                // check if mm_incr, lm_incr is correct...
                //   else set loop = false
                //   check: previous -> second

              if (mm_incr != (int32_t)(second.dma.mm_addr - previous.dma.mm_addr)) {
                    if (debug)
                        printf("  \e[91m  [DMA LOOP Extract] Undo. No New Loop: mm != %i\e[m\n", (second.dma.mm_addr - previous.dma.mm_addr));
                    loop = false; // TODO: loop will not be created -> fix to create one and continue
                } else {
                    if (loop_update_target == INTER_UNIT && cluster_mask_check.second == 0 && unit_mask_check.second == 0) {
                        if (lm_incr != (int32_t)(second.dma.lm_addr - previous.dma.lm_addr)) {
                            if (debug)
                                printf("  \e[91m  [DMA LOOP Extract] Undo. No New Loop: lm != %i (target: %i)\e[m\n", (second.dma.lm_addr - previous.dma.lm_addr), lm_incr);
                            loop = false; // TODO: loop will not be created -> fix to create one and continue
                        } else {
                            loop_update_target = INTER_UNIT;
                            inter_unit_len++;
                        }
                    } else if (loop_update_target == INTER_UNIT){    // no longer update inter unit loop count
                        loop_update_target = UNIT;
                    }
                    if (loop_update_target == UNIT && cluster_mask_check.second == 0 && unit_mask_check.second != 0 && unit_mask_shift_last != unit_mask_check.second) {
                        if (unit_mask_check.second < unit_mask_shift_last){
                            if (debug)
                                printf("  \e[91m  [DMA LOOP Extract] Undo. No New Loop: unit mask shift changed!\e[m\n");
                            loop = false; // TODO: loop will not be created -> fix to create one and continue
                        } else {
                            unit_mask_shift_last = unit_mask_check.second;
                            loop_update_target = UNIT;
                            unit_len++;
                        }
                    } else if (loop_update_target == UNIT && cluster_mask_check.second != 0){ // no longer update unit loop count
                        loop_update_target = CLUSTER;
                    }
                    if (loop_update_target == CLUSTER && cluster_mask_check.second != 0 && cluster_mask_shift_last != cluster_mask_check.second){
                        cluster_mask_shift_last = cluster_mask_check.second;
                        cluster_len++;
                    }
                }
            }
        }

        /**
         * generate list of commands to be merged into one loop
         */
        if (loop) {
            if (is_looping) {
                loop_list.push_back(second);
                if (debug)
                    printf("    [DMA LOOP Extract] Continue Loop parameter update: mm %i, lm %i, cluster_len: %i (<< %i), unit_len: %i (<< %i), inter_unit_len: %i, list length: %lu, Update: %s\n",
                           mm_incr, lm_incr, cluster_len, cluster_shift, unit_len, unit_shift, inter_unit_len, loop_list.size(), UPDATE_TARGET_LOOP_to_string(loop_update_target));
            } else {
                is_looping = true;
                loop_list.push_back(base);
                loop_list.push_back(second);
                if (debug)
                    printf("    [DMA LOOP Extract] New Loop      parameter update: mm %i, lm %i, cluster_len: %i (<< %i), unit_len: %i (<< %i), inter_unit_len: %i, list length: %lu, Update: %s\n",
                           mm_incr, lm_incr, cluster_len, cluster_shift, unit_len, unit_shift, inter_unit_len, loop_list.size(), UPDATE_TARGET_LOOP_to_string(loop_update_target));
            }
            // front stays the same
        } else {
            if (is_looping) {
                if (debug)
                    printf("    [DMA LOOP Extract] Loop End -> Gen Parameter\n");
                // end loop list with base: front. generate loop command
                //   validate lm and mm for all inter, units, clusters
                //   append to new_list with base
                if (loop_list.size() >= minimal_count_of_instr_in_loop_to_generate_loop) {
                    auto loop_cmd = generateLoopCommand(loop_list, cluster_shift, unit_shift, cluster_len, unit_len, inter_unit_len, mm_incr, lm_incr, loop_list.size());
                    if (!verifyLoopCommand(loop_list, loop_cmd, debug)) {
                        if (debug)
                            printf("\e[91m[Error] Loop Command not executing all required DMA Commands! -> using old way with a lot of commands!\e[0m\n");
                        new_list.insert(new_list.end(), loop_list.begin(), loop_list.end());
                    } else {
                        new_list.push_back(loop_cmd);
                        new_list.push_back(base);
                    }
                } else {
                    if (debug)
                        printf("    [DMA LOOP Extract] No Loop as length of list of commands ( %zu ) smaller than %i!\n", loop_list.size(), minimal_count_of_instr_in_loop_to_generate_loop);
                    for (auto i: loop_list)
                        new_list.push_back(i);
                }
                cluster_shift_set = false;
                unit_shift_set = false;
                is_looping = false;
//                loop_update_target = INTER_UNIT;
                loop_list.clear();
            } else {
                if (debug)
                    printf("    [DMA LOOP Extract] No Loop\n");
                total_dmas += 1;
                new_list.push_back(base);
            }
            if (same_dim_dma_list.empty()){
                total_dmas += 1;
                new_list.push_back(second);
            }
            base = second;
        }
        previous = second;
        if (same_dim_dma_list.empty()) break;


        second = same_dim_dma_list.front();
        same_dim_dma_list.pop_front();
    }

    if (is_looping){
        if (loop_list.size() > minimal_count_of_instr_in_loop_to_generate_loop){
            auto loop_cmd = generateLoopCommand(loop_list, cluster_shift, unit_shift, cluster_len, unit_len, inter_unit_len, mm_incr, lm_incr, loop_list.size());
            if (!verifyLoopCommand(loop_list, loop_cmd, debug)) {
                if (debug)
                    printf("\e[91m[Error] Finalizing Loop Command not executing all required DMA Commands! -> using old way with a lot of commands!\e[0m\n");
                new_list.insert(new_list.end(), loop_list.begin(), loop_list.end());
            } else {
                new_list.push_back(loop_cmd);
                new_list.push_back(base);
            }
        } else {
            for (auto i : loop_list)
                new_list.push_back(i);
        }
        if (debug)
            printf("    [DMA LOOP Extract] Finalize. Loop End -> Gen Parameter\n");
    }

    if (debug) {
        printf("    [DMA LOOP Extract] Out List Size: %lu \n", new_list.size());
        for (auto i: new_list) {
            printf("       %s\n", i.to_char());
        }
    }

    uint output_size = new_list.size();
    for (auto i : new_list) {
        if (i.type.type == DMA_CMD && i.dma.direction == loop){
            output_size += i.dma_loop.dma_cmd_count - 2;    // base + loop command
        }
    }
    if (output_size != input_size){
        printf("    [DMA LOOP Extract] Out Size wrong when unrolling all loops! out: %i, in: %i\n", output_size, input_size);
        for (auto i : new_list) {
            printf("                %s\n", i.to_char());
        }
        exit(1);
    }

    return new_list;
}

uint8_t DmaLoopExtension::get_tailing_zeros(const uint32_t value) {
    /*if (value & 0xffff0000){
        printf("\e[91m[Error] get_tailing_zeros only implemented for numbers with maximal 16 tailing zeros!\n\e[0m");
        exit(1);
    }*/

    if (value == 0) {
        return 32;
    }
    if (!(value & 0b1111111111111111111111111111111)) {
        return 31;
    }
    if (!(value & 0b111111111111111111111111111111)) {
        return 30;
    }
    if (!(value & 0b11111111111111111111111111111)) {
        return 29;
    }
    if (!(value & 0b1111111111111111111111111111)) {
        return 28;
    }
    if (!(value & 0b111111111111111111111111111)) {
        return 27;
    }
    if (!(value & 0b11111111111111111111111111)) {
        return 26;
    }
    if (!(value & 0b1111111111111111111111111)) {
        return 25;
    }
    if (!(value & 0b111111111111111111111111)) {
        return 24;
    }
    if (!(value & 0b11111111111111111111111)) {
        return 23;
    }
    if (!(value & 0b1111111111111111111111)) {
        return 22;
    }
    if (!(value & 0b111111111111111111111)) {
        return 21;
    }
    if (!(value & 0b11111111111111111111)) {
        return 20;
    }
    if (!(value & 0b1111111111111111111)) {
        return 19;
    }
    if (!(value & 0b111111111111111111)) {
        return 18;
    }
    if (!(value & 0b11111111111111111)) {
        return 17;
    }
    if (!(value & 0b1111111111111111)) {
        return 16;
    }
    if (!(value & 0b111111111111111)) {
        return 15;
    }
    if (!(value & 0b11111111111111)) {
        return 14;
    }
    if (!(value & 0b1111111111111)) {
        return 13;
    }
    if (!(value & 0b111111111111)) {
        return 12;
    }
    if (!(value & 0b11111111111)) {
        return 11;
    }
    if (!(value & 0b1111111111)) {
        return 10;
    }
    if (!(value & 0b111111111)) {
        return 9;
    }
    if (!(value & 0b11111111)) {
        return 8;
    }
    if (!(value & 0b1111111)) {
        return 7;
    }
    if (!(value & 0b111111)) {
        return 6;
    }
    if (!(value & 0b11111)) {
        return 5;
    }
    if (!(value & 0b1111)) {
        return 4;
    }
    if (!(value & 0b111)) {
        return 3;
    }
    if (!(value & 0b11)) {
        return 2;
    }
    if (!(value & 0b1)) {
        return 1;
    }
    return 0;
}

std::pair<bool, uint8_t> DmaLoopExtension::is_modified_base_mask(const uint32_t base_mask, const uint32_t other_mask) {
    uint8_t bshift = get_tailing_zeros(base_mask);
    uint8_t shift = get_tailing_zeros(other_mask);
    bool equal = (base_mask == (other_mask >> (shift - bshift)));
//    printf("      [DMA LOOP gen] base mask %08x, other mask %08x | identical? %s, shift: %i, base was shifted: %i\n", (base_mask >> bshift), (other_mask >> shift), equal?"yes":"no", shift, bshift);
    return std::pair<bool, uint8_t>(equal, (shift - bshift));
}

BIF::COMMAND_SEGMENT DmaLoopExtension::generateLoopCommand(std::list<BIF::COMMAND_SEGMENT> &loop_cmd_list, uint8_t cluster_shift, uint8_t unit_shift, int cluster_len,
                                                           int unit_len, int inter_unit_len, int mm_incr, int lm_incr, int count, bool debug) {
    BIF::COMMAND_SEGMENT loop_cmd;

    loop_cmd.dma.type = DMA_CMD;
    loop_cmd.dma.direction = DMA_DIRECTION::loop;
    loop_cmd.dma_loop.mm_incr = mm_incr;
    loop_cmd.dma_loop.lm_incr = lm_incr;
    loop_cmd.dma_loop.cluster_loop_shift_incr = cluster_shift;
    loop_cmd.dma_loop.cluster_loop_len = cluster_len;
    loop_cmd.dma_loop.unit_loop_shift_incr = unit_shift;
    loop_cmd.dma_loop.unit_loop_len = unit_len;
    loop_cmd.dma_loop.inter_unit_loop_len = inter_unit_len;
    loop_cmd.dma_loop.dma_cmd_count = count; // (1+inter_unit_len) * (1+unit_len) * (1+cluster_len);

    total_loop_encoded_dmas += loop_cmd.dma_loop.dma_cmd_count;
    total_loops +=1 ;
    return loop_cmd;
}

bool DmaLoopExtension::verifyLoopCommand(std::list<BIF::COMMAND_SEGMENT> &loop_cmd_list, BIF::COMMAND_SEGMENT &loop_cmd, bool debug) {
    auto base = loop_cmd_list.front();

    if (debug) {
        printf("      [DMA LOOP Verify] In List Size: %lu \n", loop_cmd_list.size());
        printf("        Loop: %s\n", loop_cmd.dma_loop.to_char());
//        int cnt = 0;
        for (auto i: loop_cmd_list) {
            printf("         %s\n", i.to_char());
//            cnt++;
//    //        if (cnt > 3) break;
        }
    }


    std::list<BIF::COMMAND_SEGMENT> generateCmds;

    uint32_t cluster_mask = base.dma.cluster;
    uint32_t unit_mask = base.dma.unit_mask;
    uint32_t lm_addr = base.dma.lm_addr;
    uint32_t mm_addr = base.dma.mm_addr;

    int generated_cmds = 0;

    for (int cluster = 0; cluster <= loop_cmd.dma_loop.cluster_loop_len; ++cluster) {
        for (int unit = 0; unit <= loop_cmd.dma_loop.unit_loop_len; ++unit) {
            for (int inter_unit = 0; inter_unit <= loop_cmd.dma_loop.inter_unit_loop_len; ++inter_unit) {

                BIF::COMMAND_SEGMENT gen(base);
                gen.dma.mm_addr = mm_addr;
                gen.dma.lm_addr = lm_addr;
                gen.dma.cluster = cluster_mask;
                gen.dma.unit_mask = unit_mask;

                generateCmds.push_back(gen);

                generated_cmds++;
                if (generated_cmds >= loop_cmd.dma_loop.dma_cmd_count){
                    // exit generation.
                    inter_unit = loop_cmd.dma_loop.inter_unit_loop_len;
                    unit = loop_cmd.dma_loop.unit_loop_len;
                    cluster = loop_cmd.dma_loop.cluster_loop_len;
                }

                lm_addr += loop_cmd.dma_loop.lm_incr;
                mm_addr += loop_cmd.dma_loop.mm_incr;
            }
            lm_addr = base.dma.lm_addr;
            unit_mask <<= loop_cmd.dma_loop.unit_loop_shift_incr;
        }
        unit_mask = base.dma.unit_mask;
        cluster_mask <<= loop_cmd.dma_loop.cluster_loop_shift_incr;
    }

    auto dma_compare = [](const BIF::COMMAND_SEGMENT &a, const BIF::COMMAND_SEGMENT &b){
        return (a.dma.equals(b.dma) && a.dma.mm_addr == b.dma.mm_addr);
    };

    bool error = false;
    if (generateCmds.size() != loop_cmd_list.size()){
        if (debug)
            printf("\e[91m [Error] Loop Generate DMA list size is wrong!\e[0m\n");
        error = true;
    }
    auto it1 = generateCmds.begin();
    auto it2 = loop_cmd_list.begin();
    for(; it1 != generateCmds.end() && it2 != loop_cmd_list.end(); ++it1, ++it2) {
        if (!dma_compare(*it1, *it2)){
            if (debug)
                printf("\e[91m[Error] Loop Generate DMA Commands differ from source command list!\e[0m\n");
            error = true;
        }
    }

    if (debug) {
        printf("      [DMA LOOP Verify] Out List Size: %lu \n", generateCmds.size());
//        int cnt = 0;
        for (auto i: generateCmds) {
            printf("         %s\n", i.to_char());
//            cnt++;
//        if (cnt > 3) break;
        }
    }

    return !error;
}
