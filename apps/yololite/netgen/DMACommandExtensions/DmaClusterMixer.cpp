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
// Created by gesper on 17.07.23.
//

#include "DmaClusterMixer.h"

#include <algorithm>
#include <random>
#include <map>

bool DmaClusterMixer::is_simple_dma(std::vector<BIF::COMMAND_SEGMENT>::iterator &cmd) {
    if (cmd->type.type == DMA_CMD) {
        return cmd->dma.direction != loop;
    }
    return false;
}

bool DmaClusterMixer::is_loop(std::vector<BIF::COMMAND_SEGMENT>::iterator &cmd) {
    if (cmd->type.type == DMA_CMD) {
        return cmd->dma.direction == loop;
    }
    return false;
}

void DmaClusterMixer::find_block(std::vector<BIF::COMMAND_SEGMENT>::iterator &block_begin, std::vector<BIF::COMMAND_SEGMENT>::iterator &block_end) {
    bool begin_valid = false;
    bool end_valid = false;

    while (!begin_valid) {
        if (is_simple_dma(block_begin)) {
            begin_valid = true;
        } else {
            if (is_loop(block_begin)) {
                block_begin++;
                block_end++;
            }
            block_begin++;
            block_end++;
        }
    }

    if (block_end != cmd_list.end()) {
        block_end++;
    } else {
        end_valid = true;
    }
    while (!end_valid) {
        if (is_simple_dma(block_end) && block_end->dma.direction == block_begin->dma.direction) {
            if (block_end != cmd_list.end()) {
                block_end++;    // continue search for end
            } else {    // list ended
                end_valid = true;
            }
        } else {
            // end this block
            end_valid = true;
            block_end--;    // this is final dma cmd in this block
        }
    }
}

std::vector<BIF::COMMAND_SEGMENT> DmaClusterMixer::generate(bool debug) {

    auto block_begin = cmd_list.begin();
    auto block_end = cmd_list.begin();

//    printf("\n############ DMA Cluster Mixer - begin - ##############\n");
    while (block_end != cmd_list.end()) {
        find_block(block_begin, block_end);

        // if last cmd only, exit loop
        if (block_begin == block_end && block_end == cmd_list.end())
            break;

        if (block_begin->dma.direction == l2e1D || block_begin->dma.direction == l2e2D){
            // begin to end (including) is a block of sequential dma commands

//            printf("[Before] Block found! [Length: %li]\n", block_end - block_begin + 1);
//            for (auto i = block_begin; i != block_end + 1 ; i++) {
//                printf("[Before] %s\n", i->to_char());
//            }

            // random order?
//            auto rng = std::default_random_engine {};
//            std::shuffle(block_begin, block_end, rng);

            // loop over clusters first
            std::map<int, std::list<std::vector<BIF::COMMAND_SEGMENT>::iterator>> clusters;
            for (auto i = block_begin; i != block_end + 1 ; i++) {
                clusters[i->dma.cluster].push_back(i);
            }

            std::vector<BIF::COMMAND_SEGMENT> reordered;
            reordered.reserve(block_end - block_begin + 1);

            int pos = 0;
            while(true){
                bool appended = false;
                for (auto &i : clusters) {
                    if (!i.second.empty()) {
                        reordered[pos] = *(i.second.front());
//                        printf("[Reorder] %s\n", reordered[pos].to_char());
                        i.second.pop_front();
                        pos++;
                        appended = true;
                    }
                }
                if (!appended) break;
            }
            assert(block_end - block_begin + 1 == pos);

            pos = 0;
            for (auto i = block_begin; i != block_end + 1 ; i++, pos++) {
                *i = reordered[pos];
            }
//            std::move(reordered.begin(), reordered.end() + 1, block_begin);   // not working (copy, move)

//            printf("[After] Block found! [Length: %li]\n", block_end - block_begin + 1);
//            for (auto i = block_begin; i != block_end + 1 ; i++) {
//                printf("[After] %s\n", i->to_char());
//            }
        }

        if (block_end != cmd_list.end()) {
            block_end++;
            block_begin = block_end;
        }
    }
//    printf("\n############ DMA Cluster Mixer - end - ##############\n");

    return cmd_list;
}

