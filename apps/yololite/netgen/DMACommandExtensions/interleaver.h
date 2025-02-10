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
// Created by gesper on 06.04.22.
//

#ifndef INTERLEAVER_H
#define INTERLEAVER_H

//#include <helper.h>
#include <QList> // FIXME remove Qt dependency
#include <QVector>
#include "bif.h"


class SegmentInterleaver {

public:
    explicit SegmentInterleaver(std::vector<BIF::COMMAND_SEGMENT> &cmd_list) {
        //FIXME: remove qt dependencies in SegmentInterleaver, replace with std::vector
        command_list = QList<BIF::COMMAND_SEGMENT>::fromVector(QVector<BIF::COMMAND_SEGMENT>::fromStdVector(cmd_list));
    }

    std::vector<BIF::COMMAND_SEGMENT> interleave() {

        // lambda expression to compare sizes of two dma commands
        auto dma_comp = [](const BIF::COMMAND_SEGMENT &a, const BIF::COMMAND_SEGMENT &b) {
            return a.dma.x_size * a.dma.y_size > b.dma.x_size * b.dma.y_size;
        };

        enum DIR{
            E2L, 
            L2E
        };

        QList<BIF::COMMAND_SEGMENT> dma_block;
        QList<BIF::COMMAND_SEGMENT> vpro_block;
        QList<BIF::COMMAND_SEGMENT> cmds_interleaved;

        for (int i = 0; i < command_list.size(); i++) {
            const auto &s = command_list.at(i);

            if (s.type.type == DMA_CMD) {
                dma_block.append(s);
            } else if (s.type.type == VPRO_CMD) {
                vpro_block.append(s);
            }
            if (s.type.type == VPRO_WAIT || s.type.type == DMA_WAIT || s.type.type == BOTH_SYNC || i == command_list.size() - 1){    // sync
                if (dma_block.length() != 0 && vpro_block.length() != 0) {  // both are filled
                    // interleave and append
                    // sort dma commands (largest first), but take care of direction change (needed????)
                    QList<BIF::COMMAND_SEGMENT> dma_block_sorted;
                    QList<BIF::COMMAND_SEGMENT> dma_subblock;
                    DIR dir = (dma_block.front().dma.direction <= 1) ? E2L : L2E;
                    
                    for (auto dma_c: dma_block) {
                        DIR dir_c = (dma_c.dma.direction <= 1) ? E2L : L2E;
                        if (dir_c == dir) {
                            dma_subblock.append(dma_c);
                        } else {
                            dir = dir_c;
                            std::stable_sort(dma_subblock.begin(), dma_subblock.end(),
                                             dma_comp);
                            dma_block_sorted.append(dma_subblock);
                            dma_subblock.clear();
                            dma_subblock.append(dma_c);
                        }
                    }
                    if (!dma_subblock.empty()) {
                        std::stable_sort(dma_subblock.begin(), dma_subblock.end(),
                                         dma_comp);
                        dma_block_sorted.append(dma_subblock);
                        dma_subblock.clear();
                    }

                    while (dma_block_sorted.length() != 0 && vpro_block.length() != 0) {
                        cmds_interleaved.append(dma_block_sorted.takeFirst());
                        cmds_interleaved.append(vpro_block.takeFirst());
                    }
                    cmds_interleaved.append(dma_block_sorted);
                    cmds_interleaved.append(vpro_block);
                    dma_block.clear();
                    dma_block_sorted.clear();
                    vpro_block.clear();

                } else if (dma_block.length() != 0 || vpro_block.length() != 0) { // only one is filled

                    if (!dma_block.empty()) {
                        // sort dma commands (largest first), but take care of direction change (needed????)
                        QList<BIF::COMMAND_SEGMENT> dma_block_sorted;
                        QList<BIF::COMMAND_SEGMENT> dma_subblock;
                        DIR dir = (dma_block.front().dma.direction <= 1) ? E2L : L2E;
                        for (auto dma_c: dma_block) {
                            DIR dir_c = (dma_c.dma.direction <= 1) ? E2L : L2E;
                            if (dir_c == dir) {
                                dma_subblock.append(dma_c);
                            } else {
                                dir = dir_c;
                                std::stable_sort(dma_subblock.begin(), dma_subblock.end(),
                                                 dma_comp);
                                dma_block_sorted.append(dma_subblock);
                                dma_subblock.clear();
                                dma_subblock.append(dma_c);
                            }
                        }
                        if (!dma_subblock.empty()) {
                            std::stable_sort(dma_subblock.begin(), dma_subblock.end(),
                                             dma_comp);
                            dma_block_sorted.append(dma_subblock);
                            dma_subblock.clear();
                        }
                        cmds_interleaved.append(dma_block_sorted);
                        dma_block.clear();
                    }

                    if (!vpro_block.empty()) {
                        cmds_interleaved.append(vpro_block);
                        vpro_block.clear();
                    }
                } else {  // both lists are empty
                }
                cmds_interleaved.append(s);

                if(!vpro_block.empty() || !dma_block.empty()){
                    printf/*_error*/("either VPRO or DMA Block list not yet empty!\n");
                }
            }
        }

        if(!vpro_block.empty() || !dma_block.empty()){
            printf/*_error*/("BLOCKS not yet empty!\n");
        }


        /**
         * generate masks for dma command clusters (merge if possible)
         */
        QList<BIF::COMMAND_SEGMENT> cluster_unit_maked_cmds_interleaved;
        BIF::COMMAND_SEGMENT *start = nullptr;
        uint32_t cluster_mask = 0;
        
        for (auto &cmd_seg : cmds_interleaved) {
            if (cmd_seg.type.type != DMA_CMD){
                if (start != nullptr){
                    // push to list with existing mask/reference
                    // reset reference
                    start->dma.cluster = cluster_mask;
                    cluster_unit_maked_cmds_interleaved.append(*start);
                    start = nullptr;
                    cluster_mask = 0;
                }
                // add non dma command
                cluster_unit_maked_cmds_interleaved.append(cmd_seg);
            } else {
                // this is a dma command, check if mask get extended/set
                if (start == nullptr){
                    // set mask/reference
                    start = &(cmd_seg);
                    cluster_mask = uint32_t(0b1u) << cmd_seg.dma.cluster;
                } else {
                    // extend mask
                    if (cmd_seg.dma.mm_addr == start->dma.mm_addr &&
                        cmd_seg.dma.lm_addr == start->dma.lm_addr &&
                        cmd_seg.dma.x_size == start->dma.x_size &&
                        cmd_seg.dma.y_leap == start->dma.y_leap &&
                        cmd_seg.dma.y_size == start->dma.y_size &&
                        cmd_seg.dma.padding == start->dma.padding &&
                        cmd_seg.dma.unit_mask == start->dma.unit_mask &&
                        cmd_seg.dma.direction == start->dma.direction &&
                        cmd_seg.dma.isBiasOffset == start->dma.isBiasOffset &&
                        cmd_seg.dma.isKernelOffset == start->dma.isKernelOffset) {
                        assert(cmd_seg.dma.cluster != start->dma.cluster);
                        cluster_mask |= uint32_t(0b1u) << cmd_seg.dma.cluster;
                    } else {
                        // no extension possible
                        // push current reference to list and set this cmd as new reference
                        start->dma.cluster = cluster_mask;
                        cluster_unit_maked_cmds_interleaved.append(*start);

                        start = &(cmd_seg);
                        cluster_mask = uint32_t(0b1u) << cmd_seg.dma.cluster;
                    }
                }
            }
        }
        // append remaining reference
        if (start != nullptr){
            start->dma.cluster = cluster_mask;
            cluster_unit_maked_cmds_interleaved.append(*start);
        }

//        return std::vector<BIF::COMMAND_SEGMENT> (cmds_interleaved.begin(), cmds_interleaved.end());
        return std::vector<BIF::COMMAND_SEGMENT> (cluster_unit_maked_cmds_interleaved.begin(), cluster_unit_maked_cmds_interleaved.end());
    }


private:
    QList<BIF::COMMAND_SEGMENT> command_list;
};


#endif // INTERLEAVER_H

