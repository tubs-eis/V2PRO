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
// Created by gesper on 20.07.23.
//

#ifndef NETGEN_BASE_INPUT_H
#define NETGEN_BASE_INPUT_H
#include "base_layer.h"

namespace CNN_LAYER {
    class Input: public Layer {

    public:
        Input() {
            produces_binary_data = false;
            is_input_layer = true;
            padding.enabled = false;
        }

        virtual std::string getLayerTypeName() { return "Input"; }

        virtual void generateSegments() {} // input layer does not produce segments

    }; // class Input

} // namespace CNN_LAYER

#endif //NETGEN_BASE_INPUT_H
