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
#ifndef ELWISE_H
#define ELWISE_H

#include "Base/fusedfunc_layer.h"


namespace CNN_LAYER {

// base class for layers with 2 inputs that perform elementwise operations
// e.g. add, subtract, multiply
// currently only image-like inputs with quadratic segementation are supported
class Elementwise: public FusedFunc {

  // user-supplied parameters
public:
  qparam_t input_shift_left_0{0}; //FIXME: use vector?
  qparam_t input_shift_left_1{0};

  // methods
public:
  virtual std::string getLayerTypeName() { return "Elementwise"; }

  virtual void processParams() {
    padding.enabled = false;
    if (src_layers.size() == 2) {
      assert((in_dim(0).x  == in_dim(1).x  || in_dim(0).x  == 1 || in_dim(1).x  == 1) &&  "x-dimensions must be either identical or 1 (broadcasting)");
      assert((in_dim(0).y  == in_dim(1).y  || in_dim(0).y  == 1 || in_dim(1).y  == 1) &&  "y-dimensions must be either identical or 1 (broadcasting)");
      assert((in_dim(0).ch == in_dim(1).ch || in_dim(0).ch == 1 || in_dim(1).ch == 1) && "ch-dimensions must be either identical or 1 (broadcasting)");

      // performance: broadcasting in x and y is cheaper for input 0 (fewer data loaded into RF)
      // channel broadcasting is equally expensive for inputs 0 and 1
      if (in_dim(0).x*in_dim(0).y > in_dim(1).x*in_dim(1).y) {
        std::swap(src_layers[0], src_layers[1]);
        std::swap(input_shift_left_0, input_shift_left_1);
      }
    }
    
    FusedFunc::processParams();

    groups = out_dim.ch; // each output channel uses one input channel
  }

  void computeOutputDim() {
    // broadcasting: out_dim = max(in_dims)
    // find max over all input channels, dimension-wise
    out_dim = in_dim(0);
    for (int src_idx = 1; src_idx < (int)src_layers.size(); src_idx++) {
      out_dim.x  = std::max(out_dim.x , in_dim(src_idx).x );
      out_dim.y  = std::max(out_dim.y , in_dim(src_idx).y );
      out_dim.ch = std::max(out_dim.ch, in_dim(src_idx).ch);
    }

    // sanity check
    for (int src_idx = 0; src_idx < (int)src_layers.size(); src_idx++) {
      assert((in_dim(src_idx).x  == 1 || in_dim(src_idx).x  == out_dim.x ) && "input dimensions must be either identical or 1");
      assert((in_dim(src_idx).y  == 1 || in_dim(src_idx).y  == out_dim.y ) && "input dimensions must be either identical or 1");
      assert((in_dim(src_idx).ch == 1 || in_dim(src_idx).ch == out_dim.ch) && "input dimensions must be either identical or 1");
    }
  }

  // broadcasting?
  bool bc_x (int src_idx) { return in_dim(src_idx).x  < out_dim.x ; }
  bool bc_y (int src_idx) { return in_dim(src_idx).y  < out_dim.y ; }
  bool bc_ch(int src_idx) { return in_dim(src_idx).ch < out_dim.ch; }
  
  // binary export
  virtual void generateBifLayer(BIF::LAYER &bl) {
    FusedFunc::generateBifLayer(bl);
    bl.elwise_0_left_shift = input_shift_left_0;
    bl.elwise_1_left_shift = input_shift_left_1;
  }

  virtual void setSegmentDimensions();
  virtual SEGMENT *getSegment(int x, int y, int in_ch, int out_ch);

  virtual bool compatibleSegmentsBlock(const SEGMENT *a, const SEGMENT *b, int lane, int lane_out_ch) const;
  virtual int firstInputChannel(int x, int y, int out_ch, int src_idx);
  virtual int lastInputChannel(int x, int y, int out_ch, int src_idx);
  virtual int nextInputChannel(int x, int y, int in_ch, int out_ch, int src_idx);
  virtual int numUsedInputChannels(int x, int y, int out_ch, int src_idx);
  virtual bool usesInputCh(int x, int y, int in_ch, int out_ch, int src_idx);
  
  virtual void generateCommands();

  virtual DMA_COMMANDS::DMA_DESCRIPTOR dataLoad(const SEGMENT &segment, int cluster, int unit, BUFFER &buffer, int source/*=0*/);
  virtual void load(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer);
  virtual void compute(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer, BUFFER &store_buffer);

  virtual VPRO_TYPE get_vpro_type() = 0;
}; // class Elementwise


class Add: public Elementwise {
  virtual LAYERTYPE getLayerType() { return LAYERTYPE::ADD; }
  virtual std::string getLayerTypeName() { return "Add"; }
  virtual VPRO_TYPE get_vpro_type() { return VPRO_TYPE::add; }
}; // class Add

class Mul: public Elementwise {
public:
  qparam_t mulh_shift_right{0};

  virtual LAYERTYPE getLayerType() { return LAYERTYPE::MUL; }
  virtual std::string getLayerTypeName() { return "Mul"; }
  virtual VPRO_TYPE get_vpro_type() { return VPRO_TYPE::mul; }

  // binary export
  virtual void generateBifLayer(BIF::LAYER &bl) {
    Elementwise::generateBifLayer(bl);
    bl.conv_result_shift_right = mulh_shift_right;
  }
  
}; // class Mul

} // namespace CNN_LAYER

#endif
