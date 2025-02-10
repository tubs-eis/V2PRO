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
#ifndef RESHAPE_LAYER_H
#define RESHAPE_LAYER_H
#include "base_layer.h"

namespace CNN_LAYER {
    class Reshape: public Layer {

    public:
        Reshape() {
            produces_binary_data = false;
        }
        
        virtual std::string getLayerTypeName() { return "Reshape"; }
        
        virtual void processParams() {
            assert(src_layers.size() == 1 && "reshape must have exactly one input");
            assert(in_dim(0).x*in_dim(0).y*in_dim(0).ch == out_dim.x*out_dim.y*out_dim.ch && "number of elements must not change");
        }
        
        virtual mm_size_type getOutputMMSize() {
            // output is an alias to the input data, no additional memory required
            return 0;
        }

        virtual void setOutputMMAddr(mm_addr_type base_addr) {
            // FIXME allow dimensions to actually change, convert mem layout descriptor (if possible)
            assert(out_dim.x == in_dim(0).x);
            assert(out_dim.y == in_dim(0).y);
            assert(out_dim.ch == in_dim(0).ch);
            out_dim.mm = in_dim(0).mm;
        }
        

        //        virtual void generateSegments() {}

    }; // class Input

} // namespace CNN_LAYER

#endif // RESHAPE_LAYER_H
