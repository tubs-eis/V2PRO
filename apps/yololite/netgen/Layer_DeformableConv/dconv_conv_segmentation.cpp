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
#include "dconv_conv_layer.h"
#include "Base/segment.h"
#include "vpro_globals.h"

namespace CNN_LAYER {

void DConvConv::setSegmentDimensions() {
    // Based on Conv2D dimensions
    // Different sizes (1x9) elements

    // 1024 RF entries -> 32 for dim of out seg
    int n_weights = kernel_length + int(use_bias);
    int rf_free_entries = RF_DISCARD_ADDR - n_weights;

    // local memory is halved for double buffering
    int lm_free_entries = VPRO_CFG::LM_SIZE / 2 - 2 * n_weights;
    int lm_in_seg_max = int(floor(sqrt(lm_free_entries / kernel_length)));

    int max_beta = 31;
    int max_xend_yend = 31; //FIXME: use globals

    //  VPRO can 2D address max 31 * 31 sections (beta limited to 5 bit - next line address), so dim is max 31
    lm_in_seg_max = std::min(max_beta, lm_in_seg_max);

    int rf_out_seg_max = std::min(lm_in_seg_max, int(floor(sqrt(rf_free_entries))));

    // Segment
    seg.num.x = std::max(ceil_div(conv_out_dim.x, rf_out_seg_max),
                         ceil_div(in_dim(0).x / kernel_length, lm_in_seg_max)); // FIXME is the 1st arg really meant to be a floor div?
    seg.num.y = std::max(ceil_div(conv_out_dim.y, rf_out_seg_max),
                         ceil_div(in_dim(0).y, lm_in_seg_max));

    seg.out.w = ceil_div(conv_out_dim.x, seg.num.x);
    seg.out.h = ceil_div(conv_out_dim.y, seg.num.y);

    // limit to 16 -> segment addressing by x_end, y_end
    int max_seg_dim = max_xend_yend + 1;   // for all segments...
    if (seg.out.w > max_seg_dim) {
      seg.num.x = ceil_div(conv_out_dim.x, max_seg_dim);
      seg.out.w = ceil_div(conv_out_dim.x, seg.num.x);
    }
    if (seg.out.h > max_seg_dim) {
      seg.num.y = ceil_div(conv_out_dim.y, max_seg_dim);
      seg.out.h = ceil_div(conv_out_dim.y, seg.num.y);
    }

    seg.in.w = seg.out.w * kernel_length;
    seg.in.h = seg.out.h;

    conv_seg_w = seg.out.w;
    conv_seg_h = seg.out.h;
}

}; // namespace CNN_LAYER
