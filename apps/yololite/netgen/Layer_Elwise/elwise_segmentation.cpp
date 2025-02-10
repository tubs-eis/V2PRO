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
 #include "elwise_layer.h"
 #include "Base/segment.h"
 #include "vpro_globals.h"

namespace CNN_LAYER {

void Elementwise::setSegmentDimensions() {
    
    // 1024 RF entries (no weights required)
    int rf_free_entries = RF_DISCARD_ADDR;

    if (activation == RELU6) {
        rf_free_entries -= 1; // one entry required for shifted six
    }

    // 2048 LM entries (double buffering)
    int lm_free_entries = VPRO_CFG::LM_SIZE / 4;
    int lm_in_seg_max = floor(sqrt(lm_free_entries));

    int max_beta = 31;
    int max_xend_yend = 31;
    
    lm_in_seg_max = std::min(max_beta, lm_in_seg_max);

    int rf_out_seg_max = std::min(lm_in_seg_max, int(floor(sqrt(rf_free_entries))));
    rf_out_seg_max = std::min(rf_out_seg_max, max_xend_yend+1);

    seg.num.x = std::max(ceil_div(out_dim.x, rf_out_seg_max),
                         ceil_div(in_dim(0).x, lm_in_seg_max));
    seg.num.y = std::max(ceil_div(out_dim.y, rf_out_seg_max),
                         ceil_div(in_dim(0).y, lm_in_seg_max));

    seg.out.w = ceil_div(out_dim.x, seg.num.x);
    seg.out.h = ceil_div(out_dim.y, seg.num.y);
    seg.in.w = seg.out.w;
    seg.in.h = seg.out.h;
 }

SEGMENT *Elementwise::getSegment(int x, int y, int in_ch, int out_ch) {
    SEGMENT *segment = FusedFunc::getSegment(x, y, in_ch, out_ch);

    //    segment->isFirst = true;
    //    segment->isLast = true;
     
    // adjust source addresses for broadcasting
    for (int src_idx = 0; src_idx < (int)src_layers.size(); src_idx++) {
        in_ch = bc_ch(src_idx) ? 0 : out_ch; // function parameter in_ch can not represent channels for both inputs in the broadcasting case -> derive from output channel
        segment->in_MM_base[src_idx] = padded_in_MM_base(src_idx, in_ch) + 2 * (x * (bc_x(src_idx) ? 0 : seg.in.x_stride) + y * (bc_y(src_idx) ? 0 : seg.in.y_stride) * in_dim(src_idx).mm.x);
    }

    return segment;
}


// can these segments be placed into the same unit?
bool Elementwise::compatibleSegmentsBlock(const SEGMENT *a, const SEGMENT *b, int lane, int lane_out_ch) const {
    if (!a || !b)
        return true; // there is no segment to compare to
    if (a->dummy || b->dummy)
        return true; // dummy segments compatible to everything

    return lane % VPRO_CFG::LANES == 0; // use L0 only
}

// lowest index of any input channel used by output channel
int Elementwise::firstInputChannel(int x, int y, int out_ch, int src_idx) {
    return -1; // not used; sources may need different input channels due to broadcasting, handled in overwritten getSegment()
}

// highest index of any input channel used by output channel
int Elementwise::lastInputChannel(int x, int y, int out_ch, int src_idx) {
    return -1; // not used
}

// iterate to next used input channel; return -1: no more input channels
int Elementwise::nextInputChannel(int x, int y, int in_ch, int out_ch, int src_idx) {
    return -1; // always exactly one input channel per output channel
}

// total number of input channels used by output channel (there might be gaps between first and lastInputChannel)
int Elementwise::numUsedInputChannels(int x, int y, int out_ch, int src_idx) {
    return 1;
}

// is input channel in_ch used by out_ch?
bool Elementwise::usesInputCh(int x, int y, int in_ch, int out_ch, int src_idx) {
    assert((in_ch >= 0 && in_ch < in_dim(src_idx).ch) && "in_ch out of range");
    assert((out_ch >= 0 && out_ch < out_dim.ch) && "out_ch out of range");
    return bc_ch(src_idx) ? in_ch == 0 : in_ch == out_ch;
}

  
} // namespace CNN_LAYER
