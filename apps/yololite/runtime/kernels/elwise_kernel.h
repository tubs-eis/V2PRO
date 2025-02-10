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
#ifndef ELWISE_KERNEL_H
#define ELWISE_KERNEL_H

#include "bif.h"
#include "vpro_functions.h"
#include <vpro.h>
#include "eisv.h"
#include "activation_kernel.h"

// FIXME function unused?
inline void _bias(const BIF::LAYER &layer, const uint32_t xend, const uint32_t yend, const uint32_t offset) {
    insertNops(vector_length_compensate);
    VPRO::DIM2::PROCESSING::add(L0_1,
                                DST_ADDR(offset, 1, layer.seg_out_w),
                                SRC1_ADDR(offset, 1, layer.seg_out_w),
                                SRC2_ADDR(RF_BIAS_BASE, 0, 0),
                                xend,
                                yend);
}

inline void _elwise(VPRO_TYPE command, int32_t input_0_left_shift, int32_t input_1_left_shift, uint32_t broadcast_map, uint32_t xend, uint32_t yend, uint32_t rf_base, uint32_t lm_base) {
    // only one L/S lane ==> can not simultaneously load and store from/to LM ==> need to copy from/to RF twice
    // Approach selection:
    // (A): copy both inputs to RF and chain computation output to LM
    // (B): copy one input to RF, compute into RF and copy to LM
    // Performance:
    //    |                        cycles                               |          RF size
    // ---+-------------------------------------------------------------+-------------------------
    // (A)| size(in0) + size(in1) + size(out)                           |   size(in0) + size(out)
    // (B)| size(in0)             +             size(out) + size(out)   |               size(out)  <- this is used
    //    | copy->RF    copy->RF   comp&store  load&comp    copy->LM    |
    //
    // broadcasting   cycles   mem    winner
    //    none         A = B  A=2*B     B
    //    in0          A = B  A > B     B
    //  in0+in1        A < B  A > B     ?    irrelevant corner case?
    //
    // ==> use approach B
    // - broadcasting in0 saves cycles, bc in1 does not -> netgen swaps inputs
    // - result is in RF -> easy to fuse pooling+activation
    // - instruction for store required -> use for shift of output data

    // shift left/right implemented in _both_ loads and store, as it comes for free
    // Add: shifting _one_ input should (?) be sufficient, output shift required
    // Mul: input shifts not required, output shift required
    // 
    // Potential benefit of using L0 and L1 in parallel instead of chained mode
    // - sequentially load and store data to/from both lanes -> no gain
    // - computation runs in parallel
    // - larger DMA transfers    

    bool bc_0_x = (broadcast_map >> 0) & 1; // in_dim(0).x == 1 && out_dim.x > 1
    bool bc_0_y = (broadcast_map >> 1) & 1;
    bool bc_1_x = (broadcast_map >> 3) & 1;
    bool bc_1_y = (broadcast_map >> 4) & 1;
    // channel broadcasting is completely handled by netgen

    // LM layout of input data: elements on consecutive addresses, also when broadcasting is active

    // mem layout of unbroadcast input 0 in RF
    // transfer LM -> RF
    int load_0_xend = xend; // number of elements in x-dir transferred LM to RF (- 1)
    int load_0_yend = yend;

    // transfer LM -> RF and load from RF
    int rf_offs_0 = rf_base; // RF start address of unbroadcasted input 0 data

    // load from RF
    int rf_x_stride_0 = 1; // RF address distance for step in x-direction
    int rf_y_stride_0 = xend+1;
    
    // broadcasting: store non-broadcasted elements in RF last result row/column to avoid premature overwriting
    if (bc_0_x) {
        rf_offs_0 += xend;
        load_0_xend = 0;
        rf_x_stride_0 = 0;
    }
    if (bc_0_y) {
        rf_offs_0 += yend * (xend+1);
        load_0_yend = 0;
        rf_y_stride_0 = 0;
    }


    if (0) {
        printf("\n\nelwise(input_0_left_shift=%d, input_1_left_shift=%d, broadcast_map=%d, xend=%d, yend=%d, rf_base=0x%03x, lm_base=0x%03x",
               input_0_left_shift, input_1_left_shift, broadcast_map, xend, yend, rf_base, lm_base);
        printf("  bc_0_x = %d, bc_0_y = %d, bc_1_x = %d, bc_1_y = %d\n", bc_0_x, bc_0_y, bc_1_x, bc_1_y);
        printf("  load_0_xend = %d, load_0_yend = %d, rf_offs_0 = %d, rf_x_stride_0 = %d, rf_y_stride_0 = %d\n\n", load_0_xend, load_0_yend, rf_offs_0, rf_x_stride_0, rf_y_stride_0);
    }

    // load input 0 into L0 RF
    // no broadcasting here, number of elements stays the same
    // distribute elements to end of rows and columns
    VPRO::DIM2::LOADSTORE::loads(lm_base,
                                 rf_base, 1, bc_0_x ? 1 : (xend+1), // input 0 MM/LM layout: y-stride depends on row length
                                 load_0_xend, load_0_yend);

    if (input_0_left_shift >= 0) {
        VPRO::DIM2::PROCESSING::mull(L0,
                                     DST_ADDR(rf_offs_0, 1, xend+1), // output layout (post-broadcast)
                                     SRC1_IMM_2D(1 << input_0_left_shift),
                                     SRC2_LS_2D,
                                     load_0_xend, load_0_yend, false, false, true);
    } else {
        VPRO::DIM2::PROCESSING::shift_ar(
                L0,
                DST_ADDR(rf_offs_0, 1, xend+1),
                SRC1_LS_2D,
                SRC2_IMM_2D(-input_0_left_shift),
                load_0_xend,
                load_0_yend, false, false, true);
    }

    // input 1 -> L1 (shift) -> L0 (add)
    VPRO::DIM2::LOADSTORE::loads(lm_base + (xend+1) * (yend+1),
                                 rf_base, bc_1_x ? 0 : 1, bc_1_y ? 0 : (bc_1_x ? 1 : (xend+1)),
                                 xend, yend);

    if (input_1_left_shift >= 0) {
        VPRO::DIM2::PROCESSING::mull(L1,
                                     DST_DISCARD_2D,
                                     SRC1_IMM_2D(1 << input_1_left_shift),
                                     SRC2_LS_2D,
                                     xend, yend,
                                     true); // continue chain
    } else {
        VPRO::DIM2::PROCESSING::shift_ar(
                L1,
                DST_DISCARD_2D,
                SRC1_LS_2D,
                SRC2_IMM_2D(-input_1_left_shift),
                xend,
                yend, true);  // continue chain
    }

    // load input 1
    switch (command) {
    case VPRO_TYPE::add:
        VPRO::DIM2::PROCESSING::add(L0,
                                    DST_ADDR(rf_base, 1, xend+1),
                                    SRC1_CHAINING_LEFT_2D, // input 1
                                    SRC2_ADDR(rf_offs_0, rf_x_stride_0, rf_y_stride_0), // input 0
                                    xend,
                                    yend,
                                    false, true); // update flags for _act_leakyrelu()
        break;
    case VPRO_TYPE::mul:
        // mul_h_bit_shift == layer.conv_result_shift_right
        VPRO::DIM2::PROCESSING::mulh(L0,
                                     DST_ADDR(rf_base, 1, xend+1),
                                     SRC1_CHAINING_LEFT_2D, // input 1
                                     SRC2_ADDR(rf_offs_0, rf_x_stride_0, rf_y_stride_0), // input 0
                                     xend,
                                     yend,
                                     false, true); // update flags for _act_leakyrelu()
        break;
    default:
        break;
    }
}

#endif // ELWISE_KERNEL_H
