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
// Created by gesper on 13.01.23.
//

#ifndef CONV2D_DMA_WRAPPER_OUTDATED_ID_H
#define CONV2D_DMA_WRAPPER_OUTDATED_ID_H

#ifndef SIMULATION
#include "vpro_aux.h"
#else
#include "iss_aux.h"
#endif

inline void __attribute__((always_inline)) dma_ext1D_to_loc1D(uint32_t cluster, intptr_t ext_base, uint32_t loc_base, uint32_t word_count){
    printf_warning("[Warning] Deprecated usage; use dma_e2l_1d(...) (with mask parameters) instead dma_ext1D_to_loc1D(...)!\n");
    uint32_t cluster_mask = 1 << cluster;
    uint32_t unit_mask = 1 << (loc_base >> 22);
    dma_e2l_1d(cluster_mask, unit_mask, ext_base, loc_base, word_count);
}

inline void __attribute__((always_inline))  dma_ext2D_to_loc1D(uint32_t cluster, intptr_t ext_base, uint32_t loc_base, uint32_t x_stride, uint32_t x_size,
                        uint32_t y_size, const bool pad_flags[4] = default_flags){
    printf_warning("[Warning] Deprecated usage; use dma_e2l_2d(...) (with mask parameters) instead dma_ext2D_to_loc1D(...)!\n");
    uint32_t cluster_mask = 1 << cluster;
    uint32_t unit_mask = 1 << (loc_base >> 22);
    dma_e2l_2d(cluster_mask, unit_mask, ext_base, loc_base, x_size, y_size, x_stride, pad_flags);
}

inline void __attribute__((always_inline))  dma_loc1D_to_ext1D(uint32_t cluster, intptr_t ext_base, uint32_t loc_base, uint32_t word_count){
    printf_warning("[Warning] Deprecated usage; use dma_l2e_1d(...) (with mask parameters) instead dma_loc1D_to_ext1D(...)!\n");
    uint32_t cluster_mask = 1 << cluster;
    uint32_t unit_mask = 1 << (loc_base >> 22);
    dma_l2e_1d(cluster_mask, unit_mask, ext_base, loc_base, word_count);
}

inline void __attribute__((always_inline))  dma_loc1D_to_ext2D(uint32_t cluster, intptr_t ext_base, uint32_t loc_base, uint32_t x_stride, uint32_t x_size,
                        uint32_t y_size){
    printf_warning("[Warning] Deprecated usage; use dma_l2e_2d(...) (with mask parameters) instead dma_loc1D_to_ext2D(...)!\n");
    uint32_t cluster_mask = 1 << cluster;
    uint32_t unit_mask = 1 << (loc_base >> 22);
    dma_l2e_2d(cluster_mask, unit_mask, ext_base, loc_base, x_size, y_size, x_stride);
}

inline void __attribute__((always_inline))  dma_ext1D_to_loc1D_broadcast(uint32_t cluster, uint64_t unit_mask, uint64_t ext_base, uint32_t loc_base, uint32_t word_count){
    printf_warning("[Warning] Deprecated usage; use dma_e2l_1d(...) (with mask parameters) instead dma_ext1D_to_loc1D_broadcast(...)!\n");
    uint32_t cluster_mask = 1 << cluster;
    dma_e2l_1d(cluster_mask, unit_mask, ext_base, loc_base, word_count);
}

inline void __attribute__((always_inline))  dma_ext2D_to_loc1D_broadcast(uint32_t cluster, uint32_t unit_mask, uint32_t ext_base, uint32_t loc_base,
                                  int32_t x_stride, uint32_t x_size, uint32_t y_size,
                                  bool pad_flags[4] = default_flags){
    printf_warning("[Warning] Deprecated usage; use dma_e2l_2d(...) (with mask parameters) instead dma_ext2D_to_loc1D_broadcast(...)!\n");
    uint32_t cluster_mask = 1 << cluster;
    dma_e2l_2d(cluster_mask, unit_mask, ext_base, loc_base, x_size, y_size, x_stride, pad_flags);
}

#endif //CONV2D_DMA_WRAPPER_OUTDATED_ID_H
