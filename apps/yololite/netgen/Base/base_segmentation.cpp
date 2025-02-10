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
#include "base_layer.h"
#include "segment.h"
#include "vpro_globals.h"

namespace CNN_LAYER {

SEGMENT *Layer::getSegment(int x, int y, int in_ch, int out_ch) {
    SEGMENT *segment = new SEGMENT;
    segment->x_seg = x;
    segment->y_seg = y;
    segment->out_channel = out_ch;
    segment->in_channel = in_ch;

    // if first input channel, bias needs to be loaded additionally
    segment->isFirst = (in_ch == firstInputChannel(x, y, out_ch));
    // if last input channel, store result to main memory
    segment->isLast = (in_ch == lastInputChannel(x, y, out_ch));
    segment->dummy = false;

    // main memory address of top left segment corner (padded)
    // default: compute for all inputs
    for (int src_idx = 0; src_idx < (int)src_layers.size(); src_idx++) {
        segment->in_MM_base.push_back(padded_in_MM_base(src_idx, in_ch) +
                /* 16 bit elements */ 2 * (x * seg.in.x_stride + y * seg.in.y_stride * in_dim(src_idx).mm.x));
        segment->in_MM_y_stride.push_back(in_dim(src_idx).mm.x);
    }

    // main memory address of segment result
    segment->out_MM_base = out_dim.mm.channel_base[out_ch] +
    /* 16 bit elements */  2 * (x * seg.out.x_stride + y * seg.out.y_stride * out_dim.mm.x);
    segment->out_MM_y_stride = out_dim.mm.x;

    // compute padding widths for this segment
    if (padding.enabled) {
        BIF::PAD_REDUCED pw;
        pw.top = std::max(0, padding.dma.top - y*seg.in.y_stride);
        pw.right = std::max(0, padding.dma.right - (seg.num.x-1-x)*seg.in.x_stride);
        pw.bottom = std::max(0, padding.dma.bottom - (seg.num.y-1-y)*seg.in.y_stride);
        pw.left = std::max(0, padding.dma.left - x*seg.in.x_stride);

        // catch e.g. padding > seg.in.stride
        assert(pw.top == 0 || pw.top == padding.dma.top);
        assert(pw.right == 0 || pw.right == padding.dma.right);
        assert(pw.bottom == 0 || pw.bottom == padding.dma.bottom);
        assert(pw.left == 0 || pw.left == padding.dma.left);

        // switch padding on/off per segment
        segment->pad_top = pw.top > 0;
        segment->pad_right = pw.right > 0;
        segment->pad_bottom = pw.bottom > 0;
        segment->pad_left = pw.left > 0;
    }

    return segment;
}

void Layer::setSegmentDimensions() {
    // default: one segment for whole image, output size = input size
    seg.num.x = 1;
    seg.num.y = 1;
    if (!src_layers.size()) {
        seg.in.w = 0;
        seg.in.h = 0;
    } else {
        seg.in.w = in_dim(0).x;
        seg.in.h = in_dim(0).y;
    }

    seg.out.w = out_dim.x;
    seg.out.h = out_dim.y;
}

void Layer::setOutputMemDimensions() {
    out_dim.mm.x = seg.out.w + (seg.num.x-1)*seg.out.x_stride;
    out_dim.mm.y = seg.out.h + (seg.num.y-1)*seg.out.y_stride;
}

// fill out_dim.mm based on single channel memory dimensions
void Layer::calcOutputMemLayout() { // setOutputMMAccessor() {
    out_dim.mm.ch_size = 2 * out_dim.mm.x * out_dim.mm.y;

    out_dim.mm.channel_base = std::vector<mm_addr_type>(out_dim.ch);
    for (int oc = 0; oc < out_dim.ch; oc++) {
      out_dim.mm.channel_base[oc] = out_dim.mm.base + oc * out_dim.mm.ch_size;
    }

    out_dim.mm.size = out_dim.ch * out_dim.mm.ch_size;
}

void Layer::generateSegments() {
    // This function is responsible for
    // - distributing segments to the individual hardware elements (lanes), i.e. creating a batch of segments for each lane
    // - inserting dummy segments to ensure all lanes receive a batch of the same length
    // - generating a vector of the layer's segments with following layout:
    //   seg 0 lane 0, seg 0 lane 1, ..., seg 0 lane n-1, seg 1 lane 0, ... , seg m-1 lane n-1
    //   where n is the number of parallel lanes and m is the batch length

    assert(parallel_inchannels_per_lane == 1); // TODO

    assert(out_dim.mm.layout_known && "Main memory address for this layer's output has not been set! Call setOutputMMAddr()!");
    for (auto l: src_layers) {
        assert(l->out_dim.mm.layout_known && "Main memory address for input of this layer has not been set! Call setOutputMMAddr() on all input layers!");
    }
    std::vector<SEGMENT *> set; // for all lanes, use this initial set of segments
    segments.reserve(seg.num.x * seg.num.y * out_dim.ch * in_dim(0).ch);

    int appended_segs = 0, appended_dummies = 0;   // counters. final added segments for execution

    // get paralellLanes * n segments [same inc, preferred is same block, diff outc]
    // loop ics when all lanes have a segment
    std::list<SEGMENT *> seg_blocks;

    if (layercfg.scheduling_order == ITERATE_ALL_SORTED_X) {
        for (int c_start = 0; c_start < out_dim.ch; c_start += parallel_outchannels_per_lane * (int)VPRO_CFG::parallel_Lanes) {
            for (int x = 0; x < seg.num.x; ++x) {
                for (int y = 0; y < seg.num.y; ++y) {
                    for (int out_ch = c_start; out_ch < c_start + parallel_outchannels_per_lane * (int)VPRO_CFG::parallel_Lanes && out_ch < out_dim.ch; ++out_ch) {
                        // new location(x,y) after every (VPRO_CFG::parallel_Lanes) outc set
                        seg_blocks.emplace_back(getSegment(x, y, firstInputChannel(x, y, out_ch), out_ch));
                    }
                }
            }
        }
    } else if (layercfg.scheduling_order == ITERATE_ALL_SORTED_X2) {
        for (int c_start = 0; c_start < out_dim.ch; c_start += parallel_outchannels_per_lane * (int)VPRO_CFG::LANES) {
            for (int x = 0; x < seg.num.x; ++x) {
                for (int y = 0; y < seg.num.y; ++y) {
                    for (int out_ch = c_start; out_ch < c_start + parallel_outchannels_per_lane * (int)VPRO_CFG::LANES && out_ch < out_dim.ch; ++out_ch) {
                        // new location(x,y) after every (parallel_outchannels_per_lane * VPRO_CFG::LANES) outc set
                        seg_blocks.emplace_back(getSegment(x, y, firstInputChannel(x, y, out_ch), out_ch));
                    }
                }
            }
        }
    } else {    // default
        for (int y = 0; y < seg.num.y; ++y) {
            for (int x = 0; x < seg.num.x; ++x) {
                for (int out_ch = 0; out_ch < out_dim.ch; ++out_ch) {
                    seg_blocks.emplace_back(getSegment(x, y, firstInputChannel(x, y, out_ch), out_ch));
                }
            }
        }
    }

    while (!seg_blocks.empty()) {

        // new set for all lanes
        set.clear();

        bool n_fill_with_dummies = false; // only for parallel_outchannels_per_lane > 1
        for (uint lane = 0; lane < VPRO_CFG::parallel_Lanes; lane++) {
            if (lane % VPRO_CFG::LANES == 0) { // fill with dummies until next unit (new local memory)
                n_fill_with_dummies = false;
            }
            for (int n_iteration = 0; n_iteration < parallel_outchannels_per_lane; n_iteration++) { // parallel outc per lane
                if (seg_blocks.empty() || // no remaining segments
                    n_fill_with_dummies ||  // fill all remaining parallel_outchannels_per_lane
                    !compatibleSegmentsBlock(seg_blocks.front(), set.size() ? set.back() : NULL, lane, n_iteration)) { // different input data -> force new unit
                    set.push_back(new DUMMY_SEGMENT(*set.back()));
                    n_fill_with_dummies = true;
                    continue;
                }

                set.push_back(seg_blocks.front());
                seg_blocks.pop_front();
            }
        }
        assert(set.size() == VPRO_CFG::parallel_Lanes * parallel_outchannels_per_lane);
        insertRepeatedSegmentSetForAllInChannels(set, appended_segs, appended_dummies);
    }   // all segments added
    
    // sanity check for number of segments
    if (segments.size() % VPRO_CFG::parallel_Lanes != 0 || (int)segments.size() != appended_segs + appended_dummies) {
        std::cout << "Generated " << segments.size() << " segments (" << appended_dummies << " dummies + " << appended_segs << ") for layer "
                  << name //<< ", expected " << expected_segment_count
                  << " (seg.num.x = " << seg.num.x << ", seg.num.y = " << seg.num.y << ", in_dim(0).ch = " << in_dim(0).ch << ", out_dim.ch = " << out_dim.ch << ")\n";
        assert(0);
    }
}

// can these segments be placed into the same unit?
bool Layer::compatibleSegmentsBlock(const SEGMENT *a, const SEGMENT *b, int lane, int lane_out_ch) const {
    if (lane % VPRO_CFG::LANES == 0 && lane_out_ch == 0) // first location has no dependencies
        return true;

    if (!a || !b) {
        return true; // there is no segment to compare to
    }
    if (a->dummy || b->dummy) {
        return true; // dummy segments compatible to everything
    }

    // segments are considered compatible if their inputs are identical
    // same number of input channels?
    if (a->in_MM_base.size() != b->in_MM_base.size()) {
        return false;
    }

    // same input data on all channels?
    for (int ch = 0; ch < (int)a->in_MM_base.size(); ch++)
        if (a->in_MM_base[ch] != b->in_MM_base[ch] ||
            a->in_MM_y_stride[ch] != b->in_MM_y_stride[ch]) {
                return false;
            }

    if (a->pad_top != b->pad_top ||
        a->pad_right != b->pad_right ||
        a->pad_bottom != b->pad_bottom ||
        a->pad_left != b->pad_left) {
            return false;
        }

    return true;
}

void Layer::insertRepeatedSegmentSetForAllInChannels(std::vector<SEGMENT *> &set, int &appended_segs, int &appended_dummies) {
    // add segments to final list for all ics
    // set: s[oc = 0, x = 0, y = 0] | s[1, Dummy] | s[2]        ---- s[127]

    //        ic = 0                | DDD  ...    | ic = 2
    //        ic = 1                | DDD  ...    | ic = 3

    int num_sets = 0;
    int cont_requests; // number of lanes requesting another segment (i.e. more input channels)
    int stop_requests; // number of lanes that are done after this set
    do {
        cont_requests = 0;
        stop_requests = 0;
        for (SEGMENT *s: set) { // for all lanes one segment (missing: in_channel)
            if (s->dummy) {
                appended_dummies++;
                segments.push_back(new DUMMY_SEGMENT());
            } else {
                auto newSeg = getSegment(s->x_seg, s->y_seg, s->in_channel, s->out_channel);
                s->in_channel = nextInputChannel(s->x_seg, s->y_seg, s->in_channel, s->out_channel);
                assert((num_sets > 0 || newSeg->isFirst) && "getSegment(..., firstInputChannel(), ...) returned a segment with isFirst==false");
                assert((newSeg->isLast == (s->in_channel < 0)) && "nextInputChannel and isLast are inconsistent");
                if (newSeg->isLast)
                    stop_requests++;
                else
                    cont_requests++;
                appended_segs++;
                segments.push_back(newSeg);
            }
        }
        assert(!(cont_requests && stop_requests) && "Some non-dummy segments in this set are isLast, some are not");
        num_sets++;
    } while (cont_requests);
}

// lowest index of any input channel used by output channel
int Layer::firstInputChannel(int x, int y, int out_ch, int src_idx) {
    assert((out_ch >= 0 && out_ch < out_dim.ch) && "out_ch out of range");
    int in_group_len = in_dim(src_idx).ch / groups; // input channels per group
    int out_group_len = out_dim.ch / groups; // output channels per group
    int out_group = out_ch / out_group_len;
    return out_group * in_group_len;
}

// highest index of any input channel used by output channel
int Layer::lastInputChannel(int x, int y, int out_ch, int src_idx) {
    assert((out_ch >= 0 && out_ch < out_dim.ch) && "out_ch out of range");
    int in_group_len = in_dim(src_idx).ch / groups; // input channels per group
    int out_group_len = out_dim.ch / groups; // output channels per group
    int out_group = out_ch / out_group_len;
    return out_group * in_group_len + (in_group_len - 1);
}

// iterate to next used input channel; return -1: no more input channels
int Layer::nextInputChannel(int x, int y, int in_ch, int out_ch, int src_idx) {
    assert((in_ch >= 0 && in_ch < in_dim(src_idx).ch) && "in_ch out of range");
    assert((out_ch >= 0 && out_ch < out_dim.ch) && "out_ch out of range");
    do {
        in_ch++;
        if (in_ch == in_dim(src_idx).ch) // no more input channels available
            return -1;
    } while (!usesInputCh(x, y, in_ch, out_ch, src_idx));
    return in_ch;
}

// total number of input channels used by output channel (there might be gaps between first and lastInputChannel)
int Layer::numUsedInputChannels(int x, int y, int out_ch, int src_idx) {
    assert((out_ch >= 0 && out_ch < out_dim.ch) && "out_ch out of range");
    int in_group_len = in_dim(src_idx).ch / groups; // input channels per group
    return in_group_len;
}

// is input channel in_ch used by out_ch?
bool Layer::usesInputCh(int x, int y, int in_ch, int out_ch, int src_idx) {
    assert((in_ch >= 0 && in_ch < in_dim(src_idx).ch) && "in_ch out of range");
    assert((out_ch >= 0 && out_ch < out_dim.ch) && "out_ch out of range");
    int in_group_len = in_dim(src_idx).ch / groups; // input channels per group
    int out_group_len = out_dim.ch / groups; // output channels per group
    int in_group = in_ch / in_group_len;
    int out_group = out_ch / out_group_len;
    //    std::cout << "in_group_len " << in_group_len << ", out_group_len " << out_group_len << ", in_group " << in_group << ", out_group " << out_group << "\n";
    return in_group == out_group;
}


}; // namespace CNN_LAYER
