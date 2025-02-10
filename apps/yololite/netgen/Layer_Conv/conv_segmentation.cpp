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
#include <sys/stat.h> // mkdir
#include <set>
#include "conv_layer.h"
#include "Base/segment.h"
#include "vpro_globals.h"
#include "vpro_cmd_defs.h"
#include "ConvSegmentationHeuristic/MaxEfficiencyCalculation.h"

namespace CNN_LAYER
{

#define LEGACY_SEGMENTATION 0
#if LEGACY_SEGMENTATION
    // kind of closed-form forward solution
    // only kernel sizes 1 and 3 supported
    // badly maintainable hacks
    // kept here for reference until the new version is sufficiently tested and stable

    void Conv2D::setSegmentDimensions() {
        // Previously in cnn_struct.h, constructur of CONV

        // 1024 RF entries -> 32 for dim of out seg
        int n_weights = kernel_length * kernel_length + int(use_bias);
        int rf_free_entries = RF_DISCARD_ADDR - n_weights;

        // local memory is halved for double buffering
        int lm_free_entries = VPRO_CFG::LM_SIZE / 2 - 2 * n_weights;
        int lm_in_seg_max = int(floor(sqrt(lm_free_entries)));

        int max_beta = 31;
        int max_xend_yend = 31; //FIXME: use globals

        //  VPRO can 2D address max 31 * 31 sections (beta limited to 5 bit - next line address), so dim is max 31
        lm_in_seg_max = std::min(max_beta, int(ceil(float(lm_in_seg_max) / stride)));

        int rf_out_seg_max = std::min(lm_in_seg_max, int(floor(sqrt(rf_free_entries))));

        // Segment
        seg.num.x = std::max(ceil(float(conv_out_dim.x) / float(rf_out_seg_max)),
                             ceil(float(in_dim(0).x /*+ padding.left + padding.right?*/) / float(lm_in_seg_max)));
        seg.num.y = std::max(ceil(float(conv_out_dim.y) / float(rf_out_seg_max)),
                             ceil(float(in_dim(0).y /*+ padding.top + padding.bottom?*/) / float(lm_in_seg_max)));

        seg.out.w = ceil(float(conv_out_dim.x) / float(seg.num.x));
        seg.out.h = ceil(float(conv_out_dim.y) / float(seg.num.y));

        // limit to segment addressing by x_end, y_end
        int max_seg_dim = max_xend_yend + 1;   // for all segments...
        if (seg.out.w > max_seg_dim) {
            seg.num.x = ceil(float(conv_out_dim.x) / float(max_seg_dim));
            seg.out.w = ceil(float(conv_out_dim.x) / float(seg.num.x));
        }
        if (seg.out.h > max_seg_dim) {
            seg.num.y = ceil(float(conv_out_dim.y) / float(max_seg_dim));
            seg.out.h = ceil(float(conv_out_dim.y) / float(seg.num.y));
        }

        seg.in.w = ((kernel_length - 1) / stride) + seg.out.w * stride;
        seg.in.h = ((kernel_length - 1) / stride) + seg.out.h * stride;
        if (seg.in.w > max_beta){
            seg.num.x++;
            seg.out.w = ceil(float(conv_out_dim.x)/float(seg.num.x));
            seg.in.w = ((kernel_length - 1) / stride) + seg.out.w * stride;
        }
        if (seg.in.h > max_beta){
            seg.num.y++;
            seg.out.h = ceil(float(conv_out_dim.y)/float(seg.num.y));
            seg.in.h = ((kernel_length - 1) / stride) + seg.out.h * stride;
        }

        // if pooling is used, segment dimensions have to be adjusted
        if (pool_size[0] > 1) {
            if (seg.out.w % 2 != 0) { // has to be divideable by two
                seg.out.w--;
                seg.in.w = ((kernel_length - 1) / stride) + seg.out.w * stride;
                seg.num.x = ceil(float(conv_out_dim.x) / float(seg.out.w));
            }
            if (seg.out.h % 2 != 0) { // has to be divideable by two
                seg.out.h--;
                seg.in.h = ((kernel_length - 1) / stride) + seg.out.h * stride;
                seg.num.y = ceil(float(conv_out_dim.y) / float(seg.out.h));
            }
        }

        seg.in.x_stride = seg.out.w * conv_stride_x;
        seg.in.y_stride = seg.out.h * conv_stride_y;

        seg.out.x_stride = seg.out.w / pool_size[0];
        seg.out.y_stride = seg.out.h / pool_size[0];
    }

#else
    // new version: rate (all) segmentations supported by HW using a cost function

#define PRINT_ALL_CAND 0
#define PRINT_BEST_CAND 0
#define PRINT_DBG(...)
//#define PRINT_DBG(...) do { printf(__VA_ARGS__); } while (0)

//    void Conv2D::calcOutputMemLayout() {
//        Conv::calcOutputMemLayout();
//
//        if (kernel_length == 1 && pool_size == 1 && stride == 1 && groups == 1 && parallel_outchannels_per_lane > 1 && upsampling_scale == 1) {
//            // if segment size includes some overcalc pixels...
////            out_dim.mm.x = out_dim.x;
////            out_dim.mm.y = out_dim.y;
//            out_dim.mm.ch_size = out_dim.x * out_dim.y * 2;
//        }
//    }
    void Conv2D::setSegmentDimensions() {

        // closed-form forward solution for optimum segment size is too hard -> rate different segment sizes based on cost function
        // goal: minimize number of cycles required for overall computation (including DMA, VPRO, ...)

        // segment refers to the same region of input and output images
        // both lanes of a unit compute different output channels from the same input data (chaining, L/S-lane) using different kernels
        // input DMA is efficient if the data can be broadcast to all segments that require it

        // be prepared for separate x and y parameters
        int kernel_length_x = kernel_length;
        int kernel_length_y = kernel_length;
        int conv_stride_x = stride; // not to be confused with memory layout x_stride (gap at line end)
        int conv_stride_y = stride;

        // all sizes in elements=words, not bytes
        int n_weights = kernel_length_x * kernel_length_y + int(use_bias);

        // output is stored in RF
        int rf_free_entries = RF_DISCARD_ADDR - n_weights;
        if (activation == RELU6) {
          rf_free_entries -= 1; // one entry required for shifted six
        }

        // inputs stored in local memory; halved for double buffering
        int lm_free_entries = VPRO_CFG::LM_SIZE / 4 - VPRO_CFG::LANES * n_weights; // each lane computes one output channel

        if (layercfg.segmentation_strategy == DETAILED_HEURISTIC && kernel_length == 1 && pool_size[0] == 1 && stride == 1 && groups == 1 && upsampling_scale == 1 && pre_zp.top == 0 && pre_zp.right == 0 && pre_zp.bottom == 0 && pre_zp.left == 0) {
            // if defined [manual] by layer, use this parametrization
            if (outchannel_parallelism > 0) {    // TODO setting in layer
                seg.num.x = MaxEfficiencyCalculation::getBlockCount(outchannel_block_size, in_dim(0).mm.x * in_dim(0).y);
                seg.num.y = 1;
                seg.in.w = outchannel_block_size;
                seg.in.h = 1;
                seg.in.x_stride = seg.in.w;
                seg.in.y_stride = 0; // unneeded, as 1D -> y is always 1!

                seg.out.w = outchannel_block_size;
                seg.out.h = 1;
                seg.out.x_stride = seg.out.w;
                seg.out.y_stride = 0; // unneeded, as 1D -> y is always 1!

                parallel_outchannels_per_lane = outchannel_parallelism;
                parallel_inchannels_per_lane = 1;

                overcalc_elements_1d =  MaxEfficiencyCalculation::getBlockOverlap(outchannel_block_size, seg.num.x, in_dim(0).mm.x * in_dim(0).y);
            }  else if ((VPRO_CFG::UNITS == 8 && VPRO_CFG::CLUSTERS == 8) &&
                        ((in_dim(0).mm.x == 112 && in_dim(0).mm.y == 112 && in_dim(0).ch == 16 && out_dim.ch == 96) ||
                         (in_dim(0).mm.x == 112 && in_dim(0).mm.y == 112 && in_dim(0).ch == 32 && out_dim.ch == 16) ||
                         (in_dim(0).mm.x == 56 && in_dim(0).mm.y == 56 && in_dim(0).ch == 144 && out_dim.ch == 24) ||
                         (in_dim(0).mm.x == 56 && in_dim(0).mm.y == 56 && in_dim(0).ch == 24 && out_dim.ch == 144) ||
                         (in_dim(0).mm.x == 56 && in_dim(0).mm.y == 56 && in_dim(0).ch == 96 && out_dim.ch == 24) ||
                         (in_dim(0).mm.x == 28 && in_dim(0).mm.y == 28 && in_dim(0).ch == 144 && out_dim.ch == 32) ||
                         (in_dim(0).mm.x == 28 && in_dim(0).mm.y == 28 && in_dim(0).ch == 192 && out_dim.ch == 32) ||
                         (in_dim(0).mm.x == 28 && in_dim(0).mm.y == 28 && in_dim(0).ch == 32 && out_dim.ch == 192) ||
                         (in_dim(0).mm.x == 14 && in_dim(0).mm.y == 14 && in_dim(0).ch == 192 && out_dim.ch == 64) ||
                         (in_dim(0).mm.x == 14 && in_dim(0).mm.y == 14 && in_dim(0).ch == 384 && out_dim.ch == 64) ||
                         (in_dim(0).mm.x == 14 && in_dim(0).mm.y == 14 && in_dim(0).ch == 576 && out_dim.ch == 96) ||
                         (in_dim(0).mm.x == 14 && in_dim(0).mm.y == 14 && in_dim(0).ch == 64 && out_dim.ch == 384)  ||
                         (in_dim(0).mm.x == 14 && in_dim(0).mm.y == 14 && in_dim(0).ch == 96 && out_dim.ch == 576) ||
                         (in_dim(0).mm.x == 7 && in_dim(0).mm.y == 7 && in_dim(0).ch == 160 && out_dim.ch == 960) ||
                         (in_dim(0).mm.x == 7 && in_dim(0).mm.y == 7 && in_dim(0).ch == 320 && out_dim.ch == 1280)  ||
                         (in_dim(0).mm.x == 7 && in_dim(0).mm.y == 7 && in_dim(0).ch == 576 && out_dim.ch == 160)  ||
                         (in_dim(0).mm.x == 7 && in_dim(0).mm.y == 7 && in_dim(0).ch == 960 && out_dim.ch == 160) ||
                         (in_dim(0).mm.x == 7 && in_dim(0).mm.y == 7 && in_dim(0).ch == 960 && out_dim.ch == 320))
                        ) {    // use mobilenetv2 evluation results (brute force runs on FPGA)

                uint32_t outchannel_block_size_w;
                uint32_t outchannel_block_size_h{1};

                if (in_dim(0).mm.x == 112 && in_dim(0).mm.y == 112 && in_dim(0).ch == 16 && out_dim.ch == 96){
                    outchannel_block_size_w = 40;
                    outchannel_block_size_h = 2;
                    outchannel_parallelism = 12;
//                  outchannel_block_size_w = 60;   // this is best with h == 1
//                  outchannel_parallelism = 16;
                } else if (in_dim(0).mm.x == 112 && in_dim(0).mm.y == 112 && in_dim(0).ch == 32 && out_dim.ch == 16){
                    outchannel_block_size_w = 56;
                    outchannel_block_size_h = 2;
                    outchannel_parallelism = 8;
//                  outchannel_block_size_w = 14;    // this is best with h == 1
//                  outchannel_parallelism = 1;
                } else if (in_dim(0).mm.x == 56 && in_dim(0).mm.y == 56 && in_dim(0).ch == 144 && out_dim.ch == 24){
                  outchannel_block_size_w = 52;
                  outchannel_parallelism = 12;
                } else if (in_dim(0).mm.x == 56 && in_dim(0).mm.y == 56 && in_dim(0).ch == 24 && out_dim.ch == 144){
                  outchannel_block_size_w = 49;
                  outchannel_parallelism = 18;
                } else if (in_dim(0).mm.x == 56 && in_dim(0).mm.y == 56 && in_dim(0).ch == 96 && out_dim.ch == 24){
                  outchannel_block_size_w = 52;
                  outchannel_parallelism = 12;
                } else if (in_dim(0).mm.x == 28 && in_dim(0).mm.y == 28 && in_dim(0).ch == 144 && out_dim.ch == 32){
                  outchannel_block_size_w = 14;
                  outchannel_parallelism = 16;
                } else if (in_dim(0).mm.x == 28 && in_dim(0).mm.y == 28 && in_dim(0).ch == 192 && out_dim.ch == 32){
                  outchannel_block_size_w = 14;
                  outchannel_parallelism = 16;
                } else if (in_dim(0).mm.x == 28 && in_dim(0).mm.y == 28 && in_dim(0).ch == 32 && out_dim.ch == 192){
                    outchannel_block_size_w = 49;
                    outchannel_block_size_h = 196/49;
                    outchannel_parallelism = 4;
//                  outchannel_block_size_w = 20;   // this is best with h == 1
//                  outchannel_parallelism = 1;
                } else if (in_dim(0).mm.x == 14 && in_dim(0).mm.y == 14 && in_dim(0).ch == 192 && out_dim.ch == 64){
                  outchannel_block_size_w = 52;
                  outchannel_block_size_h = 2;
                  outchannel_parallelism = 2;
//                  outchannel_block_size_w = 26;   // this is best with h == 1
//                  outchannel_parallelism = 4;
                } else if (in_dim(0).mm.x == 14 && in_dim(0).mm.y == 14 && in_dim(0).ch == 384 && out_dim.ch == 64){
                    outchannel_block_size_w = 33;
                    outchannel_block_size_h = 3;
                    outchannel_parallelism = 2;
//                  outchannel_block_size_w = 26;   // this is best with h == 1
//                  outchannel_parallelism = 4;
                } else if (in_dim(0).mm.x == 14 && in_dim(0).mm.y == 14 && in_dim(0).ch == 576 && out_dim.ch == 96){
                    outchannel_block_size_w = 50;
                    outchannel_block_size_h = 2;
                    outchannel_parallelism = 2;
//                  outchannel_block_size_w = 52;   // this is best with h == 1
//                  outchannel_parallelism = 3;
                } else if (in_dim(0).mm.x == 14 && in_dim(0).mm.y == 14 && in_dim(0).ch == 64 && out_dim.ch == 384){
                    outchannel_block_size_w = 44;
                    outchannel_block_size_h = 132/44;
                    outchannel_parallelism = 7;
//                  outchannel_block_size_w = 49;   // this is best with h == 1
//                  outchannel_parallelism = 12;
                } else if (in_dim(0).mm.x == 14 && in_dim(0).mm.y == 14 && in_dim(0).ch == 96 && out_dim.ch == 576){
                    outchannel_block_size_w = 49;
                    outchannel_block_size_h = 196/49;
                    outchannel_parallelism = 5;
//                  outchannel_block_size_w = 49;   // this is best with h == 1
//                  outchannel_parallelism = 18;
                } else if (in_dim(0).mm.x == 7 && in_dim(0).mm.y == 7 && in_dim(0).ch == 160 && out_dim.ch == 960){
                  outchannel_block_size_w = 25;
                  outchannel_parallelism = 15;
                } else if (in_dim(0).mm.x == 7 && in_dim(0).mm.y == 7 && in_dim(0).ch == 320 && out_dim.ch == 1280){
                  outchannel_block_size_w = 25;
                  outchannel_parallelism = 20;
                } else if (in_dim(0).mm.x == 7 && in_dim(0).mm.y == 7 && in_dim(0).ch == 576 && out_dim.ch == 160){
                  outchannel_block_size_w = 25;
                  outchannel_parallelism = 4;
                } else if (in_dim(0).mm.x == 7 && in_dim(0).mm.y == 7 && in_dim(0).ch == 960 && out_dim.ch == 160){
                  outchannel_block_size_w = 25;
                  outchannel_parallelism = 4;
                } else if (in_dim(0).mm.x == 7 && in_dim(0).mm.y == 7 && in_dim(0).ch == 960 && out_dim.ch == 320){
                  outchannel_block_size_w = 25;
                  outchannel_parallelism = 5;
                } else {
                  printf("ERROR! mobilenetv2 evluation results (brute force runs on FPGA) no result!!!\n");
                  exit(1);
                }
                printf("\e[92mUsing 8C8U brute force result (mobilenetv2 eval) for Layer: Input: %i x %i x %i, Outchannels: %i [block size: %i, parallel: %i]\e[0m\n",
                       in_dim(0).mm.x, in_dim(0).mm.y, in_dim(0).ch, out_dim.ch, outchannel_block_size_w, outchannel_parallelism);

                outchannel_block_size = outchannel_block_size_w * outchannel_block_size_h;
                seg.num.x = MaxEfficiencyCalculation::getBlockCount(outchannel_block_size, in_dim(0).mm.x * in_dim(0).y);
                seg.num.y = 1;
                seg.in.w = outchannel_block_size_w;
                seg.in.h = outchannel_block_size_h;
                seg.in.x_stride = seg.in.w * seg.in.h;
                seg.in.y_stride = seg.in.w * seg.in.h; // unneeded, as 1D!

                seg.out.w = outchannel_block_size_w;
                seg.out.h = outchannel_block_size_h;
                seg.out.x_stride = seg.out.w * seg.in.h;
                seg.out.y_stride = seg.out.w * seg.out.h; // unneeded, as 1D!

                parallel_outchannels_per_lane = outchannel_parallelism;
                parallel_inchannels_per_lane = 1;

                overcalc_elements_1d = (seg.out.w * seg.num.x * seg.out.h * seg.num.y) - (out_dim.x * out_dim.y);
                assert(overcalc_elements_1d >= 0);
            } else {
                // check for a cache version
                char cache_fname[1024];
                sprintf(cache_fname, "../../cache/conv2d1x1_segmentation_%dc%du%dl_%ldx%dx%d_%d.bin",
                        VPRO_CFG::CLUSTERS, VPRO_CFG::UNITS, VPRO_CFG::LANES,
                        in_dim(0).mm.x, in_dim(0).y, in_dim(0).ch, out_dim.ch);
                std::ifstream is(cache_fname, std::ofstream::binary | std::ofstream::in);
                if (is) {
#define READ_BIN(DATA) is.read(reinterpret_cast<char *>(&DATA), sizeof(DATA))
                    READ_BIN(seg);
                    READ_BIN(parallel_outchannels_per_lane);
                    READ_BIN(parallel_inchannels_per_lane);
                    READ_BIN(overcalc_elements_1d);
                    is.close();
                } else {
                    // evaluate heuristic for most efficient version
                    setvbuf(stdout, NULL, _IONBF, 0);
                    auto eval = MaxEfficiencyCalculation(in_dim(0).mm.x, in_dim(0).y, in_dim(0).ch, out_dim.ch, true);
                    eval.runCalculation();

                    seg.num.x = eval.getBlockCount();
                    seg.num.y = 1;
                    seg.in.w = eval.get1DInputSize();
                    seg.in.h = 1;
                    seg.in.x_stride = seg.in.w;
                    seg.in.y_stride = 0; // unneeded, as 1D -> y is always 1!

                    seg.out.w = eval.get1DInputSize();
                    seg.out.h = 1;
                    seg.out.x_stride = seg.out.w;
                    seg.out.y_stride = 0; // unneeded, as 1D -> y is always 1!

                    parallel_outchannels_per_lane = eval.getParallelOutChannelsPerLane();
                    parallel_inchannels_per_lane = eval.getParallelInChannelsPerLane();

                    overcalc_elements_1d = eval.getOvercalc();

                    // save version to cache
                    /*int status = */mkdir("../../cache", 0777);//S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                    std::ofstream fd(cache_fname, std::ofstream::binary | std::ofstream::out);
                    if (!fd) {
                        std::cout << "Could not open cache file '" << cache_fname << "' for writing\n";
                    }
#define WRITE_BIN(DATA) fd.write(reinterpret_cast<char *>(&DATA), sizeof(DATA))
                    WRITE_BIN(seg);
                    WRITE_BIN(parallel_outchannels_per_lane);
                    WRITE_BIN(parallel_inchannels_per_lane);
                    WRITE_BIN(overcalc_elements_1d);
                    fd.close();
                }
            }
            
            if (parallel_outchannels_per_lane > 1) {
                layercfg.scheduling_order = ITERATE_ALL_SORTED_X2;  // TODO: eval, which order is best
                conv_seg_w = seg.out.w; // no pool, upsample
                conv_seg_h = seg.out.h; // no pool, upsample
                padding.enabled = false; // padding only works for 2D input
                return;
            } else {
                parallel_outchannels_per_lane = 1; // use old way... continue to define block size, etc...
            }
        }

        struct {
            segDim seg; // c.seg is unused (stored directly in Layer::seg) -> available to other member functions inside loop

            // per usage, one unit computes seg.out.w*seg.out.h pixels for up to VPRO_CFG::LANES output channels
            int unit_usages{};

            int conv_seg_w{};
            int conv_seg_h{};

            double effective_unit_usage{};
            double effective_squariness{};
            double effective_seg_area{};
            double effective_pixel_calc_factor{};

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
                double penalty{};
                double total{}; // sum
            } cost;
        } c, best; // current, lowest cost

        best.cost.total = INT_MAX;

#if PRINT_ALL_CAND || PRINT_BEST_CAND
        std::cout << "segment dimensions for layer " << getFullName() << " " << src_layers[0]->out_dim.algoStr() << " -> " << out_dim.algoStr() << ", kernel " << kernel_length_x << "x" << kernel_length_y << ":\n";
#endif

        int segmentation_search_count = 0;
        // output image size <= input image size (always!)
        // input and output segment sizes are equal <==> kernel size=1 and stride=1
        // pooling: seg.out.(w|h) is post-conv and pre-pool

        // chosen pivotal parameter: segment output size (must be the same for all processing units)

        // brute force: try all sane output sizes
        // limitations tightly depend on the runtime implementation in _conv(), _conv_add(), _maxpool2x2(), _act_*(), _shift_store()
        bool kernel1x1 = kernel_length_x == 1; // separate implementation in _conv[_add]()
        //      int max_seg_out_w = std::min({(int)MAX_Z_END-1, rf_free_entries, conv_out_dim.x}); // z_end: HW limitation in _conv[_add](); rf: height 1; conv_out_dim: segment width > image width makes no sense
        int max_seg_out_w = std::min(rf_free_entries, conv_out_dim.x); // rf: height 1; conv_out_dim: segment width > image width makes no sense
        int max_seg_out_h = std::min(rf_free_entries, conv_out_dim.y); // rf: width 1

        // limitations in _conv[_add]()
        if (kernel1x1) {
            max_seg_out_w = std::min({max_seg_out_w, (int)MAX_X_END + 1, (int)MAX_BETA}); // x_end: loads + mach; beta: DST_ADDR
            max_seg_out_h = std::min(max_seg_out_h, (int)MAX_Y_END + 1);
        } else {
            max_seg_out_w = std::min(max_seg_out_w, (int)MAX_Z_END + 1); // z_end: HW limitation in _conv[_add]()
        }

        // depending on activation, pooling and pool_after_activation, the conv result is either processed by
        // _maxpool2x2(), _act_*() or _shift_store. All of them use seg.out.w - 1 as x_end and seg_out.w as beta
        max_seg_out_w = std::min({max_seg_out_w, (int)MAX_X_END + 1, (int)MAX_BETA});
        // All of them use seg.out_h - 1 as y_end.
        max_seg_out_h = std::min(max_seg_out_h, (int)MAX_Y_END + 1);

        PRINT_DBG("max_seg_out_w %d, max_seg_out_h %d\n", max_seg_out_w, max_seg_out_h);
        for (conv_seg_w = 1; conv_seg_w <= max_seg_out_w; conv_seg_w++) {
            PRINT_DBG("conv_seg_w %d\n", conv_seg_w);

            // how much input data is accessed to compute the output? load smallest sufficient number of input samples
            // 1st output sample requires kernel_length input samples; each additional output sample requires stride additional input samples
            seg.in.w = dilated_kernel_w + (conv_seg_w - 1) * conv_stride_x;
            
            // addressing limitation: maximum stride (addr distance between two rows, VPRO command parameter)
            if (( kernel1x1 && conv_stride_x*seg.in.w > (int)MAX_BETA) ||
                (!kernel1x1 && dilation_rate[0]*seg.in.w > (int)MAX_BETA)) {
                PRINT_DBG("beta\n");
                break;
            }

            // dirty hack: special case for MaxPool2D (child of Conv2D)
            if (getLayerType() == LAYERTYPE::MAXPOOL2D && conv_stride_x*seg.in.w > (int)MAX_BETA) {
                PRINT_DBG("MaxPool2D beta\n");
                break;
            }
            
            if (upsampling_scale != 1)  // _shift_store_upsample(): iterating in vpro command parameter over multiple lines (beta)
                // upsampling uses 4 x (xend+1) (RF result seg width) in beta! -> limit width here
                if (conv_seg_w / pool_size[0] * 4 > (int)MAX_BETA) {
                    PRINT_DBG("upsampling: conv_seg_w %d too large for beta\n", conv_seg_w);
                    break;
                }

            // _pool() needs even input width (pool_stride = pool_size = 2)
            if (conv_seg_w % pool_size[0]) {
                PRINT_DBG("conv_seg_w %% pool_size\n");
                continue;
            }

            // any use in trying something smaller than available mem size?
            // yes: if a new pass has to be opened for a few rows, smaller heights may be better balanced
            for (conv_seg_h = 1; conv_seg_h <= max_seg_out_h; conv_seg_h++) {
                PRINT_DBG("conv_seg_h %d\n", conv_seg_h);

                // does one output channel fit into the RF?
                if (conv_seg_w * conv_seg_h > rf_free_entries) { // can be moved into for loop condition
                    PRINT_DBG("RF size reached\n");
                    break; // larger conv_seg_h will further increase memory requirement -> break conv_seg_h loop
                }

                if (upsampling_scale != 1){  // iterating in vpro command parameter over multiple lines (beta)
                    // FIXME see bif.h -> lm_lane_stride (VPRO_CFG::RF_SIZE)
                    // enough space in LM?
                    if ((conv_seg_w / pool_size[0] * upsampling_scale) * (conv_seg_h / pool_size[1] * upsampling_scale) > (int16_t)VPRO_CFG::RF_SIZE) {
                        PRINT_DBG("upsampling: conv_seg_h %d too large for LM\n", conv_seg_h);
                        break;
                    }
                }

                // load smallest sufficient number of input samples
                // 1st output sample requires kernel_length input samples; each additional output sample requires stride additional input samples
                seg.in.h = dilated_kernel_h + (conv_seg_h - 1) * conv_stride_y;
                // does one input channel fit into LM?
                if (seg.in.w * seg.in.h > lm_free_entries) {
                    PRINT_DBG("seg.in too large for LM\n");
                    break; // larger conv_seg_h will not decrease input memory requirement -> break conv_seg_h loop
                }

                // conv2d_kernel.h, _conv[_add](), offset_in: maximum LM offset for input line start
                if ((conv_seg_h - 1) * seg.in.w * conv_stride_y > (int) MAX_OFFSET) {
                    PRINT_DBG("MAX_OFFSET\n");
                    break;
                }

                // _pool() needs even input height (pool_stride = pool_size = 2)
                if (conv_seg_h % pool_size[1]) {
                    PRINT_DBG("conv_seg_h %% pool_size\n");
                    continue;
                }

                // geometry: number of segments per (input|output) channel
                seg.num.x = ceil_div(conv_out_dim.x, conv_seg_w);
                seg.num.y = ceil_div(conv_out_dim.y, conv_seg_h);

                seg.in.x_stride = conv_seg_w * conv_stride_x;
                seg.in.y_stride = conv_seg_h * conv_stride_y;

                if (pool_type == MAX_POOLING) { // requires segment size to be pool-able FIXME: not supported till now: pool for size!=stride merged in conv -> segment size not div-able
                    if (conv_seg_w % pool_size[0] != 0 || conv_seg_h % pool_size[1] != 0){  // not dividable by max pool segment
                        continue;    // set this a bad segmentation
//                        printf("[MAXPOOLING] Bad Seg: %i x %i, (pool: %i x %i)\n", seg.in.w, seg.in.h, pool_stride[0], pool_stride[1]);
                    }
                }

                seg.out.w = conv_seg_w / pool_size[0] * upsampling_scale;
                seg.out.h = conv_seg_h / pool_size[1] * upsampling_scale;

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
                if (seg.in.w < min_seg_in_w) {
                    PRINT_DBG("seg.in.w < min_seg_in_w (padding must fit into one segment)\n");
                    continue;
                }

                int min_seg_in_h;
                if (seg.num.y < 2)
                    min_seg_in_h = padding.dma.top + padding.dma.bottom; // one segment handles both paddings
                else
                    min_seg_in_h = std::max(padding.dma.top, padding.dma.bottom);
                if (seg.in.h < min_seg_in_h) {
                    PRINT_DBG("seg.in.h < min_seg_in_h (padding must fit into one segment)\n");
                    continue;
                }

                // padding widths can only be configured per layer
                // -> all segments have common (t|r|b|l) padding widths or no padding
                // -> padding must be handled by the outermost segments; inner segments must be padding-free
                if (padding.dma.top > seg.in.y_stride ||
                    padding.dma.right > seg.in.x_stride ||
                    padding.dma.bottom > seg.in.y_stride ||
                    padding.dma.left > seg.in.x_stride) {
                    PRINT_DBG("padding t %d, r %d, b %d, l %d > in.stride x %d, y %d\n", padding.dma.top, padding.dma.right, padding.dma.bottom, padding.dma.left, seg.in.x_stride, seg.in.y_stride);
                    continue;
                }

                // -- here: segment size can be handled by hardware

                // compute cost function to determine the best segmentation
                // actual execution time depends on too many factors -> heuristic approach
                c.cost.commands = 0;
                c.cost.dma_in = 0;
                c.cost.compute = 0;
                c.cost.dma_out = 0;
                c.cost.penalty = 0;

                // some possibly useful stuff, WIP
                // lanes can only operate in parallel on the same input data -> idle lanes if number of out channels is not dividable by lanes
                c.unit_usages = seg.num.x * seg.num.y * ceil_div(conv_out_dim.ch, (int)VPRO_CFG::LANES) * VPRO_CFG::LANES;
                c.passes = ceil_div(c.unit_usages, (int)VPRO_CFG::parallel_Lanes);
                c.ccpp = kernel_length_x * kernel_length_y * conv_seg_w * conv_seg_h;
                c.discarded_out_pixels = (c.passes * VPRO_CFG::parallel_Lanes * conv_seg_w * conv_seg_h) - (conv_out_dim.x * conv_out_dim.y * conv_out_dim.ch);
                c.used_rf_entries = conv_seg_w * conv_seg_h;
                c.used_lm_entries = seg.in.w * seg.in.h;

#if 0 // currently unused ideas, kept for reference
                /*
                c.cost.commands = 100*c.unit_usages; // penalty: prefer small number of segments
                c.cost.dma_in = (10 + in_dim[0].ch * seg.in.w * seg.in.h) * c.unit_usages;
                c.cost.compute = in_dim[0].ch * kernel_length_x * kernel_length_y * seg.out.w * conv_seg_h * c.passes;
                c.cost.dma_out = 0;*/

                int overlap;
                overlap = seg.in.w * seg.in.h - (seg.in.w - (kernel_length_x - 1)) * (seg.in.h - (kernel_length_y - 1)); // input overlap area = area including overlap - core area
                overlap = seg.in.w * seg.in.h; // input area // behaves different for kernel size =1 and >1
                //overlap = (seg.out.w+1)*(conv_seg_h+1);
                //            c.cost.dma_in = in_dim[0].ch * c.unit_usages * overlap; // idea: use overlap pixels only

                //            c.cost.compute = (c.passes*VPRO_CFG::parallel_Lanes - c.unit_usages) * seg.out.w * conv_seg_h; // unused computation slots
                //            c.cost.compute = c.passes*VPRO_CFG::parallel_Lanes*seg.out.w*conv_seg_h*conv_out_dim.ch - conv_out_dim.x*conv_out_dim.y*conv_out_dim.ch; // unused computation slots 'algo1'
                // c.cost.compute = c.passes * c.ccpp; // clock cycles spent computing

                //            c.cost.penalty += c.passes; // prefer small number of passes if everything else is identical
                //            c.cost.penalty -= seg.out.w*conv_seg_h; // prefer large segments (output); at constant output area, skewing increases input area

                int seg_area;
                seg_area = seg.in.w * seg.in.h; // depends on kernel size -> does not prefer square segments for kernel size 1
                seg_area = conv_seg_w * conv_seg_h; // only total output area, does not prefer square segments
                seg_area = (conv_seg_w + 1) * (conv_seg_h + 1); // penalty for non-square segments with same area; overhead per output area increases with shrinking segment size; +1: arbitrary positive constant
#endif                
                //int seg_area = (conv_seg_w * conv_stride_x + 1) * (conv_seg_h * conv_stride_y + 1); // prefer square inputs, not outputs
                //            c.cost.penalty = c.passes * (conv_seg_w+1)*(conv_seg_h+1); // evenly distribute segments to all lanes
                //            c.cost.penalty = c.unit_usages * (conv_seg_w+1)*(conv_seg_h+1); // create

                //            c.cost.penalty = c.passes * seg_area; // distributes segments evenly to all lanes
                // c.cost.penalty = c.unit_usages * seg_area; // minimizes number of segments -> maximizes (useful?) segment size

                /**
                 * Units calculating segments
                 */
                c.unit_usages = seg.num.x * seg.num.y * ceil_div(conv_out_dim.ch, (int)VPRO_CFG::LANES) * VPRO_CFG::LANES;
                int iterations = ceil(float(c.unit_usages) / VPRO_CFG::parallel_Lanes);   // theoretical time of units available
                int executed_units = VPRO_CFG::parallel_Lanes * iterations;        // all units are available for this number of unit usage
                double effective_unit_usage = double(c.unit_usages) / executed_units;   // higher -> more unit are in effective use

                /**
                 * Segment total size
                 */
                double effective_seg_area = double(conv_seg_w * conv_seg_h) / VPRO_CFG::RF_SIZE;   // higher -> more of RF is in use

                /**
                 * Segment Oversize to output channel size
                 */
                // if segmentation is bad. too much pixels are calculated
                double calc_pixels = seg.num.x * seg.out.w * seg.num.y * seg.out.h;
                double req_pixels = out_dim.x * out_dim.y;
                double effective_pixel_calc_factor = req_pixels / calc_pixels;  // higher -> more pixels are in effective calculation required

                /**
                 * Segment's Squareiness (magic influence)
                 * If segments dont follow square, more padding is required -> more DMA
                 */
                double area = conv_seg_w * conv_seg_h;
                double perimeter = conv_seg_w*2+conv_seg_h*2;
                double max_squarienss = pow(double(std::max(conv_seg_h, conv_seg_w)), 2) / (std::max(conv_seg_h, conv_seg_w)*4);
                double squariness = area / perimeter;   // includes factor of size (again -> RF factor already used)
                double effective_squariness = squariness / max_squarienss; // higher -> more square (e.g. 2x2 -> 4/8=0.5, 1x4 -> 4/10=0.4, 8x8 -> 64/32=2)

                /**
                 * Overall
                 *
                 * Notes for missing influences:
                 *  - DMA Broadcast capability
                 *  - DCMA buffering - line size ~ seg width
                 *  - VPRO/DMA cycles ratio (in RF usage -> large blocks have more VPRO/DMA operations)
                 */
                c.cost.penalty = 1. - ( // cost is inverse of efficiency -> when compared to best (later)
                        pow(effective_unit_usage, 2) *
                        pow(effective_squariness, 1) *
                        pow(effective_seg_area, 1.5) *
                        pow(effective_pixel_calc_factor, 2)
                        );  // pow -> relevance of this usage factor

                c.effective_unit_usage  = effective_unit_usage;
                c.effective_squariness = effective_squariness;
                c.effective_seg_area = effective_seg_area;
                c.effective_pixel_calc_factor = effective_pixel_calc_factor;

                segmentation_search_count++;

                //            c.cost.penalty = c.unit_usages; // minimizes number of segments -> maximize (useful?) segment size

                c.cost.total = c.cost.penalty; // + c.cost.commands + c.cost.dma_in + c.cost.compute + c.cost.dma_out;

#if PRINT_ALL_CAND
                printf("conv_out %3d x %3d, out %3d x %3d, in %3d x %3d s %3d x %3d, num %3d x %3d, passes %4d, ccpp %5d, cycles %8d, dop %6d, LM %4d/%4d, RF %4d/%4d, cost: commands %8d, dma_in %8d, compute %8d, dma_out %6d, penalty %6d, total %8d\n",
                       conv_seg_w, conv_seg_h,
                       seg.out.w, seg.out.h,
                       seg.in.w, seg.in.h, seg.in.x_stride, seg.in.y_stride,
                       seg.num.x, seg.num.y,
                       c.passes, c.ccpp, c.passes*c.ccpp, c.discarded_out_pixels,
                       c.used_lm_entries, lm_free_entries, c.used_rf_entries, rf_free_entries,
                       c.cost.commands, c.cost.dma_in, c.cost.compute, c.cost.dma_out, c.cost.penalty, c.cost.total);
#endif

                // temporary patch for yololite until pooling is fixed
                // pooling is fixed, but this clamping slightly improves performance
                if (pool_size[0] > 1 && conv_seg_w == 28 && conv_seg_h == 28 && number == 0) {
                    printf("FIXME clamping segmentation to 28x28 for yololite special case; pooling is broken for larger sizes\n");
                    c.cost.total = INT_MIN;
                }

                if (c.cost.total <= best.cost.total) {
                    best = c;
                    best.seg = seg;
                    best.conv_seg_w = conv_seg_w;
                    best.conv_seg_h = conv_seg_h;
                }
            }
        }

        assert(best.cost.total != INT_MAX && "could not find a valid segmentation");
        assert(best.cost.total != 0 && "could not find a valid segmentation");

        seg = best.seg;
        conv_seg_w = best.conv_seg_w;
        conv_seg_h = best.conv_seg_h;

        printf("Best Segmentation [after %i iterations]\n", segmentation_search_count);
        printf("  c.effective_unit_usage:        %f \n", best.effective_unit_usage);
        printf("  c.effective_squariness:        %f \n", best.effective_squariness);
        printf("  c.effective_seg_area:          %f \n", best.effective_seg_area);
        printf("  c.effective_pixel_calc_factor: %f \n", best.effective_pixel_calc_factor);
        printf("  seg.num.x: %d, y: %d \n", best.seg.num.x, best.seg.num.y);
        printf("  seg.out.w: %d, h: %d \n", best.seg.out.w, best.seg.out.h);

        // FIXME seg.out is pre-pooling, it should be post-pooling. Currently worked around in conv_commands

#if PRINT_BEST_CAND
        c = best;
        printf("conv_out %3d x %3d, out %3d x %3d, in %3d x %3d s %3d x %3d, num %3d x %3d, passes %4d, ccpp %5d, cycles %8d, dop %6d, LM %4d/%4d, RF %4d/%4d, cost: commands %8d, dma_in %8d, compute %8d, dma_out %6d, penalty %6d, total %8d  <- BEST\n",
               conv_seg_w, conv_seg_h,
               seg.out.w, seg.out.h,
               seg.in.w, seg.in.h, seg.in.x_stride, seg.in.y_stride,
               seg.num.x, seg.num.y,
               c.passes, c.ccpp, c.passes*c.ccpp, c.discarded_out_pixels,
               c.used_lm_entries, lm_free_entries, c.used_rf_entries, rf_free_entries,
               c.cost.commands, c.cost.dma_in, c.cost.compute, c.cost.dma_out, c.cost.penalty, c.cost.total);
#endif
    }


#endif // LEGACY_SEGMENTATION

    void Conv2D::setOutputMemDimensions() {
        Conv::setOutputMemDimensions();

        if (kernel_length == 1 && pool_size[0] == 1 && stride == 1 && groups == 1 && parallel_outchannels_per_lane > 1 && upsampling_scale == 1) {
            // if segment size includes some overcalc pixels...
            // 1x1 convolution -> equivalent 1d conv is more flexible for segmentation -> garbage right of input image is also contained in output
            assert(out_dim.x == in_dim(0).x && "Assumption about propagation of garbage right of image from input to output failed");
            out_dim.mm.x = in_dim(0).mm.x;
            out_dim.mm.y = out_dim.y;
//            out_dim.mm.ch_size = out_dim.x * out_dim.y * 2;
        }
    }

    void Conv2D::generateSegments() {
        Conv::generateSegments();
    }

}; // namespace CNN_LAYER
