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
#ifndef SLICE_LAYER_H
#define SLICE_LAYER_H

#include "Base/base_layer.h"

namespace CNN_LAYER {

class SliceChannel: public Layer {
public:
    SliceChannel() {produces_binary_data = false;}

    virtual std::string getLayerTypeName() { return "SliceChannel"; }

    virtual void processParams() {
        Layer::processParams();
        groups = out_dim.ch; // each output channel uses one input channel
    }

    virtual void computeOutputDim() {
        assert(!src_layers.empty() && "cannot compute output dim without src layers");
        out_dim = in_dim(0);
        out_dim.mm.layout_known = false;
        stop = (stop==-1) ? in_dim(0).ch : stop;
        out_dim.ch = stop - start;
    }

    virtual mm_size_type getOutputMMSize() {
        // output is an alias to the input data, no additional memory required
        return 0;
    }

    virtual void setOutputMMAddr(mm_addr_type base_addr) {
        // output points to input+offset
        Layer::setOutputMMAddr(in_dim(0).mm.channel_base[start]);
    }

    virtual void generateSegments() {} // Slice layer does not produce segments
    
    virtual void setOutputMemDimensions() {
        // memory layout identical to input, except shifted by offset and fewer channels
        out_dim.mm.x = in_dim(0).mm.x;
        out_dim.mm.y = in_dim(0).mm.y;

        // rest of memory layout filled by inherited calcOutputMemLayout(), depends on out_dim.ch and .mm.(base|x|y) only
    }

    int start{0};
    int stop{-1}; // index of last channel + 1
}; // class Slice

} // namespace CNN_LAYER

#endif
