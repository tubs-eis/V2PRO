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

#ifndef CNN_CONVERTER_DMALOOPEXTENSION_H
#define CNN_CONVERTER_DMALOOPEXTENSION_H

#include "bif.h"
#include <list>
#include <utility>

class DmaLoopExtension {

public:
    explicit DmaLoopExtension(std::vector<BIF::COMMAND_SEGMENT> &cmd_list) : cmd_list(cmd_list) {}

    std::vector<BIF::COMMAND_SEGMENT> generate(bool debug = false);


private:
    const uint32_t minimal_count_of_instr_in_loop_to_generate_loop = 5;

    std::vector<BIF::COMMAND_SEGMENT> &cmd_list;

    uint total_loop_encoded_dmas{};
    uint total_dmas{};
    uint total_loops{};


    /**
     * Extract the parameters for a loop (if possible)
     * Generate List with loop command + base.
     * Commands not loop possible remain in returned list.
     *
     * @param cmd_list List of DMA Commands with identical size/stride/pad and direction
     * @return vector of size 2 (if all are merged in one loop), else up to input length command list
     */
    std::list<BIF::COMMAND_SEGMENT> extractLoopCommand(std::list<BIF::COMMAND_SEGMENT> same_dim_dma_list, bool debug = false);

    /**
     * Use parameters to generate the Loop Parameter Command
     * @param loop_cmd_list all commands to be generated inside the loop. First is the base
     * @param cluster_shift the shift amount of the cluster loop
     * @param unit_shift the shift amount of the unit loop
     * @param cluster_len cluster loop size
     * @param unit_len unit loop size
     * @param inter_unit_len inter unit loop size
     * @param mm_incr increment of mm address of each command
     * @param lm_incr increment of lm address inside unit loop (reset) of each command
     * @return The generated Command
     */
    BIF::COMMAND_SEGMENT generateLoopCommand(std::list<BIF::COMMAND_SEGMENT> &loop_cmd_list, uint8_t cluster_shift, uint8_t unit_shift, int cluster_len, int unit_len,
                                             int inter_unit_len, int mm_incr, int lm_incr, int count, bool debug = false);


    /**
     * Check the generated Loop Command
     * @param loop_cmd_list all commands to be generated inside the loop. First is the base
     * @param loop_cmd the loop command itself
     * @return whether the loop command correctly generates all dma cmds
     */
    bool verifyLoopCommand(std::list<BIF::COMMAND_SEGMENT> &loop_cmd_list, BIF::COMMAND_SEGMENT &loop_cmd, bool debug = false);

    /**
     * evaluation of a binary number -> count tailing zeros
     * @param value data
     * @return decimal count of the number of zeros
     */
    uint8_t get_tailing_zeros(const uint32_t value);

    /**
     * @param base_mask referenced mask (already shifted right (no '0 at right)
     * @param other_mask tested mask
     * @return pair of is_mask and shift amount of base mask to get other mask
     */
    std::pair<bool, uint8_t> is_modified_base_mask(const uint32_t base_mask, const uint32_t other_mask);

};


#endif //CNN_CONVERTER_DMALOOPEXTENSION_H
