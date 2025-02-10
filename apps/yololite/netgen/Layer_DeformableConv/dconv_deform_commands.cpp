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

#include "dconv_deform_layer.h"
#include "bif.h"

namespace CNN_LAYER {

DMA_COMMANDS::DMA_DESCRIPTOR DConvDeform::staticOffsetLoad(int cluster, int unit) {
    DMA_COMMANDS::DMA_DESCRIPTOR dma;
    dma.dir = e2l1D;
    dma.cluster = cluster;
    dma.unit = unit;
    dma.word_count = lm_static_offset_size();
    dma.lm_addr = lm_static_offset_addr();
    dma.mm_addr = getWeightsMMAddr();

    return dma;
}

DMA_COMMANDS::DMA_DESCRIPTOR DConvDeform::inputLoad(const SEGMENT &segment, BIF::PAD_REDUCED pad, int cluster, int unit, BUFFER buffer) {
    DMA_COMMANDS::DMA_DESCRIPTOR dma = dataLoad(segment, cluster, unit, buffer, INPUT_SRC_INDEX);

    Dim const& input_dim = getInputDim();

    // Adjust for extra input data
    dma.mm_addr -= 2 * (max_offset_x - pad.left);
    dma.mm_addr -= 2 * (max_offset_y - pad.top) * (input_dim.mm.x);
    dma.x_size += 2 * max_offset_x;
    dma.y_size += 2 * max_offset_x;
    dma.y_leap -= (2 * max_offset_x - pad.left - pad.right);

    // Overwrite lm_addr to adjust for static offset
    dma.lm_addr = lm_input_addr(buffer);

    return dma;
}

// Interleaves the rows at y=row for all input channels
DMA_COMMANDS::DMA_DESCRIPTOR DConvDeform::offsetLoad(const SEGMENT &segment, int row, int cluster, int unit, BUFFER buffer) {
    Dim offset_dim = getOffsetDim();

    DMA_COMMANDS::DMA_DESCRIPTOR dma;
    dma.dir = e2l2D;
    dma.cluster = cluster;
    dma.unit = unit;
    dma.x_size = seg.in.w;
    dma.y_size = offset_dim.ch;
    dma.y_leap = segment.in_MM_y_stride[OFFSET_SRC_INDEX] - dma.x_size + 1;
    dma.mm_addr = segment.in_MM_base[OFFSET_SRC_INDEX] + 2 * (row * offset_dim.x);
    dma.lm_addr = lm_offset_addr(buffer) + row * seg.in.w * offset_dim.ch;

    return dma;
}

BIF::COMMAND_SEGMENT DConvDeform::dataStore(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER &buffer_load) {
    BIF::COMMAND_SEGMENT cmd = Layer::dataStore(segment, cluster, unit, lane, buffer_load);

    cmd.dma.unit_mask = 1 << unit;
    cmd.dma.lm_addr = lm_output_addr(buffer_load);

    return cmd;
}

BIF::COMMAND_SEGMENT DConvDeform::setDMAPadding(BIF::PAD_REDUCED pad) {
    BIF::COMMAND_SEGMENT cmd;

    cmd.type.type = DMA_SET_PADDING;
    cmd.dma_padding.pad = pad;

    return cmd;
}

BIF::COMMAND_SEGMENT DConvDeform::dconvDeformVPRO(const CNN_LAYER::SEGMENT &segment, BUFFER &buffer) {
    BIF::COMMAND_SEGMENT cmd;
    cmd.type.type = VPRO_CMD;
    cmd.vpro.command = VPRO_TYPE::dconv_deform_8x8;

    cmd.vpro.buffer = lm_input_addr(buffer);
    cmd.vpro.deform_offset_buffer = lm_offset_addr(buffer);
    cmd.vpro.deform_output_buffer = lm_output_addr(buffer);

    return cmd;
}

void DConvDeform::load(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer) {
    // Find current x/y location and setup padding based on it
    // new_location_min_lane_alignment() ensures that x/y will be constant, so take the first valid segment
    int batch_x_seg = -1;
    int batch_y_seg = -1;
    for (unsigned int i = 0; i < VPRO_CFG::parallel_Lanes; i++) {
        SEGMENT &segment = *segments[i + seg_cnt];
        if (!segment.dummy) {
            batch_x_seg = segment.x_seg;
            batch_y_seg = segment.y_seg;
            break;
        }
    }
    BIF::PAD_REDUCED pad;
    if (batch_x_seg != -1 && batch_y_seg != -1) {
        int x = batch_x_seg * seg.in.w;
        int y = batch_y_seg * seg.in.h;

        // Assume all parts of output segment are completly valid
        // TODO(Jasper): This might be wrong for certain inputs. How does conv deal WITH OOB segments

        int x_padding = max_offset_x;
        int y_padding = max_offset_y;

        Dim const& input_dim = getInputDim();

        pad.left = std::max(x_padding - x, 0);
        pad.right = std::max(x + seg.in.w + x_padding - input_dim.x, 0);

        pad.top = std::max(y_padding - y, 0);
        pad.bottom = std::max(y + seg.in.h + y_padding - input_dim.y, 0);

        commands.push_back(setDMAPadding(pad));
    }

    // Generate DMA commands for loading inputs and offsets
    std::vector<DMA_COMMANDS::DMA_DESCRIPTOR> dmas_1d, dmas_2d;
    unsigned int cl=0, un=0, ln=0;
    for (unsigned int i = 0; i < VPRO_CFG::parallel_Lanes; i++) {
        SEGMENT &segment = *segments[i + seg_cnt];
        if (ln == 0) { // Deform uses both lanes, only schedule to lane 0
            if (seg_cnt == 0) {
                // First segment, upload static offsets
                dmas_1d.push_back(staticOffsetLoad(cl, un));
            }

            if (!segment.dummy) {
                // Load input
                dmas_2d.push_back(inputLoad(segment, pad, cl, un, buffer));

                // Load offsets
                for (int row = 0; row < seg.out.h; row++) {
                    dmas_2d.push_back(offsetLoad(segment, row, cl, un, buffer));
                }
            }
        } else {
            assert(segment.dummy);
        }

        next_hardware_element(cl, un, ln);
    }

    auto dma_commands = DMA_COMMANDS::DMA_DESCRIPTOR::startBroadcastLoad(dmas_1d, dmas_2d);
    cmd_cnt.dma += (int) dma_commands.size();
    commands.insert(std::end(commands), std::begin(dma_commands), std::end(dma_commands));
}

void DConvDeform::compute(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer) {
    SEGMENT &segment = *segments[seg_cnt];

    commands.push_back(dconvDeformVPRO(segment, buffer));
}

void DConvDeform::store(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer) {
    unsigned int cl=0, un=0, ln=0;
    for (unsigned int i = 0; i < VPRO_CFG::parallel_Lanes; i++) {
        SEGMENT &segment = *segments[i + seg_cnt];
        if (ln == 0) { // Deform uses both lanes, only schedule to lane 0
            if (!segment.dummy) {
                cmd_cnt.dma++;
                commands.push_back(dataStore(segment, cl, un, ln, buffer));
            }
        } else {
            assert(segment.dummy);
        }
        next_hardware_element(cl, un, ln);
    }
}

uint16_t DConvDeform::lm_input_segment_width() const {
    return seg.in.w + 2 * max_offset_x;
}

uint16_t DConvDeform::lm_input_segment_height() const{
    return seg.in.h + 2 * max_offset_y;
}

uint16_t DConvDeform::lm_input_size() const{
    return lm_input_segment_width() * lm_input_segment_height();
}

uint16_t DConvDeform::lm_offset_size() const{
    return seg.in.w * seg.in.h * 3 * kernel_size;
}

uint16_t DConvDeform::lm_output_size() const{
    return seg.out.w * seg.out.h;
}

uint16_t DConvDeform::lm_static_offset_size() const{
    return seg.in.w * seg.in.h * kernel_size;
}

uint16_t DConvDeform::lm_input_addr(BUFFER buffer) const {
    return int(buffer) * lm_input_size();
}

uint16_t DConvDeform::lm_offset_addr(BUFFER buffer) const {
    return 2 * lm_input_size() + int(buffer) * lm_offset_size();
}

uint16_t DConvDeform::lm_output_addr(BUFFER buffer) const {
    return 2 * (lm_input_size() + lm_offset_size()) + int(buffer) * lm_output_size();
}

uint16_t DConvDeform::lm_static_offset_addr() const {
    return 2 * (lm_input_size() + lm_offset_size() + lm_output_size());
}

}; // namespace CNN_LAYER
