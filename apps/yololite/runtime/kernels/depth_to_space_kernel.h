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
#ifndef DEPTHTOSPACE_KERNEL_H
#define DEPTHTOSPACE_KERNEL_H

#include "bif.h"
#include "vpro_functions.h"
#include <vpro.h>
#include "eisv.h"





inline void _depth_to_space(const BIF::LAYER &layer, const uint16_t buffer, const uint32_t x_end, const uint32_t y_end) {

    // x_end = seg_in_w
    // y_end = seg_in_h
    //debug &= DEBUG_INSTRUCTION_DATA;
    // std::cout << std::endl;
    // std::cout << std::endl;
    // std::cout << " src offs=" << 0 * layer.seg_in_w * layer.seg_in_h;
    // std::cout << " addr0=" << 1 * x_end + layer.seg_in_w * y_end + 0 * layer.seg_in_w * layer.seg_in_h;
    // std::cout << " addr1=" << 1 * x_end + layer.seg_in_w * y_end + 1 * layer.seg_in_w * layer.seg_in_h;
    // std::cout << " addr2=" << 1 * x_end + layer.seg_in_w * y_end + 2 * layer.seg_in_w * layer.seg_in_h;
    // std::cout << " addr3=" << 1 * x_end + layer.seg_in_w * y_end + 3 * layer.seg_in_w * layer.seg_in_h;
    // std::cout << std::endl;
    // std::cout << std::endl;
   
    VPRO::DIM2::LOADSTORE::loads(
        0,                                      // uint32_t offset
        0,                                      // uint32_t src_offset 
        1,                                      // uint32_t src_alpha
        2,                                      // uint32_t src_beta    
        x_end,                                  // uint32_t x_end
        y_end);                                 // uint32_t y_end
    
    // 4 = block_size * block_size
    VPRO::DIM2::PROCESSING::add(
        L0,
        DST_ADDR(0, 2, 8),
        SRC1_LS_2D,
        SRC2_IMM_2D(0),
        x_end,
        y_end);

    VPRO::DIM2::LOADSTORE::loads(
        0,                                      // uint32_t offset
        4,                                      // uint32_t src_offset 
        1,                                      // uint32_t src_alpha
        2,                                      // uint32_t src_beta    
        x_end,                                  // uint32_t x_end
        y_end);                                 // uint32_t y_end
    // 4 = block_size * block_size
    VPRO::DIM2::PROCESSING::add(
        L0,
        DST_ADDR(4, 2, 8),
        SRC1_LS_2D,
        SRC2_IMM_2D(0),
        x_end,
        y_end);


    VPRO::DIM2::LOADSTORE::loads(
        0,                                      // uint32_t offset
        8,                                      // uint32_t src_offset 
        1,                                      // uint32_t src_alpha
        2,                                      // uint32_t src_beta    
        x_end,                                  // uint32_t x_end
        y_end);                                 // uint32_t y_end
    // 4 = block_size * block_size
    VPRO::DIM2::PROCESSING::add(
        L0,
        DST_ADDR(1, 2, 8),
        SRC1_LS_2D,
        SRC2_IMM_2D(0),
        x_end,
        y_end);

    VPRO::DIM2::LOADSTORE::loads(
        0,                                      // uint32_t offset
        12,                                      // uint32_t src_offset 
        1,                                      // uint32_t src_alpha
        2,                                      // uint32_t src_beta    
        x_end,                                  // uint32_t x_end
        y_end);                                 // uint32_t y_end
    
    // 4 = block_size * block_size
    VPRO::DIM2::PROCESSING::add(
        L0,
        DST_ADDR(5, 2, 8),
        SRC1_LS_2D,
        SRC2_IMM_2D(0),
        x_end,
        y_end);


    VPRO::DIM2::PROCESSING::add(
        L0,
        DST_DISCARD_2D,
        SRC1_ADDR(0, 1, 4),
        SRC2_IMM_2D(0),
        2 * (x_end + 1) - 1,
        2 * (y_end + 1) - 1,
        true);
    VPRO::DIM2::LOADSTORE::store(
        0,                                      // uint32_t offset
        0,                                      // uint32_t dst_offset
        1,                                      // uint32_t dst_alpha
        4,                                      // uint32_t dst_besta
        2 * (x_end + 1) - 1,                    // uint32_t x_end
        2 * (y_end + 1) - 1,                    // uint32_t y_end
        L0);                                    // uint32_t src_lane
    


}

#endif
