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
#include "bif.h"

const char* mmAddrStr(mm_addr_type addr) {
  static char buf[32];
  sprintf(buf, "0x%08" PRIx64, addr);
  return buf;
}

const char* to_bin(size_t const size, void const *const ptr, char *buf) {
  static char static_buf[256];
  if (!buf)
    buf = static_buf;
  uint64_t v = *(uint64_t *)ptr;

  char *p = buf;
  for (int i = size - 1; i >= 0; i--) {
    *p++ = '0' + ((v >> i) & 1);
  }
  *p = 0;
  return buf;
}

const char* to_char(POOLTYPE type) {
  switch(type) {
  case NO_POOLING: return "NONE";
  case MAX_POOLING: return "MAX_POOLING";
  case AVG_POOLING: return "AVG_POOLING";
  }
  return "unknown POOLTYPE";
};

const char* to_char(VPRO_TYPE type) {
  switch (type) {
  case conv_start           : return "conv_start";
  case conv_add             : return "conv_add";
  case maxpool2x2_fused     : return "maxpool2x2_fused";
  case activation_fused     : return "activation_fused";
  case shift_store          : return "shift_store";
  case add                  : return "add";
  case conv_transpose_add   : return "conv_transpose_add";
  case conv_transpose_start : return "conv_transpose_start";
  case conv1d_start         : return "conv1d_start";
  case conv1d_add           : return "conv1d_add";
  case concatenate          : return "concatenate";
  case depth_to_space       : return "depth_to_space";
  case dconv_deform_8x8     : return "dconv_deform_8x8";
  case dconv_conv_start     : return "dconv_conv_start";
  case dconv_conv_add       : return "dconv_conv_add";
  case avgpool2d            : return "avgpool2d";
  case relu_pool_scatter    : return "relu_pool_scatter";
  case max_pooling          : return "max_pooling";
  case set_masks            : return "set_masks";
  case reset_indices        : return "reset_indices";
  case mul                  : return "mul";
  case global_avgpool2d_start:return "global_avgpool2d_start";
  case global_avgpool2d_add : return "global_avgpool2d_add";
  case global_avgpool2d_store_intermediates: return "global_avgpool2d_store_intermediates";
  case global_avgpool2d_sum_intermediates : return "global_avgpool2d_sum_intermediates";
  case global_avgpool2d_divide: return "global_avgpool2d_divide";
  case shift_store_upsample : return "shift_store_upsample";
  default                   : return "unknown";
  }
  return "unknown VPRO_TYPE";
};

const char* to_char(DMA_DIRECTION dir) {
  switch (dir) {
  case e2l1D: return "e2l1D";
  case e2l2D: return "e2l2D";
  case l2e1D: return "l2e1D";
  case l2e2D: return "l2e2D";
  case loop:  return "loop";
  }
  return "unkown DMA_DIRECTION";
}

const char* to_char(ACTIVATION type) {
  switch(type) {
  case LEAKY          : return "LEAKY";
  case RECT           : return "RECT";
  case RELU6          : return "RELU6";
  case SIGMOID        : return "SIGMOID";
  case SWISH          : return "SWISH";
  case NO_ACTIVATION  : return "NONE";
  }
  return "unknown ACTIVATION";
};

const char* to_char(LAYERTYPE type) {
  switch(type) {
  case CONV2            : return "CONV2";
  case ADD              : return "ADD";
  case MUL              : return "MUL";
  case CONV2_TRANSPOSE  : return "CONV2_TRANSPOSE";
  case CONV1            : return "CONV1";
  case CONCATENATE      : return "CONCATENATE";
  case DEPTH_TO_SPACE   : return "DEPTH_TO_SPACE";
  case SCATTER_TO_GRID  : return "SCATTER_TO_GRID";
  case DYNAMIC_AXIS     : return "DYNAMIC_AXIS";
  case MAXPOOL2D        : return "MAXPOOL2D";
  case AVGPOOL2D        : return "AVGPOOL2D";
  case DCONV_DEFORM     : return "DCONV_DEFORM";
  case DCONV_CONV       : return "DCONV_CONV";
  case UNKNOWN_LAYERTYPE: return "UNKNOWN";
  case POINTPILLARS     : return "POINTPILLARS";
  case GLOBALAVGPOOL2D  : return "GLOBALAVGPOOL2D";
  case GLOBALMAXPOOL2D  : return "GLOBALMAXPOOL2D";
  }
  return "unknown LAYERTYPE";
};

const char* to_char(COMMAND_SEGMENT_TYPE cmdtype) {
  switch (cmdtype) {
  case DMA_CMD                     : return "DMA_CMD";
  case VPRO_CMD                    : return "VPRO_CMD";
  case DMA_WAIT                    : return "DMA_WAIT";
  case VPRO_WAIT                   : return "VPRO_WAIT";
  case DMA_BLOCK                   : return "DMA_BLOCK";
  case BOTH_SYNC                   : return "BOTH_SYNC";
  case SCATTER_CMD                 : return "SCATTER_CMD";
  case DMA_SET_PADDING             : return "DMA_SET_PADDING";
  case UNKNOWN_COMMAND_SEGMENT_TYPE: return "UNKNOWN";
  }
  return "unkown COMMAND_SEGMENT_TYPE";
}
