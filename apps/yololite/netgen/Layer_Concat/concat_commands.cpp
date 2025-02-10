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
#include "vpro_globals.h"
#include "vpro_cmd_defs.h"

#include "concat_layer.h"
#include "bif.h"



namespace CNN_LAYER {

void Concatenate::load(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer) {
    std::vector<DMA_COMMANDS::DMA_DESCRIPTOR> dmas_1d, dmas_2d;

    unsigned int un=0; // cluster, unit and lane of segment
    for (unsigned int cl = 0; cl < VPRO_CFG::CLUSTERS; cl++) {
        SEGMENT &segment = *segments[cl + seg_cnt];
        if (!segment.dummy) {
            // Load data for both inputs
            dmas_2d.push_back(dataLoad(segment, cl, un, buffer));
        }
    }

    auto dma_commands = DMA_COMMANDS::DMA_DESCRIPTOR::startBroadcastLoad(dmas_1d, dmas_2d);
    cmd_cnt.dma += (int) dma_commands.size();
    commands.insert(std::end(commands), std::begin(dma_commands), std::end(dma_commands));
}

bool Concatenate::concat_compute(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer) {

    //assert((seg.out.h == seg.out.w) && "Non quadratic segments are not implemented!");
    SEGMENT &segment = *segments[seg_cnt];
    if (!segment.dummy) {
    
        int src_layer = seg_to_src_map[seg_cnt];   
        if(in_shifts_right[src_layer] == 0) return false;

        cmd_cnt.vpro++;
        BIF::COMMAND_SEGMENT cmd;
        cmd.type.type = VPRO_CMD;
        cmd.vpro.command = concatenate;
        cmd.vpro.buffer = (int(buffer) * VPRO_CFG::LM_SIZE / 2);   // input
        cmd.vpro.offset = (int(buffer) * VPRO_CFG::LM_SIZE / 2) + VPRO_CFG::LM_SIZE / 4;  // store to different region (DMA interface)
        cmd.vpro.xend = seg.out.w - 1;
        cmd.vpro.yend = seg.out.h - 1;
        cmd.vpro.shift_right = in_shifts_right[src_layer];
        commands.push_back(cmd);
        return true;
    }
    return false;
}
    
void Concatenate::store(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer, bool isProcessed) {

    unsigned int un=0, ln=0; // cluster, unit and lane of segment
    for (unsigned int cl = 0; cl < VPRO_CFG::CLUSTERS; cl++) {
        SEGMENT &segment = *segments[cl + seg_cnt];
        if (!segment.dummy) {
            cmd_cnt.dma++;
            auto cmd = dataStore(segment, cl, un, ln, buffer);
            if (!isProcessed){
                cmd.dma.lm_addr -= VPRO_CFG::LM_SIZE / 4;
            }
            commands.push_back(cmd);
        }
    }
}


void Concatenate::generateCommands() {
//    Layer::generateCommands();
    // This function defines the default behavior for generating commands with double buffering.
    // Everything specific to the layer type should be implemented in the load(), compute() and store() functions.

    // FIXME DmaMerger fails with two identical DMA transfers. Is the error here or in DmaMerger?

    cmd_cnt = CMD_COUNT{};

    BUFFER buffer_load = BUFFER::A, buffer_calc = BUFFER::A;

    /*** first load ***/
    unsigned int seg_size = segments.size();

    /*** loop over all segments ***/
    bool isProcessed;
    for (uint curr_segments = 0; curr_segments < seg_size; curr_segments += VPRO_CFG::CLUSTERS) {
        load(segments, curr_segments, buffer_load);       // load next segments
        commands.push_back(DMA_COMMANDS::DMA_DESCRIPTOR::wait());
        cmd_cnt.sync++;

        isProcessed = concat_compute(segments, curr_segments, buffer_calc);    // compute current segments
        commands.push_back(VPRO_COMMANDS::sync());
        store(segments, curr_segments, buffer_calc, isProcessed);
        cmd_cnt.sync++;

        buffer_load = (buffer_load == A) ? B : A;
        buffer_calc = (buffer_calc == A) ? B : A;
    }
    commands.push_back(DMA_COMMANDS::DMA_DESCRIPTOR::wait());
}

}   // namespace CNN_LAYER





