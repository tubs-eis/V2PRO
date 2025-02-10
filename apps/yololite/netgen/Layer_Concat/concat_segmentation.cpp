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
#include "concat_layer.h"
#include "Base/segment.h"
#include "vpro_globals.h"

namespace CNN_LAYER {

void Concatenate::generateSegments(){
    // dummy segmente erzeugen und auf die volle anzahl 
    // der lanes auffüllen, damit diese gemeinsam und mit 
    // dem gleichen shift parameter gescheduled werden können
    // in einer batch muss der gleiche input layer mit dem 
    // gleichen shift verarbeitet werden.
    // alternativ pro batch mehrere commandos erzeugen. 
    // struktur andwers als bisher aber eig kein problem


    assert(out_dim.mm.layout_known && "Main memory address for this layer's output has not been set! Call setOutputMMAddr()!");
    for (auto l: src_layers) {
        (void)l;
        assert(l->out_dim.mm.layout_known && "Main memory address for input of this layer has not been set! Call setOutputMMAddr() on all input layers!");
    }   


    // batch (vector) of segments for each lane
    SegmentBatch cluster_segments = SegmentBatch(VPRO_CFG::CLUSTERS); 
    std::vector<SEGMENT *> *batch; 
    int cluster = 0;
    int appended_dummies = 0;

    // reserve the minimum number of segments to avoid frequent reallocations
    // reallocations may still happen, due to insertion of dummies
    int num_segs = seg.num.x * seg.num.y * out_dim.ch;
    int num_segs_per_cluster = ceil_div(num_segs, (int)VPRO_CFG::CLUSTERS);
    segments.reserve(num_segs_per_cluster * VPRO_CFG::CLUSTERS);

    // iterate over segments and lanes and cyclicly assign segments to lanes to 
    // fill lane-related batches  
    for (auto oc = 0; oc < out_dim.ch; oc++) {
        for (auto y = 0; y < seg.num.y; y++) {
            for (auto x = 0; x < seg.num.x; x++) {

                batch = &cluster_segments[cluster]; // get ref to lane-related batch
                cluster++;
                auto seg = getSegment(x, y, oc_to_ic_map[oc], oc); // ic = oc for concat layer 
                batch->push_back(seg);
                seg_to_src_map.push_back(oc_to_src_map[oc]);

                // to some lanes add dummies if necessary
                int inserted_dummies = 0;
                while (seg->isLast && cluster != 0 && (cluster % VPRO_CFG::CLUSTERS) != 0) { // B)
                    inserted_dummies += DUMMY_SEGMENT::insert_dummies(cluster_segments, *seg, cluster);
                    cluster++;
                }
                appended_dummies += inserted_dummies;
                for(int i = 0; i < inserted_dummies; i++){ seg_to_src_map.push_back(oc_to_src_map[oc]); }


                // only push segments if lane is a multiple of VPRO_CFG::NUM_CLUSTERS
                if (cluster != 0 && (cluster % VPRO_CFG::CLUSTERS) == 0)
                {
                    for (unsigned int s = 0; s < cluster_segments.at(0).size(); s++) {
                        for (auto &batch: cluster_segments) { 
                            segments.push_back(batch[s]);
                        }
                    }
                    for (auto &batch: cluster_segments) { batch.clear(); }
                    cluster = 0;
                }
            }  // X
        } // Y
    } // CH

    // sanity check for number of segments
    unsigned int expected_segment_count = seg.num.x * seg.num.y * out_dim.ch + appended_dummies;
    if (segments.size() != expected_segment_count) {
        int seg_num_in_ch = out_dim.ch;
        std::cout << "Generated " << segments.size() << " segments (" << appended_dummies << " dummies) for layer " << name << ", expected " << expected_segment_count << " (seg.num.x = " << seg.num.x << ", seg.num.y = " << seg.num.y << ", seg_num_in_ch = " << seg_num_in_ch << ", out_dim.ch = " << out_dim.ch << ")\n";
        assert(0);
    }
}

void Concatenate::setSegmentDimensions() {

    // 1024 RF entries (no weights required)
    int rf_free_entries = RF_DISCARD_ADDR; // 1023

    // 4096 LM entries (double buffering)
    int lm_free_entries = VPRO_CFG::LM_SIZE / 4;   // 4096
    int lm_in_seg_max = floor(sqrt(lm_free_entries)); // 64

    int max_beta = 31;
    int max_xend_yend = 31;
    
    lm_in_seg_max = std::min(max_beta, lm_in_seg_max); // 31

    int rf_out_seg_max = std::min(lm_in_seg_max, int(floor(sqrt(rf_free_entries)))); // 31
    rf_out_seg_max = std::min(rf_out_seg_max, max_xend_yend+1); // 31

    seg.num.x = std::max(ceil_div(out_dim.x, rf_out_seg_max),
                         ceil_div(in_dim(0).x, lm_in_seg_max));
    seg.num.y = std::max(ceil_div(out_dim.y, rf_out_seg_max),
                         ceil_div(in_dim(0).y, lm_in_seg_max));

    
    seg.out.w = ceil_div(out_dim.x, seg.num.x);
    seg.out.h = ceil_div(out_dim.y, seg.num.y);
    seg.in.w = seg.out.w;
    seg.in.h = seg.out.h;
 }

SEGMENT *Concatenate::getSegment(int x, int y, int in_ch, int out_ch) {
    // add oc_src_mapping vector
    auto segment = Layer::getSegment(x, y, in_ch, out_ch);

    int src_layer = oc_to_src_map[out_ch];

    // Addresses for input data (load from main memory)
    segment->in_MM_base.clear(); // drop results of Layer::getSegment
    segment->in_MM_base.push_back(in_dim(src_layer).mm.channel_base[in_ch] +
                                  2 * (x * seg.in.w + y * seg.in.h * in_dim(src_layer).mm.x));
    
    segment->in_MM_y_stride.clear();
    segment->in_MM_y_stride.push_back(in_dim(src_layer).mm.x);

    segment->isFirst = false;
    if (x == 0 && y == 0 && in_ch == 0) { 
        segment->isFirst = true; 
    }
    segment->isLast = false;
    if(axis==2){
        if (x == seg.num.x-1 && y == seg.num.y-1 && in_ch == in_dim(src_layer).ch-1) { 
            segment->isLast = true; 
        }
    }
    return segment;
}


} // namespace CNN_LAYER
