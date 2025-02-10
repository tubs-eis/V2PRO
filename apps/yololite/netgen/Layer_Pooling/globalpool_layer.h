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
#ifndef GLOBALPOOL_LAYER_H
#define GLOBALPOOL_LAYER_H

#include "vpro_globals.h"
#include "vpro_cmd_defs.h"
#include "bif.h"
#include "Base/base_layer.h"

namespace CNN_LAYER {

    class GlobalPool: public Layer {
    // user-supplied parameters
    public:
        qparam_t pre_shift_right{0};
        qparam_t multiplier{0};
        qparam_t store_shift_right{0};

    // methods 
    public:
        virtual void processParams() {
            groups = in_dim(0).ch;
            Layer::processParams();
        }

        virtual void computeOutputDim() {
            out_dim.x = 1;
            out_dim.y = 1;
            out_dim.ch = in_dim(0).ch;
        }
        
        // binary export
        virtual void generateBifLayer(BIF::LAYER &bl) {
            Layer::generateBifLayer(bl);
            //            bl.store_shift_right = store_shift_right;
        }

        virtual std::string getLayerInfoText() {
            std::stringstream ss;
            ss << Layer::getLayerInfoText();
            // FIXME ss << "  kernel " << kernel_length << "x" << kernel_length << ", stride " << stride << ", groups " << groups << "\n";
            return ss.str();
        }

        virtual void setOutputMemDimensions() {
            // segments produce intermediate data in mm; payload output is only 1 element per channel
            out_dim.mm.x = 1;
            out_dim.mm.y = 1;
        }

        virtual void calcOutputMemLayout() {
            Layer::calcOutputMemLayout();

            // reserve additional space for intermediate results in MM
            out_dim.mm.channel_base.resize(2*out_dim.ch);

            // per channel: max. all units used, one lane per unit, 48 bit intermediate result
            int intermediate_ch_size = 3 * 2 * VPRO_CFG::CLUSTERS * VPRO_CFG::UNITS; // in byte
            out_dim.mm.channel_base[out_dim.ch] = out_dim.mm.channel_base[out_dim.ch-1] + out_dim.mm.ch_size;
            for (int oc = out_dim.ch+1; oc < 2*out_dim.ch; oc++) {
                out_dim.mm.channel_base[oc] = out_dim.mm.channel_base[oc-1] + intermediate_ch_size;
            }            
            out_dim.mm.size += out_dim.ch * intermediate_ch_size;
        }

    }; // class GlobalPool


    class GlobalAvgPool2D: public GlobalPool {
    public:

        qparam_t pool_avg_shiftr{0};
    
        virtual std::string getLayerTypeName() { return "GlobalAvgPool2D"; }
        virtual LAYERTYPE getLayerType() { return LAYERTYPE::GLOBALAVGPOOL2D; }

        virtual std::string getLayerInfoText() {  
            std::stringstream ss;
            ss << GlobalPool::getLayerInfoText();
            return ss.str();
        }  

        // is i factorizable into x, y, z with 1<=x<=63, 1<=y<=63, 1<=z<=1023?
        static bool factorize(int i, int &x, int &y, int &z) {
            static bool initialized = false;
            static int max_i;
            static std::vector<bool> factorizable;
            static std::vector<int> x_tab;
            static std::vector<int> y_tab;
            static std::vector<int> z_tab;
    
            if (!initialized) {
                // precompute tables
                max_i = VPRO_CFG::LM_SIZE / 2;
                factorizable = std::vector<bool>(max_i+1, false);
                x_tab = std::vector<int>(max_i+1);
                y_tab = std::vector<int>(max_i+1);
                z_tab = std::vector<int>(max_i+1);
                // conditions see _global_avgpool2d_add()
                for (int x = 0; x <= (int)std::min(MAX_X_END+1, MAX_BETA); x++) {
                    for (int y = 0; y <= (int)MAX_Y_END+1; y++) {
                        for (int z = 0; z <= (int)MAX_Z_END; z++) {
                            if (z > 1 && x*y > (int)MAX_GAMMA)
                                break;
                            int i = x*y*z;
                            if (i > max_i)
                                break; // increasing z will only further increase i
                            factorizable[i] = true;
                            x_tab[i] = x;
                            y_tab[i] = y;
                            z_tab[i] = z;
                        }
                    }
                }
                initialized = true;
            }

            assert(i <= max_i);

            if (!factorizable[i])
                return false;

            x = x_tab[i];
            y = y_tab[i];
            z = z_tab[i];
            return true;
        }
        
        virtual void setSegmentDimensions() {
            // inputs stored in local memory; halved for double buffering
            int lm_free_entries = VPRO_CFG::LM_SIZE / 2;

            // number of pixels in segment has to be factorizable into x, y, z with 1<=x<=63, 1<=y<=63, 1<=z<=1023
            // (3D addressing mode limitation)

            // use some heuristic
        
            // be DMA friendly: make segments as wide as possible

            struct {
                segDim seg;
                int sets = INT_MAX;
                int dmas = INT_MAX;
                int seg_size = INT_MAX;
                int lanes_per_ch = 0;
                int sets_per_ch = 0;
            } best;

            int usable_lanes = VPRO_CFG::CLUSTERS*VPRO_CFG::UNITS; // effective number of lanes that operate in parallel (only L0 used)
            for (seg.in.w = std::min(lm_free_entries, in_dim(0).x+20); seg.in.w; seg.in.w--) {
                seg.num.x = ceil_div(in_dim(0).x, seg.in.w);
                for (seg.in.h = std::min(lm_free_entries/seg.in.w, in_dim(0).y+20); seg.in.h; seg.in.h--) {
                    int x, y, z;
                    int seg_size = seg.in.w * seg.in.h;
                    // allow segment widths and heights > 63 (VPRO limitation): only the total number of elements must match
                    if (factorize(seg_size, x, y, z)) {
                        // printf("seg.in %3dx%3d = %5d factorization: x %2d, y %2d, z %3d\n", seg.in.w, seg.in.h, seg_size, x, y, z);
                        seg.num.y = ceil_div(in_dim(0).y, seg.in.h);
                        int num_segs = seg.num.x * seg.num.y;
                        int sets;
                        if (num_segs <= usable_lanes) { // segments linear mapped to lanes
                            lanes_per_ch = num_segs;
                            sets_per_ch = 1;
                            sets = ceil_div(in_dim(0).ch*num_segs, usable_lanes); // unused lanes only once in the last set
                        } else { // one channel uses all lanes more than once
                            lanes_per_ch = usable_lanes;
                            sets_per_ch = ceil_div(num_segs, usable_lanes); // unused lanes at the end of each channel
                            sets = in_dim(0).ch * sets_per_ch;
                        }

                        // FIXME simplified 3rd strategy: whole channel sequentially mapped to one lane (saves dmas of partial sums)
                        // other strategies not yet fully implemented
                        lanes_per_ch = 1;
                        sets_per_ch = num_segs;
                        sets = sets_per_ch * ceil_div(in_dim(0).ch, usable_lanes);
                        
                        int dmas = in_dim(0).ch*num_segs;
                        // printf("global pooling: segmentation num %3dx%3d, in %3dx%3d, sets %2d, dmas %3d, seg_size %4d\n", seg.num.x, seg.num.y, seg.in.w, seg.in.h, sets, dmas, seg_size);
                        if (sets < best.sets || (sets == best.sets && (dmas < best.dmas || (dmas == best.dmas && seg_size < best.seg_size)))) {
                            //                        printf("=== global pooling: new best segmentation num %3dx%3d, in %3dx%3d, sets %2d, dmas %3d, seg_size %4d\n",
                            //                               seg.num.x, seg.num.y, seg.in.w, seg.in.h,
                            //                               sets, dmas, seg_size);
                            best.seg = seg;
                            best.sets = sets;
                            best.dmas = dmas;
                            best.seg_size = seg_size;
                            best.lanes_per_ch = lanes_per_ch;
                            best.sets_per_ch = sets_per_ch;
                        }
                        //                    break;
                    }
                }
            }

            assert(best.sets != INT_MAX && "no valid segmentation found");
            
            seg = best.seg;
            lanes_per_ch = best.lanes_per_ch;
            sets_per_ch = best.sets_per_ch;

            if (lanes_per_ch == 1) {
                seg.out.w = 1; // 16 bit end result, no intermediate written to LM
            } else {
                seg.out.w = 3; // 48 bit intermediate result
            }
            seg.out.h = 1;
            seg.out.x_stride = 0;
            seg.out.y_stride = 0;

            // printf("GlobalAvgPool2D::setSegmentDimensions(): num %dx%d, in %dx%d, out %dx%d, lanes_per_ch %d, sets_per_ch %d, sets %d\n",
            // seg.num.x, seg.num.y, seg.in.w, seg.in.h, seg.out.w, seg.out.h, lanes_per_ch, sets_per_ch, best.sets);
        }
            
        virtual void generateSegments() {
            // Global pooling concept
            // - all image pixels must be added together
            // - input feature map is segmented as usual
            // - 
            // - each lane accumulates arbitrary image regions (location in input feature map is irrelevant)
            // - 
            // Mapsegments compute inter


    
            // Split segement generation into two cases
            // When all segments for one output channel fit into number of parallel lanes then every segment is first and last
            // When segments of one output channel do not fit then set with rest of out channel segments needs to be filled with dummy segments

            int usable_lanes = VPRO_CFG::CLUSTERS*VPRO_CFG::UNITS; // effective number of lanes that operate in parallel (only L0 used)
            int sets = sets_per_ch * ceil_div(in_dim(0).ch, usable_lanes);

            SEGMENT *dummy = new DUMMY_SEGMENT();
            segments = std::vector<SEGMENT *>(sets * VPRO_CFG::parallel_Lanes, dummy); // all point to the same dummy
#define IDX(SET, LANE) ((SET)*VPRO_CFG::parallel_Lanes + (LANE))

            // printf("GlobalAvgPool2D::generateSegments(): lanes_per_ch %d, sets_per_ch %d, sets %d, segments.size() %d\n",
            // lanes_per_ch, sets_per_ch, sets, (int)segments.size());

            for (int out_ch = 0; out_ch < out_dim.ch; ++out_ch) {
                int base_set = (out_ch / usable_lanes) * sets_per_ch;
                for (int y = 0; y < seg.num.y; ++y) {
                    for (int x = 0; x < seg.num.x; ++x) {
                        SEGMENT *s = getSegment(x, y, out_ch, out_ch);
                        s->isFirst = x == 0 && y == 0;
                        s->isLast = (x+1) == seg.num.x && (y+1) == seg.num.y;
                        segments[IDX(base_set + y*seg.num.x + x, (out_ch % usable_lanes) * VPRO_CFG::LANES)] = s;
                    }
                }
            }
            return;
            
            // FIXME incomplete, untested, currently disabled
            if (seg.num.x * seg.num.y <= int(VPRO_CFG::CLUSTERS*VPRO_CFG::UNITS)) { // one oc fits into num of parallel used lanes; only L0 used
                std::cout << "one oc fits into one set" << std::endl;
                for (int out_ch = 0; out_ch < out_dim.ch; ++out_ch) {
                    int ch_lane = 0; // segments of this channel linear enumerated
                    for (int y = 0; y < seg.num.y; ++y) {
                        for (int x = 0; x < seg.num.x; ++x) {
                            std::cout << "x: " << x << ", y: " << y << ", out_ch: " << out_ch << std::endl;
                            auto s = getSegment(x, y, out_ch, out_ch);
                            s->isFirst = true; // FIXME should not be required
                            s->isLast = true;
                            // int dst_ch = out_ch;
                            // // single segment: no second stage required, directly store to output
                            // if (seg.num.x*seg.num.y > 1)
                            // dst_ch += out_dim.ch;
                            // destination: temporary channels
                            // segment output location does not depend on it's input location in the image but the lane it has been mapped to
                            if (lanes_per_ch == 1) { // end result
                                s->out_MM_base = out_dim.mm.channel_base[out_ch];
                            } else { // intermediate results
                                s->out_MM_base = out_dim.mm.channel_base[out_ch+out_dim.ch] + 4*ch_lane;
                            }
                            segments.push_back(s);
                            ch_lane++;
                            // only L0 used, all others are dummies
                            for (unsigned int lane = 1; lane < VPRO_CFG::LANES; lane++)
                                segments.push_back(new DUMMY_SEGMENT());
                        }
                    }
                }
            } else { // one oc does not fit into num of parallel lanes -> need more than one set
                // FIXME
                std::cout << "one oc does not fit into num of parallel lanes" << std::endl;
                for (int out_ch = 0; out_ch < out_dim.ch; ++out_ch) {
                    for (int y = 0; y < seg.num.y; ++y) {
                        for (int x = 0; x < seg.num.x; ++x) {
                            auto s = getSegment(x, y, out_ch, out_ch);
                            s->isFirst = y * seg.num.y + x < (int)VPRO_CFG::parallel_Lanes ? true : false; // first "128" segments are isFirst
                            s->isLast = y * seg.num.y + x >= seg.num.y * seg.num.y - (int)VPRO_CFG::parallel_Lanes ? true : false; // last "128" segments are isLast
                            segments.emplace_back(s);
                        }
                    }
                }
            }
            // fill rest of set with dummies
            while (segments.size() % VPRO_CFG::parallel_Lanes)
                segments.push_back(new DUMMY_SEGMENT());

            //    // sum up intermediate data per channel
            //    for (int out_ch = 0; out_ch < out_dim.ch; out_ch++) {
            //        SEGMENT *s = new SEGMENT;
            //        s->out_channel = out_ch;
            //        s->in_channel = out_ch+out_dim.ch;
            //
            //        s->isFirst = true;
            //        s->isLast = true;
            //        s->dummy = false;
            //
            //        s->in_MM_base.push_back( FIXME here padded_in_MM_base(src_idx, in_ch) +
            //                /* 16 bit elements */ 2 * (x * seg.in.x_stride + y * seg.in.y_stride * in_dim(src_idx).mm.x));
            //        segment->in_MM_y_stride.push_back(in_dim(src_idx).mm.x);
            //    }
            //
            //    // main memory address of segment result
            //    segment->out_MM_base = out_dim.mm.channel_base[out_ch] +
            //    /* 16 bit elements */  2 * (x * seg.out.x_stride + y * seg.out.y_stride * out_dim.mm.x);
            //    segment->out_MM_y_stride = out_dim.mm.x;
            //        
            //        auto s = getSegment(0, 0, out_ch+out_dim.ch, out_ch);
            //        s->isFirst = true; // FIXME should not be required
            //        s->isLast = true;
            //        
            //        
            //    }
            //    
            //    // fill rest of set with dummies
            //    while (segments.size() % VPRO_CFG::parallel_Lanes)
            //        segments.push_back(new DUMMY_SEGMENT());
            std::cout << getSegmentsShortString() << std::endl;
        }
        
        virtual SEGMENT *getSegment(int x, int y, int in_ch, int out_ch) {
            auto seg = Layer::getSegment(x, y, in_ch, out_ch);

            return seg;
        }

        virtual void generateBifLayer(BIF::LAYER &bl) {
            GlobalPool::generateBifLayer(bl);
            bl.pool_avg_shiftr = pool_avg_shiftr;
        }

        // FIXME copy-paste. Move generic version to base_layer 
        virtual void load(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer) {
            std::vector<DMA_COMMANDS::DMA_DESCRIPTOR> dmas_1d, dmas_2d;
            dmas_2d.reserve(2 * VPRO_CFG::parallel_Lanes); // kernel and data load for each lane

            unsigned int cl=0, un=0, ln=0; // cluster, unit and lane of segment
            for (unsigned int i = 0; i < VPRO_CFG::parallel_Lanes; i++) {
                SEGMENT &segment = *segments[i+seg_cnt];
                if (!segment.dummy) {
                    assert(ln == 0);
                    dmas_2d.push_back(dataLoad(segment, cl, un, buffer));
                }
                next_hardware_element(cl, un, ln);
            }

            auto dma_commands = DMA_COMMANDS::DMA_DESCRIPTOR::startBroadcastLoad(dmas_1d, dmas_2d);
            cmd_cnt.dma += (int) dma_commands.size();
            commands.insert(std::end(commands), std::begin(dma_commands), std::end(dma_commands));
        }
        
        virtual void compute(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer, BUFFER &store_buffer) {
            // segments[seg_cnt]: 1st segment within set (i.e. cluster 0, unit 0, lane 0)
            SEGMENT &segment = *segments[seg_cnt];
    
            BIF::COMMAND_SEGMENT cmd;
            cmd.type.type = VPRO_CMD;
            cmd.vpro.command = segment.isFirst ? VPRO_TYPE::global_avgpool2d_start : VPRO_TYPE::global_avgpool2d_add;
            cmd.vpro.lane_mask = 1; // L0 only
            cmd.vpro.lm_base = int(buffer) * VPRO_CFG::LM_SIZE/2;
            int x;
            int y;
            int z;
            factorize(seg.in.w * seg.in.h, x, y, z);
            cmd.vpro.xend = x-1;
            cmd.vpro.yend = y-1;
            cmd.vpro.zend = z-1;
            
            commands.push_back(cmd);
            cmd_cnt.vpro++;
            
            if (segment.isLast) {
                if (lanes_per_ch == 1) {
                    BIF::COMMAND_SEGMENT cmd;
                    cmd.type.type = VPRO_CMD;
                    cmd.vpro.command = global_avgpool2d_divide;
                    // calculate results to a new buffer (switch store buffer)
                    store_buffer = (store_buffer == A) ? B : A;
                    cmd.vpro.lm_base = int(store_buffer) * VPRO_CFG::LM_SIZE/2 + VPRO_CFG::LM_SIZE/4;   // store to out buffer
                    cmd.vpro.pre_shift_right = pre_shift_right;
                    cmd.vpro.multiplier = multiplier;
                    cmd.vpro.shift_right = store_shift_right;
                    commands.push_back(cmd);
                    cmd_cnt.vpro++;
                }                
            }        
        }

    private:
        int sets_per_ch = 0;
        int lanes_per_ch = 0;
    }; // class AveragePooling2D


    class GlobalMaxPool2D: public GlobalPool {

        virtual std::string getLayerTypeName() { return "GlobalMaxPool2D"; }
        virtual LAYERTYPE getLayerType() { return LAYERTYPE::GLOBALMAXPOOL2D; }

        virtual void computeDmaPadding() {
            Layer::computeDmaPadding();
            padding.dma.value = -32768;
        }

        virtual std::string getLayerInfoText() {
            std::stringstream ss;
            ss << GlobalPool::getLayerInfoText();
            return ss.str();
        }  
    }; // class MaxPool2D

} // namespace CNN_LAYER

#endif // GLOBALPOOL_LAYER_H
