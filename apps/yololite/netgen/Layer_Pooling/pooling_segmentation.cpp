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
#include "pooling_layer.h"
#include "Base/segment.h"
#include "vpro_globals.h"
#include "vpro_cmd_defs.h"

namespace CNN_LAYER {

// new version: rate (all) segmentations supported by HW using a cost function

#define PRINT_ALL_CAND 0
#define PRINT_BEST_CAND 1

void AveragePooling2D::setSegmentDimensions() {

    // output is stored in RF
    int rf_free_entries = RF_DISCARD_ADDR;

    // inputs stored in local memory; halved for double buffering
    int lm_free_entries = VPRO_CFG::LM_SIZE / 4;

    struct {
	segDim seg; // c.seg is unused (stored directly in Layer::seg) -> available to other member functions inside loop
      
        // per usage, one unit computes seg.out.w*seg.out.h pixels for up to VPRO_CFG::LANES output channels
        int unit_usages{};
        
        int ccpp{}; // compute cycles per pass
        int passes{}; // number of times the vector lanes are started (i.e. _conv[_add]() is called by runtime)
        int discarded_out_pixels{};
        int used_rf_entries{};
        int used_lm_entries{};
        
        struct {
            int commands{};
            int dma_in{}; // DMA input data from MM into LM
            int compute{};
            int dma_out{}; // DMA output data from LM to MM
            int penalty{};
            int total{}; // sum
        } cost;
    } c, best; // current, lowest cost

    best.cost.total = INT_MAX;

#if PRINT_ALL_CAND || PRINT_BEST_CAND
    std::cout << "segment dimensions for layer " << getFullName() << " " << src_layers[0]->out_dim.algoStr() << " -> " << out_dim.algoStr() << ", kernel " << pool_size[0] << "x" << pool_size[1] << ":\n";
#endif

    // output image size <= input image size (always!)
    // input and output segment sizes are equal <==> kernel size=1 and stride=1

    // chosen pivotal parameter: segment output size (must be the same for all processing units

    // brute force: try all sane output sizes
    // FIXME limitations tightly depend on the runtime implementation in ...

    int max_seg_out_w = std::min({(int)MAX_X_END, rf_free_entries, out_dim.x}); // z_end: HW limitation; rf: height 1; conv_out_dim: segment width > image width makes no sense    
    for (seg.out.w = 1; seg.out.w <= max_seg_out_w; seg.out.w++) {

        // how much input data is accessed to compute the output? load smallest sufficient number of input samples
        // 1st output sample requires kernel_length input samples; each additional output sample requires stride additional input samples
        seg.in.w = 1 * pool_size[0] + (seg.out.w-1) * pool_stride[0];

        // addressing limitation: maximum stride (addr distance between two rows, VPRO command parameter)
        // if seg_in_w grows to large, break loop
        if (seg.in.w > (int)MAX_BETA)
            break;
        
        // pool size must fit into segment
        // skip iterations where seg_in_w is too small for pool_size
        if (seg.in.w % pool_size[0])
            continue;

        // any use in trying something smaller than available mem size?
        // seg.out.h = std::min(conv_out_dim.y, rf_free_entries/seg.out.w);
        // seg.out.h = std::max(1, seg.out.h-10); // debug: also show some smaller heights
        // yes: if a new pass has to be opened for a few rows, smaller heights may be better balanced
        seg.out.h = 1;
        int max_seg_out_h = std::min({(int)MAX_Y_END, out_dim.y}); // z_end: HW limitation; rf: height 1; conv_out_dim: segment width > image width makes no sense    
        for (; seg.out.h <= max_seg_out_h; seg.out.h++) {
            // is the configuration implied by seg.out.h valid?

            // averagepooling2d required map with divisors in rf & lm
            int size_div_map = seg.out.w * seg.out.h + 1; // div_map begins at RF[1] not RF[0] to prevent RAW from first MAC output to RF[0]
            // does one output channel fit into the RF?
            if ((size_div_map) > rf_free_entries) // can be moved into for loop condition
                break; // larger seg.out.h will further increase memory requirement -> break seg.out.h loop
            
            // load smallest sufficient number of input samples
            // 1st output sample requires kernel_length input samples; each additional output sample requires stride additional input samples
            seg.in.h = pool_size[1] + (seg.out.h-1) * pool_stride[1];
            // does one input channel fit into LM?
            if ((seg.in.w * seg.in.h + size_div_map) > lm_free_entries)
                break; // larger seg.out.h will not decrease input memory requirement -> break seg.out.h loop
            
             // skip iteration, if seg_in_h does not cover pool_size
            if (seg.in.h % pool_size[1])
                continue;

            // geometry: number of segments per (input|output) channel
            seg.num.x = ceil_div(out_dim.x, seg.out.w);
            seg.num.y = ceil_div(out_dim.y, seg.out.h);

            seg.in.x_stride = seg.out.w * pool_stride[0];
            seg.in.y_stride = seg.out.h * pool_stride[1];

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
            
            // some possibly useful stuff, WIP
            // lanes can only operate in parallel on the same input data -> idle lanes if number of out channels is not dividable by lanes
            c.unit_usages = seg.num.x * seg.num.y * ceil_div(out_dim.ch, (int)VPRO_CFG::LANES)*VPRO_CFG::LANES;
            c.passes = ceil_div(c.unit_usages, (int)VPRO_CFG::parallel_Lanes);
            c.ccpp = pool_size[0] * pool_size[1] * seg.out.w * seg.out.h;
            c.discarded_out_pixels = c.passes*VPRO_CFG::parallel_Lanes*seg.out.w*seg.out.h - out_dim.x*out_dim.y*out_dim.ch;
            c.used_rf_entries = seg.out.w * seg.out.h + size_div_map;
            c.used_lm_entries = seg.in.w * seg.in.h + size_div_map;

            // compute cost function to determine the best segmentation
            // actual execution time depends on too many factors -> heuristic approach
            c.cost.commands = 0;
            c.cost.dma_in = 0;
            c.cost.compute = 0;
            c.cost.dma_out = 0;
            c.cost.penalty = 0;

            int seg_area;
            seg_area = seg.in.w*seg.in.h; // depends on kernel size -> does not prefer square segments for kernel size 1
            seg_area = seg.out.w*seg.out.h; // only total output area, does not prefer square segments
            seg_area = (seg.out.w+1)*(seg.out.h+1); // penalty for non-square segments with same area; overhead per output area increases with shrinking segment size; +1: arbitrary positive constant
            seg_area = (seg.out.w * pool_stride[0] + 1)*(seg.out.h * pool_stride[1] + 1);
            c.cost.penalty = c.unit_usages * seg_area; // minimizes number of segments -> maximizes (useful?) segment size


            c.cost.total = c.cost.commands + c.cost.dma_in + c.cost.compute + c.cost.dma_out + c.cost.penalty;
            

#if PRINT_ALL_CAND
            printf("out %3d x %3d, in %3d x %3d, num %3d x %3d, passes %4d, ccpp %5d, cycles %8d, dop %6d, LM %4d/%4d, RF %4d/%4d, cost: commands %8d, dma_in %8d, compute %8d, dma_out %6d, penalty %6d, total %8d\n",
                   seg.out.w, seg.out.h,
                   seg.in.w, seg.in.h,
                   seg.num.x, seg.num.y,
                   c.passes, c.ccpp, c.passes*c.ccpp, c.discarded_out_pixels,
                   c.used_lm_entries, lm_free_entries, c.used_rf_entries, rf_free_entries,                   
                   c.cost.commands, c.cost.dma_in, c.cost.compute, c.cost.dma_out, c.cost.penalty, c.cost.total);
#endif
          
            if (c.cost.total < best.cost.total) {
                best = c;
                best.seg = seg;
            }
        }
    }

    seg = best.seg;
    
#if PRINT_BEST_CAND
    c = best;
    printf("out %3d x %3d, in %3d x %3d, num %3d x %3d, passes %4d, ccpp %5d, cycles %8d, dop %6d, LM %4d/%4d, RF %4d/%4d, cost: commands %8d, dma_in %8d, compute %8d, dma_out %6d, penalty %6d, total %8d  <- BEST\n",
           seg.out.w, seg.out.h,
           seg.in.w, seg.in.h,
           seg.num.x, seg.num.y,
           c.passes, c.ccpp, c.passes*c.ccpp, c.discarded_out_pixels,
           c.used_lm_entries, lm_free_entries, c.used_rf_entries, rf_free_entries,                   
           c.cost.commands, c.cost.dma_in, c.cost.compute, c.cost.dma_out, c.cost.penalty, c.cost.total);
#endif

    generateWeights();
}
    
void AveragePooling2D::generateWeights(){
    
    // with valid padding the divisor is constant 
    if(pool_padding_mode == PADDING_MODE::VALID) {
        int pool_area = pool_size[0] * pool_size[1];        // divisor for average calculation
        // float div = 1 / (float)pool_area;                   // replace division by multiplication with reciprocal
        // int div_frac_bits = 14;                             // FIXME: Replace hardcoded value / fpf applies to all divisors
        //int div_int = (int) div * pow(2, div_frac_bits);    // scale factor and cast to int
        
        pool_avg_shiftr = 14;
        float div = 1 / (float)pool_area;
        int div_int = (int)(div * pow(2, pool_avg_shiftr));    
        
        
        for(int x = 0; x < out_dim.x; x++) {
            for(int y = 0; y < out_dim.y; y++) { 
                weights_packed.push_back(div_int);
            }
        }
    }

    // REMOVE
    // std::cout << "padding.algo.left " << padding.algo.left << std::endl;
    // std::cout << "padding.algo.top " << padding.algo.top << std::endl;
    // std::cout << "padding.algo.right " << padding.algo.right << std::endl;
    // std::cout << "padding.algo.bottom " << padding.algo.bottom << std::endl;
    // std::cout << " out_dim.x " << out_dim.x << std::endl;
    // std::cout << " out_dim.y " << out_dim.y << std::endl;
    // std::cout << " pool_size[0] " << pool_size[0] << std::endl;
    // std::cout << " pool_size[1] " << pool_size[1] << std::endl;    
    // std::cout << " pool_stride[0] " << pool_stride[0] << std::endl;
    // std::cout << " pool_stride[1] " << pool_stride[1] << std::endl;

    // FIXMEEEEEEEEEEEEEEE
    if(pool_padding_mode == PADDING_MODE::SAME) {
        // compute x of first pixel of interest
        int kleft = (pool_size[0] - 1) / 2; // left expansion of pool/kernel
        int x_start = kleft - padding.algo.left;
        int kright = (pool_size[0] / 2);
        // compute y of first pixel of interest
        int ktop = (pool_size[1] - 1) / 2; // top expansion of pool/kernel
        int y_start = ktop - padding.algo.top;
        int kbottom = (pool_size[1] / 2);

        for(int y = y_start; y < in_dim(0).y; y+=pool_stride[1]) {
            for(int x = x_start; x < in_dim(0).x; x+=pool_stride[0]) {
                int valid_pixels = pool_size[0] * pool_size[1];
                for(int kx = -kleft; kx <= kright; kx++)
                    for(int ky = -ktop; ky <= kbottom; ky++){
                        int u = x + kx;
                        int v = y + ky;

                        if(u < 0 || u >= in_dim(0).x || v < 0 || v >= in_dim(0).y){
                            valid_pixels--;
                        }

                        // REMOVE
                        // std::cout << " x " << x;
                        // std::cout << " y " << y;
                        // std::cout << " kx " << kx;
                        // std::cout << " ky " << ky;
                        // std::cout << " u " << u;
                        // std::cout << " v " << v;
                        // std::cout << " valid_pixels " << valid_pixels << std::endl;

                    }
                    
                // most bottom right value in div map is always 1
                // with 16bit lm the fixed point format can be hardcoded to 2.14
                // TODO: assert limit accumulation terms to avoid overflow
                pool_avg_shiftr = 14;
                float div = 1 / (float)valid_pixels;
                int div_int = (int)(div * pow(2, pool_avg_shiftr));    
                weights_packed.push_back(div_int);
                
                // REMOVE
                // std::cout << " x " << x;
                // std::cout << " y " << y;
                // std::cout << " x-padding.algo.left " << x - padding.algo.left;
                // std::cout << " y-padding.algo.top " << y - padding.algo.top;
                // std::cout << " x+padding.algo.right " << x + padding.algo.right;
                // std::cout << " y+padding.algo.bottom " << y + padding.algo.bottom;
                // std::cout << " valid_pixels " << valid_pixels << std::endl;
            }
        }


        if(0) {
            std::cout << "weights_packed.size() " << weights_packed.size() << std::endl;
            for(int y = 0; y < out_dim.y; y++) {
                for(int x = 0; x < out_dim.x; x++) {
                    std::cout << weights_packed.at(x + y * out_dim.x) << " ";
                }
                std::cout << std::endl;
            }
        }

    }

    if(1) {
        std::cout << std::endl;
        std::cout << std::endl;
        std::cout << std::endl;

        std::cout << "weights_packed.size() " << weights_packed.size() << std::endl;
        for(int x = 0; x < (int)weights_packed.size(); x++) {
            std::cout << weights_packed[x] << " ";
        }
        std::cout << std::endl;
        std::cout << std::endl;
        std::cout << std::endl;
    }                
    weights_loaded = true;
}

} // namespace CNN_LAYER  
