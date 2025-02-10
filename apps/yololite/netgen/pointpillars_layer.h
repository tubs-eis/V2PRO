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
#ifndef POINTPILLARS_H
#define POINTPILLARS_H

#include "conv1d_layer.h"

namespace CNN_LAYER {
/* Data format for PointPillars
Inputs:
    - (0): Input point cloud padded to shape (max_points, 1, in_ch)
    - (1): Grid segmentation of shape (n_segments + max_points, 1, 1), containing
        - valid points per segment
        - segment indices of valid points
Memory layout:
    Local memory
    (before computation)
        - [0, max_points_per_seg): grid indices
        - [max_points_per_seg, 2*max_points_per_seg): inputs
        - weights
    (after computation)
        - [0, max_points_per_seg): grid indices
        - [max_points_per_seg, max_points_per_seg + seg.out.w * seg.out.h): outputs of L0
        - [RF_SIZE + max_points_per_seg +     seg.out.w * seg.out.h, 
           RF_SIZE + max_points_per_seg + 2 * seg.out.w * seg.out.h): outputs of L1
        - weights
    Register file
        - [0, seg.out.w * seg.out.h): 2D output segment
        - [seg.out.w * seg.out.h, seg.out.w * seg.out.h + max_points_per_seg): point-wise features
        - [RF_SIZE - in_ch - 1, RF_SIZE): weights
*/


class PointPillars: public Conv1D {
    // user-supplied parameters
public:
    // grid configuration
    float xmin{0.};
    float xmax{0.};
    float ymin{0.};
    float ymax{0.};
    float res{0.};

    int max_points_per_seg{};

    //SCATTER_POOL_MODE pool_mode{SCATTER_POOL_MODE::NONE};

  // methods
public:
    virtual std::string getLayerTypeName() { return "PointPillars"; }
    virtual LAYERTYPE getLayerType() { return LAYERTYPE::POINTPILLARS; }

    virtual void processParams() {
        padding_mode = PADDING_MODE::VALID;
        assert(src_layers.size() == 2 && "expecting inputs of format [features, grid segmentation]");
        assert(parallel_outchannels_per_lane == 1);
        assert(xmin != xmax && ymin != ymax);
    
        n_cells_x = floor((xmax-xmin) / res);
        n_cells_y = floor((ymax-ymin) / res);
        n_cells = n_cells_x * n_cells_y;
        layercfg.use_dma_loop_extension = false; // loop extension currently does not work with this layer
        layercfg.use_dma_store_splitter = false;

        lm_lane_stride = seg.out.h * seg.out.w;

        Conv1D::processParams();
    };

    virtual void computeOutputDim() {
        out_dim.x = n_cells_x;
        out_dim.y = n_cells_y;
    }

    virtual void generateBifLayer(BIF::LAYER &bl) {
        Conv1D::generateBifLayer(bl);
        bl.seg_out_w = seg.out.w;
        bl.seg_out_h = seg.out.h;

        // Store address of grid segmentation (src_layers[1])
        bl.input.mm_base = in_dim(1).mm.channel_base[0];
    }

    int calc_flat_segment_index(int x, int y) {
        return y * seg.num.x + x;
    }

    /*============================================ Segmentation ===========================================================*/
    virtual void setSegmentDimensions() {
        // following parameters are determined and passed by qutk:
        // seg.num.x, seg.num.y, seg.out.w, seg.out.h
        // following parameters are not used since they are dynamic and determined during runtime:
        // seg_in_w, seg_in_h
        return;
    }

    virtual SEGMENT *getSegment(int x, int y, int in_ch, int out_ch) {
        // Previously in cnn_struct.h, constructur of CONV
        auto segment = Layer::getSegment(x, y, in_ch, out_ch);

        // base address of features for the current input channel,
        // the offset for current segment has to be determined dynamically
        segment->in_MM_base[0] = in_dim(0).mm.channel_base[groups == 1 ? in_ch : out_ch];

        // base address for indices of points within a segment
        segment->in_MM_base[1] = in_dim(1).mm.base;
        return segment;
    }

    /*============================================ Command generation =====================================================*/

    BIF::COMMAND_SEGMENT setMasksVPRO(uint16_t cluster_mask, uint16_t unit_mask, uint16_t segment_index) {
        BIF::COMMAND_SEGMENT cmd;
        cmd.type.type = VPRO_CMD;
        cmd.vpro.command = VPRO_TYPE::set_masks;
        cmd.vpro.cluster_mask = cluster_mask;
        cmd.vpro.unit_mask = unit_mask;
        cmd.vpro.offset = segment_index;
        return cmd;
    }

    virtual BIF::COMMAND_SEGMENT convVPRO(const CNN_LAYER::SEGMENT &segment, BUFFER &buffer, uint32_t lane_mask, BIF::COMMAND_VPRO &mem_layout) {
        auto cmd = Conv1D::convVPRO(segment, buffer, lane_mask, mem_layout);
        cmd.vpro.lm_base += max_points_per_seg; // lm base address of features (first max_points_per_seg entries are reserved for grid indices)
        cmd.vpro.rf_base = seg.out.w * seg.out.h; // rf base of results (first entries reserved for 2D output segment)

        mem_layout.rf_base = cmd.vpro.rf_base;
        
        return cmd;
    }

    virtual void poolActivationVPRO(BIF::COMMAND_VPRO &mem_layout) {
        BIF::COMMAND_SEGMENT cmd;
        cmd.vpro = mem_layout;
        cmd.vpro.command = VPRO_TYPE::relu_pool_scatter;
        cmd.vpro.lm_base = mem_layout.lm_base + 2 * max_points_per_seg; // lm base of results (offset due to grid indices and features)
        cmd.vpro.pp_index_buffer = int(buffer_indices_calc) *(VPRO_CFG::LM_SIZE / 2);
        
        cmd_cnt.vpro++;
        commands.push_back(cmd);

        // memory layout of 2D output segment after scattering
        mem_layout.rf_base = 0; // results at index 1 in RF
        mem_layout.lm_base = cmd.vpro.lm_base;
        mem_layout.lm_lane_stride = seg.out.h * seg.out.w;
        mem_layout.xend = 0;
        mem_layout.yend = 0;
        mem_layout.zend = seg.out.h * seg.out.w - 1;
    }

    void resetIndicesVPRO() {
        BIF::COMMAND_SEGMENT cmd;
        cmd.type.type = VPRO_CMD;
        cmd.vpro.command = VPRO_TYPE::reset_indices;
        cmd.vpro.lm_base = int(buffer_indices_load) * VPRO_CFG::LM_SIZE/2;
        cmd.vpro.zend = max_points_per_seg - 1;
        commands.push_back(cmd);
        commands.push_back(VPRO_COMMANDS::wait());
    }

    virtual DMA_COMMANDS::DMA_DESCRIPTOR dynamicDataLoad(const SEGMENT &segment, int cluster, int unit, BUFFER &buffer, bool is_input_feature=true) {
        DMA_COMMANDS::DMA_DESCRIPTOR dma;
        dma.dir = e2l2D;
        dma.cluster = cluster;
        dma.unit = unit;
        // number of words (x_size) is determined during runtime
        dma.x_size = 0; 
        dma.y_size = 1;
        // use y_leap to encode index of current segment
        dma.y_leap = calc_flat_segment_index(segment.x_seg, segment.y_seg);

        uint32_t lm_offset = int(buffer) * (VPRO_CFG::LM_SIZE / 2);
        if (is_input_feature) {
            // For input feature: base address of the current input channel, offset to the respective segment is added during runtime
            dma.mm_addr = segment.in_MM_base[0];
            dma.lm_addr = lm_offset + max_points_per_seg;
        } else {
            // For grid indices: base address of indices (offset by seg.num.x * segn_num_y for the point counts per segment)
            dma.mm_addr = segment.in_MM_base[1] + 2 * seg.num.x * seg.num.y;
            dma.lm_addr = lm_offset;
        }
        return dma;
    }

    virtual BIF::COMMAND_SEGMENT dataStore(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER &buffer_load) {
        BIF::COMMAND_SEGMENT cmd = dataStore2D(segment, cluster, unit, lane, buffer_load);

        uint32_t lm_offset = 2 * max_points_per_seg + lane * seg.out.h * seg.out.w; // store output behind indices and inputs
        cmd.dma.lm_addr = (int(buffer_load) * VPRO_CFG::LM_SIZE / 2) + VPRO_CFG::LM_SIZE / 4 + lm_offset;
        return cmd;
    }

    // can these segments be placed into the same unit?
    virtual bool compatibleSegmentsBlock(const SEGMENT *a, const SEGMENT *b, int lane, int lane_out_ch) const {

        if (lane % VPRO_CFG::LANES == 0 && lane_out_ch == 0) // first location has no dependencies
            return true;

        if (!a || !b)
            return true; // there is no segment to compare to

        // dummy segments compatible to everything
        if (a->dummy || b->dummy)
            return true;

        // segments requiring different input data are not compatible
        if (a->x_seg != b->x_seg) {
            return false;
        }
        if (a->y_seg != b->y_seg) {
            return false;
        }
        return true;
    }

    virtual void load(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer) {
        std::vector<DMA_COMMANDS::DMA_DESCRIPTOR> dmas_1d, dmas_2d;

        dmas_1d.reserve(2 * VPRO_CFG::parallel_Lanes); // kernel and bias load for each lane

        // broadcasting variant (padded computation):
        // maximum number of points in current set of segments is used as zend for all segments
        // thus for segments that actually have less points, garbage values are computed at the end 
        // and should be written to RF_DISCARD_ADDRESS -> reset indices to RF_DISCARD_ADDRESS
        if (!use_segmentwise_multicast && !segments[seg_cnt]->dummy && segments[seg_cnt]->isFirst) {
            resetIndicesVPRO();
        }

        unsigned int cl=0, un=0, ln=0; // cluster, unit and lane of segment
        for (unsigned int i = 0; i < VPRO_CFG::parallel_Lanes; i++) {
            SEGMENT &segment = *segments[i+seg_cnt];
            if (!segment.dummy) {
                // Load coefficients for input and output of this segment to respective hardware element
                if (segment.isFirst || !load_weights_at_once) {
                    dmas_2d.push_back(kernelLoad(segment, cl, un, ln, buffer));
                }
                if (segment.isFirst) {
                    dmas_1d.push_back(biasLoad(segment, cl, un, ln, buffer));
                }
                if (segment.isFirst && ln == 0) { // Load grid indices
                    dmas_2d.push_back(dynamicDataLoad(segment, cl, un, buffer_indices_load, false));
                }
                
                if (ln == 0) { // Load input data
                    dmas_2d.push_back(dynamicDataLoad(segment, cl, un, buffer));      
                }
            }
            next_hardware_element(cl, un, ln);
        }

        if (!segments[seg_cnt]->dummy && segments[seg_cnt]->isLast) {
            buffer_indices_load = (buffer_indices_load == BUFFER::A) ? BUFFER::B : BUFFER::A;
        }

        auto dma_commands = DMA_COMMANDS::DMA_DESCRIPTOR::startBroadcastLoad(dmas_1d, dmas_2d);
        cmd_cnt.dma += (int) dma_commands.size();
        commands.insert(std::end(commands), std::begin(dma_commands), std::end(dma_commands));
    }


    void compute_multicast(std::vector<SEGMENT *> segments, int seg_cnt, BUFFER &buffer) {
        // VPRO commands are multicasted for Pointpillars, instead of being broadcasted.
        // That is, only segments with the same x/y index are comprised into a multicast for the corresponding units.
        // This is achieved by setting the cluster and unit mask before issuing the VPRO commands
        // This allows to use different vector lengths for each segment during runtime, to account for varying number of points per segment

        unsigned int si = seg_cnt;

        // which lanes have non-dummy segments?
        uint32_t lane_mask = 0;
        for (unsigned int lane = 0; lane < VPRO_CFG::LANES; lane++) {
            for (uint si = seg_cnt + lane; si < seg_cnt + VPRO_CFG::parallel_Lanes; si += VPRO_CFG::LANES) {
                if (!segments[si]->dummy) {
                    lane_mask |= 1 << lane;
                    break;
                }
            }
        }

        uint32_t unit_mask = 0;
        uint32_t cluster_mask = 0;
        SEGMENT *start_seg = segments[seg_cnt];

        si = seg_cnt;

        while(si < seg_cnt + VPRO_CFG::parallel_Lanes) {
            start_seg = segments[si];

            // get all segments with the same x/y index and build the corresponding cluster/unit masks
            while(si < seg_cnt + VPRO_CFG::parallel_Lanes && segments[si]->x_seg==start_seg->x_seg && segments[si]->y_seg==start_seg->y_seg) {
                if (!segments[si]->dummy) {
                    // compute unit and cluster index of the current segment
                    int lane_index = si - seg_cnt;
                    int unit    = (lane_index / VPRO_CFG::LANES) % VPRO_CFG::UNITS;
                    int cluster = (lane_index / (VPRO_CFG::LANES * VPRO_CFG::UNITS) % VPRO_CFG::CLUSTERS);
                    unit_mask    |= 1 << unit;
                    cluster_mask |= 1 << cluster;
                }
                si++;
            }

            // if any unit is addressed set the masks to send the required commands to the respective units
            if (unit_mask!=0 && cluster_mask!=0) {
                BIF::COMMAND_VPRO mem_layout;
                commands.push_back(setMasksVPRO(cluster_mask, unit_mask, calc_flat_segment_index(start_seg->x_seg, start_seg->y_seg)));
                commands.push_back(convVPRO(*start_seg, buffer, lane_mask, mem_layout));
                cmd_cnt.vpro++;

                // post-processing: activation+pooling+store
                if (start_seg->isLast) {
                    poolActivationVPRO(mem_layout);
                    commands.push_back(shiftStoreVPRO(mem_layout, buffer));
                    cmd_cnt.vpro +=2;
                    
                }
            }
            // reset masks for the next block of segments
            cluster_mask = 0;
            unit_mask = 0;
        }
        // reset cluster/unit masks to broadcasting for following layers
        commands.push_back(setMasksVPRO(0xffff, 0xffff, calc_flat_segment_index(start_seg->x_seg, start_seg->y_seg)));
    }

    void compute(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer, BUFFER &store_buffer) {
        if (use_segmentwise_multicast) {
            compute_multicast(segments, seg_cnt, buffer);
        } else {
            Conv1D::compute(segments, seg_cnt, buffer, buffer);
        }
        // next segment will have new x/y indices: swap double buffer for grid indices
        if (segments[seg_cnt]->isLast) {
            buffer_indices_calc = (buffer_indices_calc == BUFFER::A) ? BUFFER::B : BUFFER::A;
        }
    }


protected:
    int n_cells_x{0};
    int n_cells_y{0};
    int n_cells{0};

    // Multicast implementation (use_segmentwise_multicast=true): 
    // - vpro commands are multicasted to units that compute segments with the same x/y index
    // - vector length is set dynamically during runtime to the point count of the respective segment
    // Broadcast implementation (use_segmentwise_multicast=false):
    // - vpro commands are broadcasted to all units
    // - vector length is set dynamically to the maximum point count for current segments
    // - lanes that compute segments with less points will compute garbage that is discarded by scattering to RF_DISCARD_ADDR
    bool use_segmentwise_multicast{false};

    // Double buffering for indices:
    // Indices remain constant for segments with the same x and y index and are loaded only once for the first segment
    // Consequently, index buffers are only swapped after the last segment of the respective output channel.
    // In contrast, double buffers for input features change for each segment of the respective output channel.
    BUFFER buffer_indices_load{BUFFER::A};
    BUFFER buffer_indices_calc{BUFFER::A};
}; // class PointPillars


} // namespace CNN_LAYER

#endif // POINTPILLARS_H
