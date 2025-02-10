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
// Created by gesper on 06.10.23.
//

#include "DMAStoreSplitter.h"
#include <math.h>

/**
 * iterator "begin" points to a dma block. return true if this block contains a l2e command / store
 * @param begin
 * @return
 */
bool DmaStoreSpliiter::dma_cmd_block_contains_store(std::vector<BIF::COMMAND_SEGMENT>::iterator begin) {
    int block_size = begin->dma.unit_mask;
    begin++;
    for (int i = 0; i < block_size; ++i) {
        if (begin->dma.direction == l2e1D || begin->dma.direction == l2e2D) {
            return true;
        }
    }
    return false;
}

/**
 * as a block of commands will be issued, all clusters should get commands if possible
 * inside a cluster [sublist], dma commands are sorted by mm_addr
 * @param stores
 */
void DmaStoreSpliiter::store_resort(std::list<BIF::COMMAND_SEGMENT *> &stores){
//    printf("Store resort init size: %zu\n", stores.size());

    // order by cluster
    std::vector<std::list<BIF::COMMAND_SEGMENT *>> cluster_list_commands{32};
    for (auto c: stores) {
        int cluster_unmasked = log2(c->dma.cluster);
        assert(cluster_unmasked <= 31);
        cluster_list_commands[cluster_unmasked].push_back(c);
    }

    // suborder by mm_addr
    for (auto list: cluster_list_commands){
        list.sort( [] (BIF::COMMAND_SEGMENT *a, BIF::COMMAND_SEGMENT * &b )
        {
            return a->dma.mm_addr > b->dma.mm_addr;
        } );
    }

    // create new list
    auto count = stores.size();
    stores.clear();

    int cluster = 0;
    for (unsigned int i = 0; i < count; ++i) {
        // check this cluster list has commands
        while(cluster_list_commands[cluster].empty()){
            cluster++;
            cluster = cluster % 32;
        }

        // push to final list
        stores.push_back(cluster_list_commands[cluster].front());
        cluster_list_commands[cluster].pop_front();

        // get next cluster
        cluster++;
        cluster = cluster % 32;
    }
//    printf("Store resort final size: %zu\n", stores.size());
}

/**
 * append to new_list
 * given segment list contains one block of store commands
 * e.g.  [STORE + LOAD + PROCESS      ||                LOAD + PROCESS ||               LOAD + PROCESS ]
 * goal: [STORE(1./3) + LOAD + PROCESS || STORE(1./3) + LOAD + PROCESS || STORE(1./3) + LOAD + PROCESS ]
 * @param segment
 */
void DmaStoreSpliiter::split_and_append(std::list<BIF::COMMAND_SEGMENT *> &segment) {
    std::list<BIF::COMMAND_SEGMENT *> stores;
    std::list<BIF::COMMAND_SEGMENT *> others;

    int last_dma_store = 0;
    int i = 0;
    BIF::COMMAND_SEGMENT *store_block = segment.front();
    for (auto s: segment) {
        assert(store_block->type.type == DMA_BLOCK);
        if (s->type.type == DMA_CMD && (s->dma.direction == l2e1D || s->dma.direction == l2e2D)) {
            stores.push_back(s);
            last_dma_store = i;
        } else {
            others.push_back(s);
        }
        i++;
    }

    if (last_dma_store == int(stores.size())) {   // all stores were at front block -> okay
        store_block->dma.unit_mask -= stores.size();

        // count dma blocks to merge stores into
        int dma_blocks = 0;
        for (auto s: others) {
            if (s->type.type == DMA_BLOCK) dma_blocks++;
        }

        // split stores to
        int num_stores_per_block = int(std::ceil(float(stores.size()) / float(dma_blocks)));

        // reorder store commands to fill all clusters in each block
        store_resort(stores);

        // insert stores
        for (auto cmd = others.begin(); cmd != others.end(); cmd++) {
            // find next block
            if ((*cmd)->type.type != DMA_BLOCK) {
                continue;
            }
            auto block = *cmd;
            cmd++;  // insert after block
            int inserted_stores = 0;
            for (int j = 0; j < num_stores_per_block; ++j) {
                if (stores.empty()) break;  // in last block, the number of stores can be smaller -> split to ceiled division result
                auto store_cmd = stores.front();
                stores.pop_front();
                others.insert(cmd, store_cmd);
                inserted_stores++;
            }
            block->dma.unit_mask += inserted_stores;
        }

        if (debug)
            printf("\e[92m  Insert  num_stores_per_block: %i, dma_blocks: %i \e[0m\n", num_stores_per_block, dma_blocks);

        // add to final list
        for (auto s: others) {
            new_list.push_back(*s);
        }

    } else {    // continue old way without splitting the store command block
        printf("\e[33m[Split of DMA Stores failed as multiple distributed store commands are present in one split block!] Continue old way...\e[0m\n");
        for (auto s: segment) {
            new_list.push_back(*s);
        }
    }
}

/**
 * push all cmds of the block to segment list.
 * Move cmd iterator to final command (of the block)
 * @param cmd
 * @param segment
 */
void DmaStoreSpliiter::collect_block(std::vector<BIF::COMMAND_SEGMENT>::iterator &cmd, std::list<BIF::COMMAND_SEGMENT *> &segment) {
    int block_size = cmd->dma.unit_mask;
    segment.push_back(&*cmd); // include DMA_BLOCK command
    for (int i = 0; i < block_size; ++i) {
        cmd++;
        segment.push_back(&*cmd);
    }
}

/**
 * Input: List of DMA(Block)|VPRO|SYNC
 *
 * split to segments
 *   begin segment with dma store block
 *   end/new segment if end | dma block with store is next
 *
 * for each segment:
 *   extract (remove all dma stores -> reduce dma block size) -> they should be at begin in a single block
 *   split (extracted store sequence to # of dma blocks in segment)
 *   add (store to dma blocks -> increase dma block size)
 *       e.g. [STORE + LOAD + PROCESS      ||                LOAD + PROCESS ||               LOAD + PROCESS ]
 *            =>
 *            [STORE(1./3) + LOAD + PROCESS || STORE(1./3) + LOAD + PROCESS || STORE(1./3) + LOAD + PROCESS ]
 *
 * Output: list as input
 */
std::vector<BIF::COMMAND_SEGMENT> DmaStoreSpliiter::generate() {

    std::list<BIF::COMMAND_SEGMENT *> segment;

    for (auto cmd = cmd_list.begin(); cmd != cmd_list.end(); cmd++) {
        // is there a store?
        if (cmd->type.type == COMMAND_SEGMENT_TYPE::DMA_BLOCK && dma_cmd_block_contains_store(cmd)) {
            if (segment.empty()) {
                // first block -> collect
                collect_block(cmd, segment);
            } else {
                // split + append segment
                split_and_append(segment);
                // start new block -> collect
                segment.clear();
                collect_block(cmd, segment);
            }
        } else if (!segment.empty()) {
            segment.push_back(&*cmd);   // collect single cmd
        } else {    // begin. no segment yet. add cmd to final list
            new_list.push_back(*cmd);
        }
    }
    if (!segment.empty()) {
        split_and_append(segment);
    }

    return std::vector<BIF::COMMAND_SEGMENT>(new_list.begin(), new_list.end());
}
