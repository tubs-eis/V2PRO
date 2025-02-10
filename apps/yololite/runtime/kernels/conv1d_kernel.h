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
#ifndef CONV1D_KERNEL_H
#define CONV1D_KERNEL_H

#include "bif.h"
#include "vpro_functions.h"
#include <vpro.h>
#include "eisv.h"

#if RV_VPRO_EXT == 1 and not defined(SIMULATION)
using namespace VPRO_RISC_EXT_VPRO;
using namespace VPRO_RISC_EXT_DMA;
#endif

inline void _conv1d(const BIF::LAYER &layer, uint32_t zend, const uint32_t lm_base, const uint32_t rf_base) {
    assert(kernel_y == 1);
    assert((unsigned int) zend <= MAX_Z_END);
    VPRO::DIM3::LOADSTORE::loads(lm_base, 0, 0, 0, layer.stride, 
                                    0, 0, zend);

    VPRO::DIM3::PROCESSING::mach_init_addr(L0_1,
                                 DST_ADDR(rf_base, 0, 0, 1),
                                 SRC1_LS_3D,
                                 SRC2_ADDR(RF_KERNEL_BASE, 0, 0, 0),
                                 0, 0, zend,
                                 RF_BIAS_BASE, 0, 0, 0,
                                 false, true);
}

inline void _conv1d_add(const BIF::LAYER &layer, uint32_t zend, const uint32_t lm_base, const uint32_t rf_base, unsigned int in_ch_offset) {
    assert(kernel_y == 1);
    assert(in_ch_offset < kernel_x);
    assert((unsigned int) zend <= MAX_Z_END);
    uint32_t rf_kernel_addr = RF_KERNEL_BASE + in_ch_offset;
    VPRO::DIM3::LOADSTORE::loads(lm_base, 0, 0, 0, layer.stride, 
                                    0, 0, zend);

    VPRO::DIM3::PROCESSING::mach_init_addr(L0_1,
                                 DST_ADDR(rf_base, 0, 0, 1),
                                 SRC1_LS_3D,
                                 SRC2_ADDR(rf_kernel_addr, 0, 0, 0),
                                 0, 0, zend,
                                 rf_base, 0, 0, 1,
                                 false, true);
}

#endif // CONV1D_KERNEL_H
