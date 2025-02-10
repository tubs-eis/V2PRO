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

#ifndef NETGEN_DMASTORESPLITTER_H
#define NETGEN_DMASTORESPLITTER_H


#include "bif.h"
#include <list>
#include <utility>
#include <functional>

class DmaStoreSpliiter {

public:
    explicit DmaStoreSpliiter(std::vector<BIF::COMMAND_SEGMENT> &cmd_list) : cmd_list(cmd_list) {}

    std::vector<BIF::COMMAND_SEGMENT> generate();

private:
    bool debug{false};

    std::vector<BIF::COMMAND_SEGMENT> &cmd_list;
    std::list<BIF::COMMAND_SEGMENT> new_list;

    bool dma_cmd_block_contains_store(std::vector<BIF::COMMAND_SEGMENT>::iterator begin);

    void split_and_append(std::list<BIF::COMMAND_SEGMENT *> &segment);

    void collect_block(std::vector<BIF::COMMAND_SEGMENT>::iterator &cmd, std::list<BIF::COMMAND_SEGMENT *> &segment);

    void store_resort(std::list<BIF::COMMAND_SEGMENT *> &stores);

};

#endif //NETGEN_DMASTORESPLITTER_H
