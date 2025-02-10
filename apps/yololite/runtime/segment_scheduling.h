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


#ifndef SEGMENT_SCHEDULING_H
#define SEGMENT_SCHEDULING_H

#include "bif.h"

/**
 * main VPRO Function to execute the layers calculations
 * performs management of segments related to this layer and starts/waits for DMA and VPRO
 *   INPUT -> OUTPUT
 *   basis is convolution2D, bias and shift
 *   uses functions of the VPRO (LOAD, MAC, ADD, SHIFT, STORE)
 *   uses functions of the DMA (1D_to_2D, 2D_to_2D)
 *   beforms calculation in segments as defined in layer using double buffering
 *
 * @param layer
 */
void calcLayer(BIF::LAYER &layer, const BIF::COMMAND_SEGMENT *segments, uint32_t seg_size);

#endif // SEGMENT_SCHEDULING_H
