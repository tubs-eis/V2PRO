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
#include "conv_transpose_layer.h"

namespace CNN_LAYER {

BIF::COMMAND_SEGMENT Conv2DTranspose::convVPRO(const CNN_LAYER::SEGMENT &segment, BUFFER &buffer, uint32_t lane_mask, BIF::COMMAND_VPRO &mem_layout) {
    auto cmd = Conv2D::convVPRO(segment, buffer, lane_mask, mem_layout);
    cmd.vpro.command = (segment.isFirst) ? VPRO_TYPE::conv_transpose_start : VPRO_TYPE::conv_transpose_add;

    return cmd;
}

} // namespace CNN_LAYER
