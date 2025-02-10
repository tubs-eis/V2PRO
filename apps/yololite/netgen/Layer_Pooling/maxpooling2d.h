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
#ifndef MAXPOOL2D_H
#define MAXPOOL2D_H

#include "Base/base_layer.h"
#include "Layer_Conv/conv_layer.h"

namespace CNN_LAYER {

    class MaxPool2D: public Conv2D{

    public:
        virtual std::string getLayerTypeName() { return "MaxPool2D"; }
        virtual LAYERTYPE getLayerType() { return LAYERTYPE::MAXPOOL2D; }

        virtual void processParams() {
            groups = out_dim.ch;    // like depthwise

            // Workaround: dirty overwrite to enforce conv2 to just use the regular conv segmentation
            pool_type = NO_POOLING;
            kernel_length = pool_size[0];
            stride = pool_stride[0];
            padding_mode = pool_padding_mode;

            assert(pool_stride[0] == pool_stride[1]);
            assert(pool_size[0] == pool_size[1]);

            pool_stride = {1, 1};
            pool_size = {1, 1};

            Conv2D::processParams();
        }

        virtual BIF::COMMAND_SEGMENT convVPRO(const CNN_LAYER::SEGMENT &segment, BUFFER &buffer, uint32_t lane_mask, BIF::COMMAND_VPRO &mem_layout) {
            auto cmd = Conv2D::convVPRO(segment, buffer, lane_mask, mem_layout);
            cmd.vpro.command = VPRO_TYPE::max_pooling;
            return cmd;
        }

        virtual void computeDmaPadding() {
            Layer::computeDmaPadding();
            padding.dma.value = -32768;
        }

        virtual void load(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer) {
            std::vector<DMA_COMMANDS::DMA_DESCRIPTOR> dmas_1d, dmas_2d;

            unsigned int cl = 0, un = 0, ln = 0; // cluster, unit and lane of segment
            for (unsigned int i = 0; i < VPRO_CFG::parallel_Lanes; i++) {
                SEGMENT &segment = *segments[i + seg_cnt];
                if (!segment.dummy) {
                    // Load input data
                    if (ln == 0) {
                        dmas_2d.push_back(dataLoad(segment, cl, un, buffer));
                    }
                }
                next_hardware_element(cl, un, ln);
            }

            auto dma_commands = DMA_COMMANDS::DMA_DESCRIPTOR::startBroadcastLoad(dmas_1d, dmas_2d);
            cmd_cnt.dma += (int) dma_commands.size();
            commands.insert(std::end(commands), std::begin(dma_commands), std::end(dma_commands));
        }

    }; // class MaxPool2D

} // namespace CNN_LAYER

#endif // MAXPOOL2D_H
