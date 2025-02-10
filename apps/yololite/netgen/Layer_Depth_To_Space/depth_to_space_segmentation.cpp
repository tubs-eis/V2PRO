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
#include "Base/base_layer.h"
#include "Base/segment.h"
#include "vpro_globals.h"
#include "depth_to_space_layer.h"

namespace CNN_LAYER {

SEGMENT *DepthToSpace::getSegment(int x, int y, int ic, int oc) {


    SEGMENT* segment = new SEGMENT;
    segment->x_seg = x;
    segment->y_seg = y; 
    segment->out_channel = oc;
    segment->in_channel = ic;
     
    int ic_offset = (ic % 4) * out_dim.ch * in_dim(0).x * in_dim(0).y;
    if(ic % 4 == 1) ic_offset = 2 * out_dim.ch * in_dim(0).x * in_dim(0).y;
    if(ic % 4 == 2) ic_offset = 1 * out_dim.ch * in_dim(0).x * in_dim(0).y;
    int oc_offset = oc * in_dim(0).x * in_dim(0).y;
    int x_offset = x * seg.in.w;
    int y_offset = y * in_dim(0).x * seg.in.h;

    // valid for test case oc= 3
    //segment->in_MM_base_0 = mm_inputs[0].channel_base[0] + x*4 + y*16 + (in_ch % (block_size * block_size)) *96 + oc*32; // oc+3
    segment->in_MM_base.push_back(in_dim(0).mm.channel_base[0] + 2 * ( ic_offset + oc_offset + x_offset + y_offset ));
    segment->in_MM_y_stride.push_back(in_dim(0).mm.x);


    int out_oc_offset = oc * out_dim.x * out_dim.y; //64
    int out_ic_offset = (ic % 4) * out_dim.x; //8
    int out_x_offset = x * block_size * block_size; //4
    int out_y_offset = y * seg.in.h * block_size * out_dim.x; //32

    // valid for test case oc= 3
    //segment->out_MM_base = mm_output.channel_base[0] + 2 * ( x * 4 + y * 32 + (ic % (block_size * block_size)) * 8 + oc * 64 );
    segment->out_MM_base = out_dim.mm.channel_base[0] + 2 * ( out_ic_offset + out_oc_offset + out_x_offset + out_y_offset );
    segment->out_MM_y_stride = out_dim.mm.x; // FIXME original used out_dim.x, not mm.x

    segment->isFirst = false;   // not used in d2s
    segment->isLast = ((x == seg.num.x-1) && (y == seg.num.y-1) && (ic == in_dim(0).ch-1));    // not used in d2s
    segment->dummy = false;
    return segment;
}

void DepthToSpace::setSegmentDimensions() {

    seg.in.h = 2;
    seg.in.w = 2;
    
    seg.num.x = in_dim(0).x / seg.in.w;
    seg.num.y = in_dim(0).y / seg.in.h;

    seg.out.w = out_dim.x / seg.num.x;
    seg.out.h = out_dim.y / seg.num.y;
}

void DepthToSpace::generateSegments() {

    assert(out_dim.mm.layout_known && "Main memory address for this layer's output has not been set! Call setOutputMMAddr()!");
    for (auto l: src_layers) {
        assert(l->out_dim.mm.layout_known && "Main memory address for input of this layer has not been set! Call setOutputMMAddr() on all input layers!");
    }    

    SegmentBatch cluster_segments = SegmentBatch(VPRO_CFG::CLUSTERS); // stores a batch (vector) of segments for each lane
    std::vector<SEGMENT *> *batch;
    int cl = 0;
    int appended_dummies = 0;



    // reserve the minimum number of segments to avoid frequent reallocations
    // reallocations may still happen, due to insertion of dummies
    segments.reserve(seg.num.x * seg.num.y * out_dim.ch);
    //for ( auto oc=0; oc < out_dim.ch; oc++) {
        for (auto y = 0; y < seg.num.y; y++) {
            for (auto x = 0; x < seg.num.x; x++) {
                for (auto ic = 0; ic < in_dim(0).ch; ic++) {
                    //int oc = ic_to_oc_map[ic];
                    int oc = (int) (ic / (block_size * block_size));

                

                    auto seg = getSegment(x, y, ic, oc);  
                    
             

                    batch = &cluster_segments[cl];
                    batch->push_back(seg);

                    if(ic == in_dim(0).ch - 1){ cl = (cl + 1) % VPRO_CFG::CLUSTERS; }
                       
                    while(seg->isLast && cl != 0 ){
                        int dummies = DUMMY_SEGMENT::insert_dummies(cluster_segments, *seg, cl);
                        appended_dummies += dummies;
                        cl = (cl + 1) % VPRO_CFG::CLUSTERS;
                    }
                    
                    if (seg->isLast && (unsigned int)cl == 0){
                        // if (cl == 0 || (cl % VPRO_CFG::CLUSTERS) != 0) {
                        //     std::cout << "Final Push Batch not successful!";
                        //     std::cout << "Remaining batches of size" << cluster_segments[0].size();
                        //     std::cout << "cluster: " << cl << std::endl;
                        // }

                        for (unsigned int s = 0; s < cluster_segments.at(0).size(); s++) {
                            for (auto &batch: cluster_segments) { segments.push_back(batch[s]); }
                        }
                        for (auto &batch: cluster_segments) { batch.clear(); }
                    }

                } // IC
            } // X
        } // Y
    //} // OC

    // sanity check for number of segments
    unsigned int expected_segment_count = seg.num.x * seg.num.y * in_dim(0).ch + appended_dummies;
    if (segments.size() != expected_segment_count) {
        std::cout << " segments.size()=" << segments.size() << std::endl; 
        std::cout << " appended_dummies=" << appended_dummies << std::endl;
        std::cout << " expected_segment_count=" << expected_segment_count << std::endl;
        std::cout << " out_dim.ch=" << out_dim.ch << std::endl;
        std::cout << " out_dim.x=" << out_dim.x << std::endl; 
        std::cout << " out_dim.y=" << out_dim.y << std::endl;
        std::cout << " in_dim(0).ch=" << in_dim(0).ch << std::endl;
        std::cout << " in_dim(0).x=" << in_dim(0).x << std::endl; 
        std::cout << " in_dim(0).y=" << in_dim(0).y << std::endl; 
        std::cout << " seg.num.y=" << seg.num.y << std::endl; 
        std::cout << " seg.num.x=" << seg.num.x << std::endl;
        std::cout << " seg.in.h=" << seg.in.h << std::endl; 
        std::cout << " seg.in.w=" << seg.in.w << std::endl;
        std::cout << " seg.out.h=" << seg.out.h << std::endl; 
        std::cout << " seg.out.w=" << seg.out.w << std::endl;
        std::cout << std::endl;
        assert(0);
    }

}

}; // namespace CNN_LAYER
