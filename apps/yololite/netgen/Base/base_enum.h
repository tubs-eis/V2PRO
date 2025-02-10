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
// Created by gesper on 20.07.23.
//

#ifndef NETGEN_BASE_ENUM_H
#define NETGEN_BASE_ENUM_H

namespace CNN_LAYER {

#define STRIDE_UNSET (INT_MIN)
#define GROUPS_UNSET (INT_MIN)

    enum PADDING_MODE {
        SAME = 0,
        VALID = 1
    };

    inline std::string to_string(PADDING_MODE p) {
        // std::stringstream ss;
        switch (p) {
        case SAME: return "same"; break;
        case VALID: return "valid"; break;
        default: return "<invalid>"; break;
        }
        //return ss.str();
    }
        

    struct CMD_COUNT {
        int sync{};
        int vpro{};
        int dma{};
    };

    enum SEGMENT_SCHEDULING_ORDER {
        ITERATE_ALL_SORTED_OUTC,
        ITERATE_ALL_SORTED_X,
        ITERATE_ALL_SORTED_X2,
    };

    enum SEGMENTATION_STRATEGY {
        FAST_HEURISTIC, // computation of the heuristic is fast, but the resulting VPRO runtime execution may be slow
        DETAILED_HEURISTIC, // heuristic takes time to design the segmentation, faster VPRO execution
    };
  
    // configure command generation
    struct LAYERCFG {
        bool use_dma_merger{true};
        bool use_dma_interleaver{false}; // deprecated, use dma_extension instead
        bool use_dma_extension{true};
        bool use_dma_store_splitter{true};
        bool use_dma_loop_extension{true};
        bool use_dma_l2e_mix_extension{false};
        SEGMENT_SCHEDULING_ORDER scheduling_order{ITERATE_ALL_SORTED_OUTC};
        SEGMENTATION_STRATEGY segmentation_strategy{DETAILED_HEURISTIC};
        bool force_segment_dump{false};
    };

    struct segDim {
        struct { // number of segments to overlap input and output feature maps (identical for in and out)
            int x{};
            int y{};
        } num;
        struct { // segment input geometry
            int w{}; // width in elements
            int h{}; // height
            // strides default to (w|h) if not explicitly assigned
            int x_stride{STRIDE_UNSET}; // distance between left edges of horizontally consecutive segments (in elements)
            int y_stride{STRIDE_UNSET}; // bistance between top edges of vertically consecutive segments
        } in;
        struct { // segment output geometry
            int w{};
            int h{};
            // strides default to (w|h) if not explicitly assigned
            int x_stride{STRIDE_UNSET};
            int y_stride{STRIDE_UNSET};
        } out;
    };
}
#endif //NETGEN_BASE_ENUM_H
