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
#ifndef SCATTER_TO_GRID_H
#define SCATTER_TO_GRID_H

#include "Base/base_layer.h"

namespace CNN_LAYER {

enum SCATTER_POOL_MODE {
  NONE = 0, // just scatter (no pooling)
  MAX = 1 // max-pooling
};

class ScatterToGrid: public Layer {
  // user-supplied parameters
public:
  
  // grid configuration
  float xmin{0.};
  float xmax{0.};
  float ymin{0.};
  float ymax{0.};
  float res{0.};
  SCATTER_POOL_MODE pool_mode{SCATTER_POOL_MODE::NONE};
  bool use_vpro_dma{};

  qparam_t index_shift;
  qparam_t xmin_fixed;
  qparam_t ymin_fixed;

  // methods
public:
  virtual std::string getLayerTypeName() { return "ScatterToGrid"; }
  virtual LAYERTYPE getLayerType() { return LAYERTYPE::SCATTER_TO_GRID; }

  virtual void processParams() {
    assert(src_layers.size() == 2 && "expecting inputs of format [grid indices, features] or [coordinates, features]");
    assert(xmin != xmax && ymin != ymax);
    
    n_cells_x = floor((xmax-xmin) / res);
    n_cells_y = floor((ymax-ymin) / res);
    n_cells = n_cells_x * n_cells_y;
    
    // get the size of memory transfers for copying grid in riscv data cache into vpro memory section
    uint32_t size = ceil_div((uint32_t)n_cells * out_dim.ch, VPRO_CFG::CLUSTERS);
    while (size > 2048) //VPRO_CFG::LM_SIZE)
      size = ceil_div(size, 2u);
    memcopy_size = uint16_t(size);
    //memcopy_size = 4000;
    //memcopy_size = int16_t(VPRO_CFG::LM_SIZE);

    Layer::processParams();
  };

  virtual void computeOutputDim() {    
    out_dim.x = n_cells_x;
    out_dim.y = n_cells_y;
    out_dim.ch = in_dim(1).ch;
  }

  virtual std::string getLayerInfoText() {
    std::stringstream ss;
    ss << Layer::getLayerInfoText();
    ss << "  x=[" << xmin << "," << xmax << "], y=[" << ymin << "," << ymax << "], res=" << res << ", cells " << n_cells_x << "x" << n_cells_y << "\n";
    return ss.str();
  }

  virtual int expectedWeightCount() {
    return 0;
  }

  /*============================================ Command generation =====================================================*/

  void generateCommands() {
    BIF::COMMAND_SEGMENT cmd;

    for (int oc=0; oc < out_dim.ch; oc++) {
      cmd = BIF::COMMAND_SEGMENT{};
      cmd.type.type = SCATTER_CMD;
      cmd.scatter.index_shift = index_shift;
      cmd.scatter.xmin_fixed = xmin_fixed;
      cmd.scatter.ymin_fixed = ymin_fixed;
      cmd.scatter.mm_addr_coords = in_dim(0).mm.base;
      cmd.scatter.mm_addr_features = in_dim(1).mm.channel_base[oc];
      cmd.scatter.mm_addr_grid = out_dim.mm.channel_base[oc];
      cmd.scatter.memcopy_size = memcopy_size;
      cmd.scatter.use_vpro_dma = uint16_t(use_vpro_dma);
      commands.push_back(cmd);
    }
}

  void compressCommands() {} // no compression
  
protected:
  int n_cells_x{0};
  int n_cells_y{0};
  int n_cells{0};
  uint16_t memcopy_size{};

}; // class ScatterToGrid

} // namespace CNN_LAYER

#endif // SCATTER_TO_GRID_H
