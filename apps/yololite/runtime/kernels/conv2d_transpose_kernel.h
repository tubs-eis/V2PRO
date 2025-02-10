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
#ifndef CONV2D_TRANSPOSE_KERNEL
#define CONV2D_TRANSPOSE_KERNEL

#include "bif.h"
#include "vpro_functions.h"
#include <vpro.h>
#include "eisv.h"

#define DEBUG_TRANSPOSE_CONV_KERNEL 0

/**
 * Conv2DTranspose VPRO Kernel
 * 
 * Calculates transpose convolution of 2D Inputs
 *  - For a transpose convolution with stride s there are s² different cases the kernel can be offset on the input values without repetition
 *    Calulate these different cases and slide them in over the inputs with a stride of s
 *  - Cases are calculated by implicitly using the 'grid'-padded zeros explained here: https://towardsdatascience.com/what-is-transposed-convolutional-layer-40e5e6e31c11
 *  - Most parameters are only depending on stride and kernel_length
 *  
**/
struct conv_transpose_table_entry {
  unsigned int offset_in_start;
  unsigned int offset_out_start;
  unsigned int x_end;
  unsigned int y_end;
  uint32_t kernel_src_op;
}; 
#define MAX_CONV_TRANSPOSE_STRIDE 7
extern conv_transpose_table_entry conv_transpose_table[MAX_CONV_TRANSPOSE_STRIDE*MAX_CONV_TRANSPOSE_STRIDE];

inline void _conv_transpose_init_table(const BIF::LAYER &layer)
{
    #if DEBUG_TRANSPOSE_CONV_KERNEL
        sim_printf("\n##### _conv_transpose_init_table #####\n");
        sim_printf("kernel: %dx%d, stride: %dx%d", layer.kernel_length, layer.kernel_length, layer.stride, layer.stride);
        sim_printf("layer.seg_in_w: %d, layer.seg_in_h: %d, layer.seg_out_w: %d, layer.seg_out_h: %d", layer.seg_in_w, layer.seg_in_h, layer.seg_out_w, layer.seg_out_h);
        sim_printf("pad.left: %d, pad.top: %d, pad.right: %d, pad.bot: %d", layer.pad.left, layer.pad.top, layer.pad.right, layer.pad.bottom);
        sim_printf("subpixel_pad.left: %d, subpixel_pad.top: %d", layer.subpixel_pad.left, layer.subpixel_pad.top);
        sim_printf("\n");
    #endif

    // Factor for boundary and used pixel for each different convolution case
    unsigned int s_factor = layer.kernel_length / layer.stride;

    // Boundary for case distinction 
    int case_boundary = layer.kernel_length - (s_factor - 1) * layer.stride - layer.stride - 1;

    #if DEBUG_TRANSPOSE_CONV_KERNEL
        sim_printf("s_factor: %d, case_boundary: %d", s_factor, case_boundary);
        sim_printf("w: %d, h: %d", w, h);
        sim_printf("\n");
    #endif

    assert(layer.stride <= MAX_CONV_TRANSPOSE_STRIDE);
    int tab_idx = 0;       
    for (int i_y = 0; i_y < layer.stride; ++i_y)
    {
        // FIXME replace modulo operators
        int s_y = (layer.stride + ((i_y - (int)layer.subpixel_pad.top) % (int)layer.stride)) % (int)layer.stride;
        for (int i_x = 0; i_x < layer.stride; ++i_x)
        {
            //sim_printf("i_x: %d, i_y: %d", i_x, i_y);

            int s_x = (layer.stride + ((i_x - (int)layer.subpixel_pad.left) % (int)layer.stride)) % (int)layer.stride;
            #if DEBUG_TRANSPOSE_CONV_KERNEL
                sim_printf("i_x: %d, i_y: %d, s_x: %d, s_y: %d", i_x, i_y, s_x, s_y);
            #endif

            // limits for x and y hardware loops
            unsigned int x_end = 0;
            unsigned int y_end = 0;

            // offset for kernel
            unsigned int kernel_offset_x = 0;
            unsigned int kernel_offset_y = 0;

            // start input pixel
            unsigned int start_in_pixel_x = 0;
            unsigned int start_in_pixel_y = 0;

            // buffer 'inverse' offset for subpixel padding
            unsigned int pixel_offset_left = 0;
            unsigned int pixel_offset_top = 0;

            // stride * stride different convolution cases
            if(s_x == 0 && s_y == 0)
            {
                #if DEBUG_TRANSPOSE_CONV_KERNEL
                    sim_printf("### TOP LEFT CORNER #############\n");
                    sim_printf("X##|###\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                    sim_printf("-------\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                #endif
                x_end = case_boundary == -1 ? s_factor : s_factor + 1;
                y_end = case_boundary == -1 ? s_factor : s_factor + 1;

                kernel_offset_x = 0;
                kernel_offset_y = 0;

                pixel_offset_left = 1;
                pixel_offset_top = 1;

                start_in_pixel_x = 0;
                start_in_pixel_y = 0;
            }
            else if (s_x > layer.stride - case_boundary - 1 && s_y == 0)
            {
                #if DEBUG_TRANSPOSE_CONV_KERNEL
                    sim_printf("### TOP RIGHT EDGE #############\n");
                    sim_printf("###|XXX\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                    sim_printf("-------\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                #endif
                x_end = case_boundary == -1 ? s_factor : s_factor + 1;
                y_end = case_boundary == -1 ? s_factor : s_factor + 1;

                kernel_offset_x = layer.stride - s_x;
                kernel_offset_y = 0;

                if (layer.subpixel_pad.left == 0)
                {
                    pixel_offset_left = 0;
                }
                else
                {
                    pixel_offset_left = i_x - layer.subpixel_pad.left > 0 ? 0 : 1;
                }
                //pixel_offset_left = layer.subpixel_pad.left != 0 ? 1 : 0;
                pixel_offset_top = layer.subpixel_pad.top != 0 ? 1 : 0;

                start_in_pixel_x = 1 - pixel_offset_left;
                start_in_pixel_y = 0;
            }
            else if (s_x == 0 && s_y > layer.stride - case_boundary - 1)
            {
                #if DEBUG_TRANSPOSE_CONV_KERNEL
                    sim_printf("### LEFT BOTTOM EDGE #############\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                    sim_printf("-------\n");
                    sim_printf("X##|###\n");
                    sim_printf("X##|###\n");
                    sim_printf("X##|###\n");
                #endif
                x_end = case_boundary == -1 ? s_factor : s_factor + 1;
                y_end = case_boundary == -1 ? s_factor : s_factor + 1;

                kernel_offset_x = 0;
                kernel_offset_y = layer.stride - s_y;

                if (layer.subpixel_pad.top == 0)
                {
                    pixel_offset_top = 0;
                }
                else
                {
                    pixel_offset_top = i_y - layer.subpixel_pad.top > 0 ? 0 : 1;
                }
                pixel_offset_left = layer.subpixel_pad.left != 0 ? 1 : 0;
                //pixel_offset_top = layer.subpixel_pad.top != 0 ? 1 : 0;

                start_in_pixel_x = 0;
                start_in_pixel_y = 1 - pixel_offset_top;
            }
            else if (s_x <= layer.stride - case_boundary - 1 && s_y == 0)
            {
                #if DEBUG_TRANSPOSE_CONV_KERNEL
                    sim_printf("### TOP LEFT EDGE #############\n");
                    sim_printf("#XX|###\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                    sim_printf("-------\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                #endif
                x_end = case_boundary == -1 ? s_factor : s_factor;
                y_end = case_boundary == -1 ? s_factor : s_factor + 1;

                kernel_offset_x = layer.stride - s_x;
                kernel_offset_y = 0;

                if (layer.subpixel_pad.left == 0)
                {
                    pixel_offset_left = 0;
                }
                else
                {
                    pixel_offset_left = i_x - layer.subpixel_pad.left > 0 ? 0 : 1;
                }
                //pixel_offset_left = s_x - layer.subpixel_pad.left > 0 ? 1 : 0;
                pixel_offset_top = layer.subpixel_pad.top != 0 ? 1 : 0;

                start_in_pixel_x = 1 - pixel_offset_left;
                start_in_pixel_y = 0;
            }
            else if (s_x == 0 && s_y <= layer.stride - case_boundary - 1)
            {
                #if DEBUG_TRANSPOSE_CONV_KERNEL
                    sim_printf("### LEFT TOP EDGE #############\n");
                    sim_printf("###|###\n");
                    sim_printf("X##|###\n");
                    sim_printf("X##|###\n");
                    sim_printf("-------\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                #endif
                x_end = case_boundary == -1 ? s_factor : s_factor + 1;
                y_end = case_boundary == -1 ? s_factor : s_factor;

                kernel_offset_x = 0;
                kernel_offset_y = layer.stride - s_y;

                if (layer.subpixel_pad.top == 0)
                {
                    pixel_offset_top = 0;
                }
                else
                {
                    pixel_offset_top = i_y - layer.subpixel_pad.top > 0 ? 0 : 1;
                }
                pixel_offset_left = layer.subpixel_pad.left != 0 ? 1 : 0;
                //pixel_offset_top = s_y - layer.subpixel_pad.top > 0 ? 1 : 0;

                start_in_pixel_x = 0;
                start_in_pixel_y = 1 - pixel_offset_top;
            }
            else if (s_x <= layer.stride - case_boundary - 1 && s_y > layer.stride - case_boundary - 1)
            {
                #if DEBUG_TRANSPOSE_CONV_KERNEL
                    sim_printf("### BOTTOM LEFT EDGE #############\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                    sim_printf("-------\n");
                    sim_printf("#XX|###\n");
                    sim_printf("#XX|###\n");
                    sim_printf("#XX|###\n");
                #endif
                x_end = case_boundary == -1 ? s_factor : s_factor;
                y_end = case_boundary == -1 ? s_factor : s_factor + 1;

                kernel_offset_x = layer.stride - s_x;
                kernel_offset_y = layer.stride - s_y;

                if (layer.subpixel_pad.left == 0)
                {
                    pixel_offset_left = 0;
                }
                else
                {
                    pixel_offset_left = i_x - layer.subpixel_pad.left > 0 ? 0 : 1;
                }

                if (layer.subpixel_pad.top == 0)
                {
                    pixel_offset_top = 0;
                }
                else
                {
                    pixel_offset_top = i_y - layer.subpixel_pad.top > 0 ? 0 : 1;
                }
                //pixel_offset_left = s_x - layer.subpixel_pad.left > 0 ? 1 : 0;
                //pixel_offset_top = layer.subpixel_pad.top != 0 ? 1 : 0;

                start_in_pixel_x = 1 - pixel_offset_left;
                start_in_pixel_y = 1 - pixel_offset_top;
            }
            else if (s_x > layer.stride - case_boundary - 1 && s_y <= layer.stride - case_boundary - 1)
            {
                #if DEBUG_TRANSPOSE_CONV_KERNEL
                    sim_printf("### TOP RIGHT EDGE #############\n");
                    sim_printf("###|###\n");
                    sim_printf("###|XXX\n");
                    sim_printf("###|XXX\n");
                    sim_printf("-------\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                #endif
                x_end = case_boundary == -1 ? s_factor : s_factor + 1;
                y_end = case_boundary == -1 ? s_factor : s_factor;

                kernel_offset_x = layer.stride - s_x;
                kernel_offset_y = layer.stride - s_y;

                if (layer.subpixel_pad.left == 0)
                {
                    pixel_offset_left = 0;
                }
                else
                {
                    pixel_offset_left = i_x - layer.subpixel_pad.left > 0 ? 0 : 1;
                }

                if (layer.subpixel_pad.top == 0)
                {
                    pixel_offset_top = 0;
                }
                else
                {
                    pixel_offset_top = i_y - layer.subpixel_pad.top > 0 ? 0 : 1;
                }
                //pixel_offset_left = layer.subpixel_pad.left != 0 ? 1 : 0;
                //pixel_offset_top = s_y - layer.subpixel_pad.top > 0 ? 1 : 0;

                start_in_pixel_x = 1 - pixel_offset_left;
                start_in_pixel_y = 1 - pixel_offset_top;
            }
            else if (s_x > layer.stride - case_boundary - 1 && s_y > layer.stride - case_boundary - 1)
            {
                #if DEBUG_TRANSPOSE_CONV_KERNEL
                    sim_printf("### BOTTOM RIGHT Corner #############\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                    sim_printf("-------\n");
                    sim_printf("###|XXX\n");
                    sim_printf("###|XXX\n");
                    sim_printf("###|XXX\n");
                #endif
                x_end = case_boundary == -1 ? s_factor : s_factor + 1;
                y_end = case_boundary == -1 ? s_factor : s_factor + 1;

                kernel_offset_x = layer.stride - s_x;
                kernel_offset_y = layer.stride - s_y;

                if (layer.subpixel_pad.left == 0)
                {
                    pixel_offset_left = 0;
                }
                else
                {
                    pixel_offset_left = i_x - layer.subpixel_pad.left > 0 ? 0 : 1;
                }

                if (layer.subpixel_pad.top == 0)
                {
                    pixel_offset_top = 0;
                }
                else
                {
                    pixel_offset_top = i_y - layer.subpixel_pad.top > 0 ? 0 : 1;
                }
                //pixel_offset_left = s_x - layer.subpixel_pad.left > 0 ? 1 : 0;
                //pixel_offset_top = s_y - layer.subpixel_pad.top > 0 ? 1 : 0;

                start_in_pixel_x = 1 - pixel_offset_left;
                start_in_pixel_y = 1 - pixel_offset_top;
            }
            else if (s_x <= layer.stride - case_boundary - 1 && s_y <= layer.stride - case_boundary - 1)
            {
                #if DEBUG_TRANSPOSE_CONV_KERNEL
                    sim_printf("### TOP LEFT MIDDLE #############\n");
                    sim_printf("###|###\n");
                    sim_printf("#XX|###\n");
                    sim_printf("#XX|###\n");
                    sim_printf("-------\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                    sim_printf("###|###\n");
                #endif
                x_end = case_boundary == -1 ? s_factor : s_factor;
                y_end = case_boundary == -1 ? s_factor : s_factor;

                kernel_offset_x = layer.stride - s_x;
                kernel_offset_y = layer.stride - s_y;

                if (layer.subpixel_pad.left == 0)
                {
                    pixel_offset_left = 0;
                }
                else
                {
                    pixel_offset_left = i_x - layer.subpixel_pad.left > 0 ? 0 : 1;
                }

                if (layer.subpixel_pad.top == 0)
                {
                    pixel_offset_top = 0;
                }
                else
                {
                    pixel_offset_top = i_y - layer.subpixel_pad.top > 0 ? 0 : 1;
                }
                //pixel_offset_left = s_x - layer.subpixel_pad.left > 0 ? 1 : 0;
                //pixel_offset_top = s_y - layer.subpixel_pad.top > 0 ? 1 : 0;

                start_in_pixel_x = 1 - pixel_offset_left;
                start_in_pixel_y = 1 - pixel_offset_top;
            }

            #if DEBUG_TRANSPOSE_CONV_KERNEL
                sim_printf("Calculating with params...\n");
                sim_printf("x_end: %d, y_end: %d", x_end, y_end);
                sim_printf("kernel_offset_x: %d, kernel_offset_y: %d", kernel_offset_x, kernel_offset_y);
                sim_printf("pixel_offset_left: %d, pixel_offset_top: %d", pixel_offset_left, pixel_offset_top);
                sim_printf("start_in_pixel_x: %d, start_in_pixel_y: %d", start_in_pixel_x, start_in_pixel_y);
                sim_printf("...\n");
            #endif

            conv_transpose_table[tab_idx] =
              {
               .offset_in_start = start_in_pixel_y * layer.seg_in_w + start_in_pixel_x,
               .offset_out_start = (unsigned)i_x + layer.seg_out_w * i_y,
               .x_end = x_end,
               .y_end = y_end,
               .kernel_src_op = SRC_ADDR_4(RF_KERNEL_BASE + (kernel_offset_y * layer.kernel_length) + kernel_offset_x, layer.stride, layer.stride * layer.kernel_length, 0)
              };
            tab_idx++;
        }
    }
}

inline void _conv_transpose_start(const BIF::LAYER &layer, const uint16_t buffer) {
    // Actual number of input pixels without the implicit padded zeros in width and height per segment
    unsigned int w = layer.input_pixels_w;
    unsigned int h = layer.input_pixels_h;

    for (int tab_idx = 0; tab_idx < layer.stride*layer.stride; tab_idx++) {
        unsigned int offset_in = conv_transpose_table[tab_idx].offset_in_start;
        unsigned int offset_out = conv_transpose_table[tab_idx].offset_out_start;
        unsigned int x_end = conv_transpose_table[tab_idx].x_end;
        unsigned int y_end = conv_transpose_table[tab_idx].y_end;
        uint32_t kernel_src_op = conv_transpose_table[tab_idx].kernel_src_op;
#if RV_VPRO_EXT == 1 and not defined(SIMULATION)
        // VPRO loads
        c_vpro_lw<5, VPRO_PARAMETER_INDIZES::x_y_z_end, VPRO_PARAMETER_INDIZES::src2_imm, NoTrigger>
          (uint32_t(x_end-1) << (10 + 6) | uint32_t(y_end-1) << 10 | uint32_t(w-1), SRC2_IMM_3D(buffer));

        // VPRO mach
        c_vpro_lw<6, VPRO_PARAMETER_INDIZES::src2_all, VPRO_PARAMETER_INDIZES::x_y_z_end, NoTrigger>
            (kernel_src_op, uint32_t(x_end-1) << (10 + 6) | uint32_t(y_end-1) << 10 | uint32_t(w-1));
#endif
        
        for (size_t y = 0; y < h; ++y) {
#if DEBUG_TRANSPOSE_CONV_KERNEL
            sim_printf("offset_in: %d, offset_out: %d", offset_in, offset_out);
#endif
#if RV_VPRO_EXT == 1 and not defined(SIMULATION)
            c_vpro_lw<5, VPRO_PARAMETER_INDIZES::src1_offset, VPRO_PARAMETER_INDIZES::nowhere, Trigger>
                (offset_in, 0);
            c_vpro_lw<6, VPRO_PARAMETER_INDIZES::dst_all, VPRO_PARAMETER_INDIZES::src1_all, Trigger>
              (DST_ADDR(offset_out, 0, 0, layer.stride), complex_ADDR_3D(SRC_SEL_LS, RF_BIAS_BASE, 0, 0, 0));
            // FIXME one nop required here, why? nn_quant testcases failing w/o nop:
            // run_params.append((184, 128,   5,  5, True , 6, 4, 'same' , 1, None       , None       , 3723907568))
            // run_params.append(( 16, 180,   4,  4, True , 7, 4, 'same' , 4, None       , 'leakyrelu',  574236635))
            // run_params.append((170,  97,   2,  2, True , 3, 2, 'same' , 1, None       , 'leakyrelu', 1824502918))
            // run_params.append((179, 167,   4,  4, True , 6, 5, 'same' , 4, None       , 'relu'     , 2542701066))
            asm volatile ("nop");
#else
            VPRO::DIM3::LOADSTORE::loads(
                                         buffer, 
                                         offset_in,
                                         1, layer.seg_in_w, 1,
                                         x_end-1, y_end-1, w-1
                                         );
            VPRO::DIM3::PROCESSING::mach_init_addr(
                                                   L0_1,
                                                   DST_ADDR(offset_out, 0, 0, layer.stride),
                                                   SRC1_LS_3D,
                                                   kernel_src_op,
                                                   x_end-1, y_end-1, w-1,
                                                   RF_BIAS_BASE, 0, 0, 0, // initialize with bias value
                                                   false, true
                                                   );
#endif // RV_VPRO_EXT
            offset_in += layer.seg_in_w;
            offset_out += layer.seg_out_w * layer.stride;
        }
    }
}
       
// same as _conv_transpose_start -> only difference is that it uses already written register file values and adds convolution to it
inline void _conv_transpose_add(const BIF::LAYER &layer, const uint16_t buffer) {
    unsigned int w = layer.input_pixels_w;
    unsigned int h = layer.input_pixels_h;

    for (int tab_idx = 0; tab_idx < layer.stride*layer.stride; tab_idx++) {
        unsigned int offset_in = conv_transpose_table[tab_idx].offset_in_start;
        unsigned int offset_out = conv_transpose_table[tab_idx].offset_out_start;
        unsigned int x_end = conv_transpose_table[tab_idx].x_end;
        unsigned int y_end = conv_transpose_table[tab_idx].y_end;
        uint32_t kernel_src_op = conv_transpose_table[tab_idx].kernel_src_op;
#if RV_VPRO_EXT == 1 and not defined(SIMULATION)
        // VPRO loads
        c_vpro_lw<5, VPRO_PARAMETER_INDIZES::x_y_z_end, VPRO_PARAMETER_INDIZES::src2_imm, NoTrigger>
          (uint32_t(x_end-1) << (10 + 6) | uint32_t(y_end-1) << 10 | uint32_t(w-1), SRC2_IMM_3D(buffer));

        // VPRO mach
        c_vpro_lw<6, VPRO_PARAMETER_INDIZES::src2_all, VPRO_PARAMETER_INDIZES::x_y_z_end, NoTrigger>
            (kernel_src_op, uint32_t(x_end-1) << (10 + 6) | uint32_t(y_end-1) << 10 | uint32_t(w-1));
#endif

        for (size_t y = 0; y < h; ++y) {
#if DEBUG_TRANSPOSE_CONV_KERNEL
            sim_printf("offset_in: %d, offset_out: %d", offset_in, offset_out);
#endif
#if RV_VPRO_EXT == 1 and not defined(SIMULATION)
            c_vpro_lw<5, VPRO_PARAMETER_INDIZES::src1_offset, VPRO_PARAMETER_INDIZES::nowhere, Trigger>
                (offset_in, 0);
            c_vpro_lw<6, VPRO_PARAMETER_INDIZES::dst_all, VPRO_PARAMETER_INDIZES::src1_all, Trigger>
                (DST_ADDR(offset_out, 0, 0, layer.stride), complex_ADDR_3D(SRC_SEL_LS, offset_out, 0, 0, layer.stride));
            // FIXME not tested if nop is required
            asm volatile ("nop");
#else
            VPRO::DIM3::LOADSTORE::loads(
                                         buffer, 
                                         offset_in,
                                         1, layer.seg_in_w, 1,
                                         x_end-1, y_end-1, w-1
                                         );
            VPRO::DIM3::PROCESSING::mach_init_addr(
                                                   L0_1,
                                                   DST_ADDR(offset_out, 0, 0, layer.stride),
                                                   SRC1_LS_3D,
                                                   kernel_src_op,
                                                   x_end-1, y_end-1, w-1,
                                                   offset_out, 0, 0, layer.stride, // adding to values already in register file
                                                   false, true
                                                   );
#endif // RV_VPRO_EXT
            offset_in += layer.seg_in_w;
            offset_out += layer.seg_out_w * layer.stride;
        }
    }
}

#endif // CONV2D_TRANSPOSE_KERNEL
