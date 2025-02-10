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
#include "conv_transpose_layer.h"
#include "Base/segment.h"
#include "vpro_globals.h"
#include "vpro_cmd_defs.h"

#define DEBUG_SEGMENTATION 0


#if DEBUG_SEGMENTATION
#define PRINTSEG(...) do { printf(__VA_ARGS__); } while (0)
#else
#define PRINTSEG(...)
#endif

namespace CNN_LAYER {

void Conv2DTranspose::setSegmentDimensions() {
    // segmentation is independent of output padding
    PRINTSEG("############## SEGMENTATION ##################\n");

    int kernel_length_x = kernel_length;
    int kernel_length_y = kernel_length;
    int transpose_conv_stride_x = stride; // not to be confused with memory layout x_stride (gap at line end)
    int transpose_conv_stride_y = stride;

    // // all sizes in elements=words, not bytes
    int n_weights = kernel_length_x * kernel_length_y + int(use_bias);

    // // output is stored in RF
    int rf_free_entries = RF_DISCARD_ADDR - n_weights;
    if (activation == RELU6) {
      rf_free_entries -= 1; // one entry required for shifted six
    }

    // inputs stored in local memory; halved for double buffering
    int lm_free_entries = VPRO_CFG::LM_SIZE / 4 - VPRO_CFG::LANES * n_weights; // each lane computes one output channel

    struct
    {
        segDim seg; // c.seg is unused (stored directly in Layer::seg) -> available to other member functions inside loop

        // per usage, one unit computes seg.out.w*seg.out.h pixels for up to VPRO_CFG::LANES output channels
        int unit_usages{};

        int ccpp{};   // compute cycles per pass
        int passes{}; // number of times the vector lanes are started (i.e. _conv[_add]() is called by runtime)
        int discarded_out_pixels{};
        int used_rf_entries{};
        int used_lm_entries{};

        struct
        {
            int commands{};
            int dma_in{}; // DMA input data from MM into LM
            int compute{};
            int dma_out{}; // DMA output data from LM to MM
            int penalty{};
            int total{}; // sum
        } cost;
    } c, best; // current, lowest cost

    best.cost.total = INT_MAX;

#if DEBUG_SEGMENTATION // compiler warning "unused variable"
    // estimate lower bound: output image has to fit into the sum of all RFs
    int min_seg_num = ceil_div(conv_out_dim.x*conv_out_dim.y, rf_free_entries);
    PRINTSEG("conv_out_dim %d x %d = %d px (padded (=cropped) output image %d x %d = %d px)\n",
             conv_out_dim.x, conv_out_dim.y, conv_out_dim.x*conv_out_dim.y,
             out_dim.x, out_dim.y, out_dim.x*out_dim.y
             );
    PRINTSEG("rf_free_entries %d ==> ceil(%.2f) = %d segment(s) required\n", rf_free_entries, 1.0*conv_out_dim.x*conv_out_dim.y/rf_free_entries, min_seg_num);
    PRINTSEG("segmentations with minimum number of segments:\n");
    for (int x = 1; x <= min_seg_num; x++) {
        int y = ceil_div(min_seg_num, x);
        PRINTSEG("num %3d x %3d = %4d   min out %3d x %3d -> %3d x %3d\n",
                 x, y, x*y,
                 round_up(ceil_div(conv_out_dim.x, x), transpose_conv_stride_x),   round_up(ceil_div(conv_out_dim.y, y), transpose_conv_stride_y),
                 round_up(ceil_div(conv_out_dim.x, x), transpose_conv_stride_x)*x, round_up(ceil_div(conv_out_dim.y, y), transpose_conv_stride_y)*y
                 );
    }
#endif // DEBUG_SEGMENTATION

    // FIXME loop over seg.IN.(w|h) instead of seg.OUT.(w|h)? Adding one output pixel does not make sense, one additional input pixel is sufficient for stride output pixels

#define WORKAROUND_DMA_BROKEN_EMU 1 // workaround for broken DMA in emulation: run_params.append(( 2,  2,  4,  4, False , 3, 2, 'valid', 1, None, None,  1)); remove when FPGA has been fixed, check performance
#if WORKAROUND_DMA_BROKEN_EMU
    int max_seg_out_w = std::min({(int)MAX_Z_END, rf_free_entries, conv_out_dim.x}); // z_end: HW limitation; rf: height 1; conv_out_dim: kernel requires seg.out.w to be an integer multiple of transpose_conv_stride, allow that to fit into a single segment for small image sizes
#else    
    int max_seg_out_w = std::min({(int)MAX_Z_END, rf_free_entries, round_up(conv_out_dim.x, transpose_conv_stride_x)}); // z_end: HW limitation; rf: height 1; conv_out_dim: kernel requires seg.out.w to be an integer multiple of transpose_conv_stride, allow that to fit into a single segment for small image sizes
#endif
    PRINTSEG("max_seg_out_w: %d\n", max_seg_out_w);
    for (seg.out.w = 1; seg.out.w <= max_seg_out_w; seg.out.w++)
    {
        PRINTSEG("\n");//####### seg.out.w: %2d", seg.out.w);

        // Input Segment can only jump one pixel in stride resulting in num stride outputpixels
        // in order to compute segments on parallel lanes, they must have the same input subpixel shift
        // kernel implementation assumes seg.out.w to be an integer multiple of transpose_conv_stride
        if (seg.out.w % transpose_conv_stride_x != 0)
        {
            PRINTSEG("seg.out.w %2d is not a multiple of transpose_conv_stride", seg.out.w);
            continue;
        }

        // seg width has to be smaller than max x_end in the complex addressing
        if (seg.out.w - 1 > (signed)MAX_X_END)
        {
            PRINTSEG("seg.out.w %2d >= MAX_X_END %d", seg.out.w, MAX_X_END);
            break;
        }

        seg.in.w = ceil_div(kernel_length_x + (seg.out.w - 1), transpose_conv_stride_x);

        // addressing limitation: (addr distance between two rows, VPRO command parameter)
        if (seg.out.w > (int)MAX_BETA) {
            PRINTSEG("seg.out.w %2d > MAX_BETA %d", seg.out.w, MAX_BETA);
            break;
        }

#if WORKAROUND_DMA_BROKEN_EMU
        for (seg.out.h = 1; seg.out.h <= conv_out_dim.y; seg.out.h++)
#else
        for (seg.out.h = 1; seg.out.h <= round_up(conv_out_dim.y, transpose_conv_stride_y); seg.out.h++)
#endif
        {
            PRINTSEG("\nout.w %2d, out.h %2d, in.w %2d, in.h %2d", seg.out.w, seg.out.h, seg.in.w, seg.in.h);
            // does one output channel fit into the RF?
            if (seg.out.w * seg.out.h > rf_free_entries) // can be moved into for loop condition
            {
                PRINTSEG("; not enough memory in register file");     // larger seg.out.h will further increase memory requirement -> break seg.out.h loop
                break;
            }    

            // seg height has to be smaller than max y_end in the complex addressing
            if (seg.out.h - 1 > (signed)MAX_Y_END)
            {
                break;
            }                               

            // Input Segment can only jump one pixel in stride resulting in num stride outputpixels
            // in order to compute segments on parallel lanes, they must have the same input subpixel shift
            // kernel implementation assumes seg.out.w to be an integer multiple of transpose_conv_stride
            if (seg.out.h % transpose_conv_stride_y != 0)
            {
                PRINTSEG("; seg.out.h is not a multiple of transpose_conv_stride");
                continue;
            }

            seg.in.h = ceil_div(kernel_length_y + (seg.out.h - 1), transpose_conv_stride_y);
            PRINTSEG(", in.h: %2d", seg.in.h);

            // does one input channel fit into LM?
            if (seg.in.w * seg.in.h > lm_free_entries)
                break; // larger seg.out.h will not decrease input memory requirement -> break seg.out.h loop

            // does one input channel fit into LM?
            if (seg.out.w * seg.out.h > lm_free_entries)
                break; // larger seg.out.h will not decrease input memory requirement -> break seg.out.h loop

            // how many input pixels are needed to compute the output
            input_pixels_w = ceil_div(seg.out.w - 1, transpose_conv_stride_x);
            input_pixels_h = ceil_div(seg.out.h - 1, transpose_conv_stride_y);

            // offset_out: maximum LM offset for output line start
            if (seg.out.w * transpose_conv_stride_y * (input_pixels_h + 1) > (int)MAX_OFFSET)
            {
                PRINTSEG("; segment too large: out_offset > MAX_OFFSET");
                break;
            }

            // offset_in: maximum LM offset for input line start
            if (seg.in.w * (input_pixels_h + 1) > (int)MAX_OFFSET)
            {
                PRINTSEG("; segment too large: in_offset > MAX_OFFSET");
                break;
            }

            // geometry: number of segments per (input|output) channel
            seg.num.x = ceil_div(conv_out_dim.x, seg.out.w);
            seg.num.y = ceil_div(conv_out_dim.y, seg.out.h);

            seg.in.x_stride = seg.out.w / transpose_conv_stride_x;
            seg.in.y_stride = seg.out.h / transpose_conv_stride_y;

            seg.out.x_stride = seg.out.w;
            seg.out.y_stride = seg.out.h;

            // -- here: ALL fields of seg are set

            // padding must fit into single segment; splitting padding across multiple segments not implemented
            computeDmaPadding();
            int min_seg_in_w;
            if (seg.num.x < 2)
                min_seg_in_w = padding.dma.left + padding.dma.right; // one segment handles both paddings
            else
                min_seg_in_w = std::max(padding.dma.left, padding.dma.right);
            if (seg.in.w < min_seg_in_w)
                continue;

            int min_seg_in_h;
            if (seg.num.y < 2)
                min_seg_in_h = padding.dma.top + padding.dma.bottom; // one segment handles both paddings
            else
                min_seg_in_h = std::max(padding.dma.top, padding.dma.bottom);
            if (seg.in.h < min_seg_in_h)
                continue;

            // padding widths can only be configured per layer
            // -> all segments have common (t|r|b|l) padding widths or no padding
            // -> padding must be handled by the outermost segments; inner segments must be padding-free
            if (padding.dma.top > seg.in.y_stride ||
                padding.dma.right > seg.in.x_stride ||
                padding.dma.bottom > seg.in.y_stride ||
                padding.dma.left > seg.in.x_stride)
              continue;

            // -- here: segment size can be handled by hardware

            c.unit_usages = seg.num.x * seg.num.y * ceil_div(conv_out_dim.ch, (int)VPRO_CFG::LANES) * VPRO_CFG::LANES;
            int seg_area = (seg.out.w * transpose_conv_stride_x + 1) * (seg.out.h * transpose_conv_stride_y + 1); // prefer square inputs, not outputs

            c.cost.total = c.unit_usages * seg_area;

            PRINTSEG("; POSSIBLE_SEG: num %2d x %2d, in %2d x %2d stride %2d x %2d, out %2d x %2d stride %2d x %2d, cost %6d: unit_usages %4d, seg_area %4d",
                     seg.num.x, seg.num.y, seg.in.w, seg.in.h, seg.in.x_stride, seg.in.y_stride, seg.out.w, seg.out.h, seg.out.x_stride, seg.out.y_stride,
                     c.cost.total, c.unit_usages, seg_area);

            if (c.cost.total <= best.cost.total)
            {
                PRINTSEG(" ### BEST");
                best = c;
                best.seg = seg;
            }
        }
    }
    PRINTSEG("\n");

    assert(best.cost.total != INT_MAX && "no possible segmentation found");

    seg = best.seg;
    conv_seg_w = seg.out.w;
    conv_seg_h = seg.out.h;
    input_pixels_w = ceil_div(seg.out.w - 1, transpose_conv_stride_x);
    input_pixels_h = ceil_div(seg.out.h - 1, transpose_conv_stride_y);
    PRINTSEG("BEST_SEG: num %2d x %2d, in %2d x %2d stride %2d x %2d, out %2d x %2d, stride %2d x %2d, in_px %2d x %2d\n",
             seg.num.x, seg.num.y, seg.in.w, seg.in.h, seg.in.x_stride, seg.in.y_stride, seg.out.w, seg.out.h, seg.out.x_stride, seg.out.y_stride, input_pixels_w, input_pixels_h);
    PRINTSEG("############## SEGMENTATION END ##############\n");
}

// implementation-view memory layout: derive pointers and sizes from strides
void Conv2DTranspose::calcOutputMemLayout() {
    // memory strides determined by convolution output -> independent of output padding

    // standard conv layout without output padding
    // computed from out_dim.mm.(x|y) only; does not use (already modified) out_dim.(x|y)
    Conv2D::calcOutputMemLayout();

    // algorithm-view output: crop top and left pixels by moving start addresses of channels.
    // Downstream layers only access data via out_dim.channel_base, never via out_dim.base
    // Right and bottom garbage/padding already cropped by subtracting padding from out_dim.(x|y)
    unsigned int n_removed = out_padding.top * out_dim.mm.x + out_padding.left;
    for (int oc = 0; oc < out_dim.ch; oc++) {
        out_dim.mm.channel_base[oc] += 2*n_removed;
    }
}

} // namespace CNN_LAYER
