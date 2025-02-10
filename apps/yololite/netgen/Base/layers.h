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
#ifndef LAYERS_H
#define LAYERS_H

// top-level include file sufficient for layer instantiation
// all available layers shall be registered here

#include "Base/base_layer.h"
#include "Base/fusedfunc_layer.h"
#include "Base/base_input.h"
#include "Base/reshape_layer.h"
#include "Layer_Conv/conv_layer.h"
#include "conv1d_layer.h"
#include "Layer_ConvTranspose/conv_transpose_layer.h"
#include "dynamic_axis.h"
#include "Layer_Elwise/elwise_layer.h"
#include "Layer_Concat/concat_layer.h"
#include "Layer_Depth_To_Space/depth_to_space_layer.h"
#include "slice_layer.h"
#include "scatter_to_grid.h"
#include "pointpillars_layer.h"
#include "Layer_Pooling/pooling_layer.h"
#include "Layer_Pooling/maxpooling2d.h"
#include "Layer_Pooling/globalpool_layer.h"
#include "Layer_DeformableConv/dconv_conv_layer.h"
#include "Layer_DeformableConv/dconv_deform_layer.h"

#endif // LAYERS_H
