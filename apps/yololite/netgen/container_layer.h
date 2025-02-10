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
#ifndef CONTAINER_LAYER_H
#define CONTAINER_LAYER_H


#include "base_layer.h"

namespace CNN_LAYER {


class ContainerLayer: public Layer {

// user-supplied parameters
public:
    virtual void processParams() {
        Layer::processParams();
    }

    std::vector<Layer *> child_layers;


}; // class ContainerLayer

class PoolingContainer: public ContainerLayer {


    
public:

    virtual void processParams() {

        ContainerLayer::processParams();
        
        // here: map container layer parameters to children layer
        // setsegmentdimensions
        // setpadding
        // 

        for (Layer l: child_layers) {
            l.processParams();
        }
    }

    PoolingContainer() {
        child_layers.push_back(center_layer);
        child_layers.push_back(edge_layer_top);
        child_layers.push_back(edge_layer_bottom);
        child_layers.push_back(edge_layer_left);
        child_layers.push_back(edge_layer_right);
        child_layers.push_back(corner_layer_top_left);
        child_layers.push_back(corner_layer_top_right);
        child_layers.push_back(corner_layer_bottom_left);
        child_layers.push_back(corner_layer_bottom_right);
    }

    virtual void setOutputMMAddr(mm_addr_type base_addr) {
        ContainerLayer::setOutputMMAddr(base_addr);

    }

    // edge bekommt input, der schon die richtigen dimensionen hat und das richtige speicherlayout
    // edge layer sieht nur 3 pixel breit und 370 pixel hoch
    // das obere edge layer ist 3 pixel hoch und 380 pixel breit
    // die output speicherbereiche werden auch zugeteilt

    // Diese Layer auch einzeln testen. Dringende Empfehlung!
    // Consider: Einzelne TestCase in NNQuant bauen, einzelne Layer in Python dafür selber bauen

    // bsp addition 9 pixel. rf hat 24bit. wie groß darf mul faktor maximal sein?
    // das hier ist nicht datenabhängig, deswegen in cnn converter
    // pooling layer do not require any knowledge of fpfs
    // move these computations from nn quantization to cnn converter
    // l001->avgpool_div = {8192, 10922, 16384, 32767, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // l001->avgpool_shift_right = 15;

    AveragePooling2DCenter center_layer;
    AveragePooling2DEdge edge_layer_top;
    AveragePooling2DEdge edge_layer_bottom;
    AveragePooling2DEdge edge_layer_left;
    AveragePooling2DEdge edge_layer_right;
    
    AveragePooling2DCorner corner_layer_top_left;
    AveragePooling2DCorner corner_layer_top_right;
    AveragePooling2DCorner corner_layer_bottom_left;
    AveragePooling2DCorner corner_layer_bottom_right;

    

};

} // namespace CNN_LAYER

#endif 