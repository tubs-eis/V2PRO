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

#ifndef CNN_CONVERTER_DMAMERGER_H
#define CNN_CONVERTER_DMAMERGER_H

#include "bif.h"
#include <list>
#include <utility>
#include <functional>

class DmaMerger {

public:
    explicit DmaMerger(std::vector<BIF::COMMAND_SEGMENT> &cmd_list) : cmd_list(cmd_list) {}

    std::vector<BIF::COMMAND_SEGMENT> generate(bool debug = false);

private:
    void merge_same_commands();
    void transform_2d_to_1d();

    void reduce_1d_by_overcalc();

    void merge_sequential_1ds();
    void merge_sequential_2ds();

    uint64_t count_dma_transfers();

    /**
     * using internal list (new_list) for data input
     * @param sortfunction
     */
    void reorderDMAs(const std::function<bool(const BIF::COMMAND_SEGMENT&, const BIF::COMMAND_SEGMENT&)>& sortfunction);

    bool debug{false};

    std::vector<BIF::COMMAND_SEGMENT> &cmd_list;
    std::list<BIF::COMMAND_SEGMENT> new_list;

    int dma_1d_merges{0}, dma_2d_merges{0};
};


#endif //CNN_CONVERTER_DMAMERGER_H
