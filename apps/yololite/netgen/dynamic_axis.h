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
#ifndef DYNAMIC_AXIS_H
#define DYNAMIC_AXIS_H

#include "Base/base_layer.h"

namespace CNN_LAYER {

class DynamicAxis: public Input {
  // user-supplied parameters
public:
  DynamicAxis() {
    produces_binary_data = true;
    is_input_layer = true;
  }
  int16_t axis{0};
  // methods
public:
  virtual std::string getLayerTypeName() { return "DynamicAxis"; }
  virtual LAYERTYPE getLayerType() { return LAYERTYPE::DYNAMIC_AXIS; }

  virtual void generateBifLayer(BIF::LAYER &bl) {
      assert(axis == 0 && "Currently only a dynamic x-axis is supported");
      bl.in_channels = out_dim.ch;
      bl.out_channels = out_dim.ch;
      bl.number = number;
      bl.type = getLayerType();
      bl.axis = axis;

      bl.output.mm_base = out_dim.mm.base;
      bl.output.x = (uint32_t) out_dim.x;
      bl.output.y = (uint32_t) out_dim.y;
      bl.output.y_stride = (uint32_t) out_dim.mm.x;
      bl.output.channels = (uint32_t) out_dim.ch;
    }

  virtual void generateCommands() { return; };
  void compressCommands() {} // no compression
}; // class DynamicAxis


} // namespace CNN_LAYER

#endif // DYNAMIC_AXIS_H
