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
#ifndef VPRO_KERNELS_H
#define VPRO_KERNELS_H

#include "kernels/activation_kernel.h"
#include "kernels/activation_sigmoid_kernel.h"
#include "kernels/conv1d_kernel.h"
#include "kernels/conv2d_kernel.h"
#include "kernels/conv2d_transpose_kernel.h"
#include "kernels/deform_kernel.h"
#include "kernels/dconv_conv_kernel.h"
#include "kernels/elwise_kernel.h"
#include "kernels/load_kernel.h"
#include "kernels/store_kernel.h"
#include "kernels/concat_kernel.h"
#include "kernels/depth_to_space_kernel.h"
#include "kernels/scatter_to_grid_kernel.h"
#include "kernels/pointpillars_kernel.h"
#include "kernels/dynamic_shape_kernel.h"
#include "kernels/avgpool2d_kernel.h"
#include "kernels/maxpool2d_kernel.h"

#endif // VPRO_KERNELS_H
