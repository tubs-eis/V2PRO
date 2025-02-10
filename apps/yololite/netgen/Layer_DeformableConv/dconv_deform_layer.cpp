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
// Created by gesper on 19.07.23.
//

#include "dconv_deform_layer.h"

using namespace CNN_LAYER;

std::string DConvDeform::getLayerInfoText() {
    std::stringstream ss;
    ss << Layer::getLayerInfoText();
    ss << "  max_offset (x, y) (" << max_offset_x << ", " << max_offset_y << ") \n";
    return ss.str();
}

// Deform requires exactly two input layers, the inputs and the offsets + masks
void DConvDeform::setSrcLayers(Layer* input, Layer* offset) {
    assert(input->out_dim.x == offset->out_dim.x);
    assert(input->out_dim.y == offset->out_dim.y);
    assert(offset->out_dim.ch == 3 * kernel_size);

    addSrcLayers({input, offset});
}

void DConvDeform::computeOutputDim() {
    int in_width = in_dim(0).x;
    int in_height = in_dim(0).y;
    int in_channels = in_dim(0).ch;

    out_dim.x = in_width * kernel_size;
    out_dim.y = in_height;
    out_dim.ch = in_channels;
}

int DConvDeform::expectedWeightCount() {
    return 8 * 8 * kernel_size;
}

// binary export
void DConvDeform::generateBifLayer(BIF::LAYER &bl) {
    Layer::generateBifLayer(bl);

    bl.deform_max_offset_x = max_offset_x;
    bl.deform_max_offset_y = max_offset_y;
    bl.deform_static_offsets = lm_static_offset_addr();
    bl.conv_result_shift_right = result_shift_right;
}