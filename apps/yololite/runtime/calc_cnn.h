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
#ifndef CALC_CNN_H
#define CALC_CNN_H

#include "bif.h"

// Versioning
#define VERSION_MAJOR 0
#define VERSION_MINOR 1

#include <versioning.h>

extern char versionVersion[];
extern char completeVersion[];

// ARM Communication Register Indexes
enum ARM_RV_Comm : uint32_t {
  rv_input_parsed = 128,
    rv_output_ready = 132,
    arm_input_ready = 136,
    arm_output_parsed = 140,
    rv_running = 144,
    };

void pre_layer_hook(int layer_exec_idx, int total_layers, const BIF::LAYER *layer);
void post_layer_hook(int layer_exec_idx, int total_layers, const BIF::LAYER *layer);

uint64_t calcCnn(BIF::NET *bnet, bool per_layer_stats);

void print_cnn_stats(uint64_t totalclock, unsigned int clockfreq_mhz);

#endif // CALC_CNN_H
