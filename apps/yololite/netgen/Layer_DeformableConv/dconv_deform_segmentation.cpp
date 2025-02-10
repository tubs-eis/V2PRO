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
#include "dconv_deform_layer.h"
#include "Base/segment.h"
#include "vpro_globals.h"

namespace CNN_LAYER {

SEGMENT *DConvDeform::getSegment(int x, int y, int in_ch, int out_ch) {
    assert(in_ch == out_ch);

    auto segment = Layer::getSegment(x, y, in_ch, out_ch);
    segment->pad_top = true;
    segment->pad_right = true;
    segment->pad_bottom = true;
    segment->pad_left = true;

    // Offsets (Source 1)
    // Offsets are indexed across channels, therefore the channels need to be contiguous in memory
    Dim const& offset_dim = getOffsetDim();
    segment->in_MM_base[OFFSET_SRC_INDEX] = [&] {
        const int x_step = seg.in.w;
        const int y_step = seg.in.h * offset_dim.mm.x;
        return offset_dim.mm.base + 2 * (x * x_step + y * y_step);
    }();
    segment->in_MM_y_stride[OFFSET_SRC_INDEX] = offset_dim.y * offset_dim.mm.x;

    return segment;
}

void DConvDeform::setSegmentDimensions() {
    // Currently the deform operation is only implemented for 4x4 blocks,
    //  which is the maximum for the default RF size
    // For a MVP use a fixed output segment size of 8x8 (2x2 blocks)

    Dim const& input_dim = getInputDim();

    seg.out.w = 8;
    seg.out.h = 8;
    seg.in.w = seg.out.w;
    seg.in.h = seg.out.h;
    seg.num.x = ceil_div(input_dim.x, seg.in.w);
    seg.num.y = ceil_div(input_dim.y, seg.in.h);
    seg.out.w *= kernel_size;
}

bool DConvDeform::compatibleSegmentsBlock(const SEGMENT *a, const SEGMENT *b, int lane, int lane_out_ch) const {
    if (lane % VPRO_CFG::LANES != 0 && lane_out_ch == 0) // use L0 only
        return false;

    if (lane % VPRO_CFG::parallel_Lanes == 0) // first location has no dependencies
        return true;

    if (a->x_seg != b->x_seg)
       return false;
    if (a->y_seg != b->y_seg)
        return false;

    return true;
}

  
}; // namespace CNN_LAYER
