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
// Created by gesper on 14.10.22.
//

#ifndef DMA_BLOCK_EXTENSION_H
#define DMA_BLOCK_EXTENSION_H

//#include <helper.h>
#include <QList>

constexpr bool skip_dma_block_gen = false;


struct CMD_BLOCK {
    COMMAND_SEGMENT_TYPE type = UNKNOWN_COMMAND_SEGMENT_TYPE;
    QList<BIF::COMMAND_SEGMENT> list;
};


class DmaBlockExtension {

public:
    DmaBlockExtension(std::vector<BIF::COMMAND_SEGMENT> &cmd_list, int dma_block_size=65535): dma_block_size(dma_block_size) {
        //FIXME: remove qt dependencies in SegmentInterleaver, replace with std::vector
        command_list = QList<BIF::COMMAND_SEGMENT>::fromVector(QVector<BIF::COMMAND_SEGMENT>::fromStdVector(cmd_list));
    }

    std::vector<BIF::COMMAND_SEGMENT> generate() {
        QList<BIF::COMMAND_SEGMENT> dma_block, vpro_block;
        QList<BIF::COMMAND_SEGMENT> dma_block_cluster_broadcasts;
        QList<BIF::COMMAND_SEGMENT> cmds_final;

        QList<CMD_BLOCK> block_list;
        int merged_dma_commands = 0;    // by cluster broadcasting

        enum dir_t {
            UNINIT = 0,
            E2L = 1,
            L2E = 2,
        } dir = UNINIT;

        bool dir_switch = false;
    //    for (const auto &s : command_list){
        for (int i = 0; i < command_list.size(); i++) {
            const auto &s = command_list.at(i);

            if (s.type.type == COMMAND_SEGMENT_TYPE::DMA_CMD) {
                if (dir == UNINIT) {
                    dir_switch = false;
                    if (s.dma.direction == e2l1D || s.dma.direction == e2l2D)
                        dir = E2L;
                    else
                        dir = L2E;    
                } else {
                    if (s.dma.direction == e2l1D || s.dma.direction == e2l2D) 
                        dir_switch = (dir != E2L);
                }
            }

            bool noblock_cmd = false;
            if (s.type.type == DMA_CMD && !dir_switch) {
                dma_block.append(s);
            } else if (s.type.type == VPRO_CMD) {
                vpro_block.append(s);
            } else {
                noblock_cmd = true;
            }

            if (dir_switch || noblock_cmd || i == command_list.size() - 1) {    // sync
                if (!dma_block.empty()) {
                    if (dma_block.size() > 1) {
                        // Create cluster broadcasts, therefore use sort to generate a mask from cluster IDs
                        std::stable_sort(dma_block.begin(), dma_block.end(),
                                [](const BIF::COMMAND_SEGMENT &d1, const BIF::COMMAND_SEGMENT &d2) -> bool {
                                    // direction hold 4 states (but '1 bit is ~ dir)
                                    assert((d1.dma.direction & 0b10) == (d2.dma.direction & 0b10));
                                    if (d1.dma.mm_addr != d2.dma.mm_addr) return d1.dma.mm_addr < d2.dma.mm_addr; // sort by main memory address
                                    if (d1.dma.lm_addr != d2.dma.lm_addr) return d1.dma.lm_addr < d2.dma.lm_addr; // sort by local memory address
                                    if (d1.dma.unit_mask != d2.dma.unit_mask) return d1.dma.unit_mask < d2.dma.unit_mask; // sort by local memory address
                                    return d1.dma.cluster < d2.dma.cluster; // sort by cluster
                                });

                        BIF::COMMAND_SEGMENT *sb = &(*(dma_block.begin()));
                        auto *starter = &sb->dma;
                        uint32_t cluster_mask = uint32_t(0b1u) << starter->cluster;
                        for (auto db = dma_block.begin(); db < dma_block.end(); db++) {
                            if (db == dma_block.begin()) continue;
                            auto *d = &db->dma;
                            if (d->mm_addr == starter->mm_addr &&
                                d->lm_addr == starter->lm_addr &&
                                d->x_size == starter->x_size &&
                                d->y_leap == starter->y_leap &&
                                d->y_size == starter->y_size &&
                                d->padding == starter->padding &&
                                d->unit_mask == starter->unit_mask &&
                                d->direction == starter->direction &&
                                d->isBiasOffset == starter->isBiasOffset &&
                                d->isKernelOffset == starter->isKernelOffset) {
                                assert(d->cluster != starter->cluster);
                                cluster_mask |= uint32_t(0b1u) << d->cluster;
                                merged_dma_commands++;
                            } else {
                                starter->cluster = cluster_mask;
                                dma_block_cluster_broadcasts.append(*sb);
                                
                                starter = d;
                                sb = &*db;
                                cluster_mask = uint32_t(0b1u) << d->cluster;
                            }
                        }
                        starter->cluster = cluster_mask;
                        dma_block_cluster_broadcasts.append(*sb);

                        // Inside this block, the largest DMAs should come first -> sort by DMA size
                        auto dma_comp = [](const BIF::COMMAND_SEGMENT &a, const BIF::COMMAND_SEGMENT &b) {
                            return a.dma.x_size * a.dma.y_size > b.dma.x_size * b.dma.y_size;
                        };

                        std::stable_sort(dma_block_cluster_broadcasts.begin(), dma_block_cluster_broadcasts.end(), dma_comp);

                        cmds_final.append(vpro_block);
                        if (!skip_dma_block_gen)
                            cmds_final.append(generate_DMABlock_Command(dma_block_cluster_broadcasts.size()));
                        cmds_final.append(dma_block_cluster_broadcasts);

                        if (!dma_block_cluster_broadcasts.empty()) {
                            CMD_BLOCK block;
                            block.type = COMMAND_SEGMENT_TYPE::DMA_CMD;
                            block.list = dma_block_cluster_broadcasts;
                            block_list.append(block);
                        }

                        dma_block_cluster_broadcasts.clear();
                    } else { // dma block size > 1
                        // set to cluster mask [ATTENTION, if this is a single dma command -> error]
                        dma_block.front().dma.cluster = uint32_t(0b1u) << dma_block.front().dma.cluster;
                        cmds_final.append(dma_block);
                        cmds_final.append(vpro_block);
                        if (!dma_block.empty()) {
                            CMD_BLOCK block;
                            block.type = COMMAND_SEGMENT_TYPE::DMA_CMD;
                            block.list = dma_block;
                            block_list.append(block);
                        }
                    }
                } else {   // dma not empty
                    cmds_final.append(vpro_block);
                }

                if (!vpro_block.empty()) {
                    CMD_BLOCK block;
                    block.type = COMMAND_SEGMENT_TYPE::VPRO_CMD;
                    block.list = vpro_block;
                    block_list.append(block);
                }
                if (!dir_switch) {
                    CMD_BLOCK block;
                    block.type = s.type.type;
                    block.list.append(s);
                    block_list.append(block);
                }

                dma_block.clear();
                vpro_block.clear();

                if (!dir_switch) {  // this is a sync or final command (not started this by dir switch)
                    cmds_final.append(s);
                    dir = UNINIT;
                } else {
                    assert(s.type.type == COMMAND_SEGMENT_TYPE::DMA_CMD);
                    if (s.dma.direction == e2l1D || s.dma.direction == e2l2D)
                        dir = E2L;
                    else
                        dir = L2E;
                    dma_block.append(s);
                }
                dir_switch = false;
            }
        }

        cmds_final.clear();
        // assuming regular structure:
        //  DMA + VPRO (parallel) or DMA (only)
        //  Sync (both)
        // loop
        struct ParallelBlock {
            CMD_BLOCK dma;
            CMD_BLOCK vpro;
            bool got_dma = false;
            bool got_vpro = false;
        } pblock;

        for (const auto &i: block_list) {
            if (i.type == COMMAND_SEGMENT_TYPE::DMA_CMD) {
                if (pblock.got_dma) {
                    pblock.dma.list.append(i.list);
                } else {
                    pblock.dma = i;
                }
                pblock.got_dma = true;
            } else if (i.type == COMMAND_SEGMENT_TYPE::VPRO_CMD) {
                if (pblock.got_dma) {
                    pblock.vpro.list.append(i.list);
                } else {
                    pblock.vpro = i;
                }
                pblock.got_vpro = true;
            } else if (i.type == COMMAND_SEGMENT_TYPE::VPRO_WAIT || i.type == COMMAND_SEGMENT_TYPE::DMA_WAIT || i.type == COMMAND_SEGMENT_TYPE::BOTH_SYNC || i.type == DMA_SET_PADDING) {
                if (pblock.got_dma && !pblock.got_vpro) { // only DMA
                    if (!pblock.dma.list.empty()) {
                        if (!skip_dma_block_gen)
                            cmds_final.append(generate_DMABlock_Command(pblock.dma.list.size()));
                    }
                    cmds_final.append(pblock.dma.list);
                } else if (pblock.got_dma && pblock.got_vpro) { // both

                    // "interleave":
                    //    x' block of dma
                    //    all vpro
                    //    remaining dmas
                    QList<BIF::COMMAND_SEGMENT> first_block_dmas;
                    for (int j = 0; j < dma_block_size; ++j) {
                        if (!pblock.dma.list.empty()) {
                            first_block_dmas.append(pblock.dma.list.front());
                            pblock.dma.list.pop_front();
                        } else {
                            break;
                        }
                    }
                    //printf_warning("Block of %i DMAs! %i Remain\n", first_block_dmas.size(), pblock.dma.list.size());

                    if (!first_block_dmas.empty()) {
                        if (!skip_dma_block_gen)
                            cmds_final.append(generate_DMABlock_Command(first_block_dmas.size()));
                    }
                    cmds_final.append(first_block_dmas);
                    cmds_final.append(pblock.vpro.list);
                    if (!pblock.dma.list.empty()) {
                        if (!skip_dma_block_gen)
                            cmds_final.append(generate_DMABlock_Command(pblock.dma.list.size()));
                    }
                    cmds_final.append(pblock.dma.list);
                } else if (!pblock.got_dma && pblock.got_vpro) {
                    cmds_final.append(pblock.vpro.list);
                }
                cmds_final.append(i.list);  // sync
                pblock.dma.list.clear();
                pblock.vpro.list.clear();
                pblock.got_dma = false;
                pblock.got_vpro = false;
            } else {printf/*_error*/("unknown segment type.type!\n"); }
        }


        int total_dma_blocks = 0;
        for (const auto &i: cmds_final) {
            if (i.type.type == COMMAND_SEGMENT_TYPE::DMA_BLOCK)
                total_dma_blocks++;
        }

        printf("  [DMA Block Extraction | Cluster Broadcasting] %i DMA Blocks\n"\
               "      %i Commands total (-%i DMA Commands by Cluster Broadcasting; -%2.2f%%)\n",
               total_dma_blocks,
               cmds_final.size(), merged_dma_commands, 100*float(merged_dma_commands)/float(cmds_final.size() + merged_dma_commands));

        return std::vector<BIF::COMMAND_SEGMENT> (cmds_final.begin(), cmds_final.end());
    }


private:
    BIF::COMMAND_SEGMENT generate_DMABlock_Command(uint32_t count) {
        BIF::COMMAND_SEGMENT seg;
        seg.type.type = DMA_BLOCK;
        seg.dma.unit_mask = count;
        return seg;
    }

    QList<BIF::COMMAND_SEGMENT> command_list;
    int dma_block_size;
};


#endif // DMA_BLOCK_EXTENSION_H
