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
#ifndef DEPTH_TO_SPACE_H
#define DEPTH_TO_SPACE_H

#include "Base/base_layer.h"

namespace CNN_LAYER {
// file for rearrange layer classes
// e.g. concatenate, depth2space, ...
// currently only image-like inputs with quadratic segementation are supported

class DepthToSpace: public Layer {

  // user-supplied parameters
public:
    int block_size{0};

// methods
public:
    virtual std::string getLayerTypeName() { return "DepthToSpace"; }
    virtual LAYERTYPE getLayerType() { return LAYERTYPE::DEPTH_TO_SPACE; }
    std::vector<int> ic_to_oc_map;

    // binary export
    virtual void generateBifLayer(BIF::LAYER &bl);
    void computeOutputDim();
    virtual void processParams();
    void addSrcLayers(std::initializer_list<Layer *> new_layers);

    virtual void setSegmentDimensions();
    virtual SEGMENT *getSegment(int x, int y, int oc, int ic);
    virtual void generateSegments();

    virtual void load(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer);
    virtual void store(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer);
    virtual void compute(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer, BUFFER &store_buffer);
    virtual void generateCommands();
    virtual void compressCommands();
};

}   // namepace CNN_LAYER

#endif
