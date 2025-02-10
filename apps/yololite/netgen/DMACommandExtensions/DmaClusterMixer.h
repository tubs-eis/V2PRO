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

#ifndef NETGEN_DMACLUSTERMIXER_H
#define NETGEN_DMACLUSTERMIXER_H

#include "bif.h"
#include <list>
#include <vector>

class DmaClusterMixer {

public:
    /**
     * Goal: increase the performance of dma execution
     * Problem: large blocks of > 32 commands are larger than hardware dma cmd fifo depth
     *      If fifo overflows, not all dma units are busy
     * Solution: Sort blocks to fill all clusters/dmas first
     * @param cmd_list
     */
    explicit DmaClusterMixer(std::vector<BIF::COMMAND_SEGMENT> &cmd_list) : cmd_list(cmd_list) {}

    std::vector<BIF::COMMAND_SEGMENT> generate(bool debug = false);

private:
    std::vector<BIF::COMMAND_SEGMENT> &cmd_list;

    void find_block(std::vector<BIF::COMMAND_SEGMENT>::iterator &block_begin, std::vector<BIF::COMMAND_SEGMENT>::iterator &block_end);

    bool is_simple_dma(std::vector<BIF::COMMAND_SEGMENT>::iterator &cmd);

    bool is_loop(std::vector<BIF::COMMAND_SEGMENT>::iterator &cmd);
};


#endif //NETGEN_DMACLUSTERMIXER_H
