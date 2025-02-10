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
#include "depth_to_space_layer.h"

using namespace CNN_LAYER;

void DepthToSpace::generateBifLayer(BIF::LAYER &bl) {
    Layer::generateBifLayer(bl);
    bl.block_size = block_size;
}

void DepthToSpace::computeOutputDim() {
    assert(!src_layers.empty() && "can not compute output dim without src layers");
    assert(block_size == 2 && "depth_to_space only implemented for block_size=2 (outdim)");
    out_dim.y = in_dim(0).y * block_size;
    out_dim.x = in_dim(0).x * block_size;
    out_dim.ch = in_dim(0).ch / block_size / block_size;
}

void DepthToSpace::processParams() {
    computeOutputDim();

    for(int ic = 0; ic < in_dim(0).ch; ic++){
        int oc = (int) ic % out_dim.ch;
        ic_to_oc_map.push_back(oc);
    }
};

void DepthToSpace::addSrcLayers(std::initializer_list<Layer *> new_layers) {
    Layer::addSrcLayers(new_layers);
    assert(src_layers.size() == 1 && "depth_to_space does not accept more than one input");
    assert(src_layers[0]->out_dim.ch % 4 == 0 && "depth_to_space only implemented for ic % 4 == 0");
    //assert(src_layers[0]->out_dim.x == src_layers[0]->out_dim.y && "depth_to_space only implemented for quadratic inputs");
}