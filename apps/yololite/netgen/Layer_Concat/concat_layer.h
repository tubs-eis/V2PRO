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
#ifndef CONCAT_H
#define CONCAT_H

#include "Base/base_layer.h"

namespace CNN_LAYER {
// file for rearrange layer classes
// e.g. concatenate, depth2space, ...
// currently only image-like inputs with quadratic segementation are supported

class Concatenate: public Layer {

  // user-supplied parameters
public:
    std::vector<qparam_t> in_shifts_right;
    std::vector<int> seg_to_src_map;
    std::vector<int> oc_to_ic_map;
    std::vector<int> oc_to_src_map;
    int axis{0}; // concat along channels

// methods
public:
    virtual std::string getLayerTypeName() { return "Concatenate"; }
    virtual LAYERTYPE getLayerType() { return LAYERTYPE::CONCATENATE; }

    // binary export
    virtual void generateBifLayer(BIF::LAYER &bl) {
        Layer::generateBifLayer(bl);
        bl.axis = axis;
    }

    void computeOutputDim() {
        assert(!src_layers.empty() && "can not compute output dim without src layers");

        if(axis==0){
            out_dim.y = in_dim(0).y;
            out_dim.ch = in_dim(0).ch;
            int x = 0;
            for(unsigned int sli = 0; sli < src_layers.size(); sli++) { x += in_dim(sli).x; }
            out_dim.x = x;
        } else if (axis==1){
            out_dim.ch = in_dim(0).ch;
            out_dim.x = in_dim(0).x;
            int y = 0;
            for(unsigned int sli = 0; sli < src_layers.size(); sli++) { y += in_dim(sli).y; }
            out_dim.y = y;
        } else if (axis==2){
            out_dim.x = in_dim(0).x;
            out_dim.y = in_dim(0).y;
            int ch = 0;
            for(unsigned int sli = 0; sli < src_layers.size(); sli++) { ch += in_dim(sli).ch; }
            out_dim.ch = ch;
        } else if(axis==3){
            out_dim.x = in_dim(0).x;
            out_dim.y = in_dim(0).y;
            int ch = 0;
            for(unsigned int sli = 0; sli < src_layers.size(); sli++) { ch += in_dim(sli).ch; }
            out_dim.ch = ch;
        }
    }    

    void addSrcLayers(std::initializer_list<Layer *> new_layers) {
        Layer::addSrcLayers(new_layers);

       

        // assert matching dimensions for concatenate
      

        for(auto l: src_layers){
            for(int ic = 0; ic < l->out_dim.ch; ic++){
                oc_to_ic_map.push_back(ic);
            }
        }

        for(uint l = 0; l < src_layers.size(); l++){
            for(int c = 0; c < src_layers[l]->out_dim.ch; c++){
                oc_to_src_map.push_back(l);
            }
        }
    }
    
    virtual void processParams() {
        Layer::processParams();
        
        // int x = src_layers[0]->out_dim.x;
        // int y = src_layers[0]->out_dim.y;
        // int ch = src_layers[0]->out_dim.ch;

        for (auto l: src_layers){
            (void)l;
            if (axis==2) {
                assert(l->out_dim.x == src_layers[0]->out_dim.x); // concat(axis=2): 0th dim of input layers do not match
                assert(l->out_dim.y == src_layers[0]->out_dim.y); // concat(axis=2): 1th dim of input layers do not match
            } 
            if (axis==1) {
                assert(0); // concat not implemented for axis=1 
                assert(l->out_dim.x == src_layers[0]->out_dim.x); // concat(axis=1): 0th dim of input layers do not match
                assert(l->out_dim.ch == src_layers[0]->out_dim.ch); // concat(axis=1): 2nd dim of input layers do not match
            }
            if (axis==0) {
                assert(0); // concat not implemented for axis=1
                assert(l->out_dim.ch == src_layers[0]->out_dim.ch); // concat(axis=1): 2nd dim of input layers do not match
                assert(l->out_dim.y == src_layers[0]->out_dim.y); // concat(axis=2): 1th dim of input layers do not match
            }
        }
    }

    virtual void setSegmentDimensions();
    virtual SEGMENT *getSegment(int x, int y, int in_ch, int out_ch);
    virtual void generateSegments();
    
    virtual void load(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer);
    virtual void store(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer, bool isProcessed);
    bool concat_compute(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer);
    virtual void generateCommands();

};


}

#endif
