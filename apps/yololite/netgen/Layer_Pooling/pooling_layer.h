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
#ifndef POOL_H
#define POOL_H

#include "Base/base_layer.h"

// TODO
// 
// bif.h: Add pool types
// check if computeInputPadding works with pool_sizes instead of kernel sizes
// consider additional layer in Inheritance for local operators to move e.g. padding features
// check if generate_Commands from base class is applicable
// check if protected members in Pooling are needed
// consider moving avg mul factors from attributes to params
// consider renaming in nn quant params --> attributes
// computeDmaPadding: consider renaming variables

// markus 
// // pool stride must fit into segment
// if (c.seg_in_w % pool_size)
//     continue;

namespace CNN_LAYER {

    // generic base class for pooling operations
    // inherits from base class layer as conv layer members do not match pooling layer members
    class Pooling: public Layer {
    // access specifier in inheritance
    // public: public/protected members of base class are public/protected in derived class
    // protected: public/protected members of base class are protected in derived class 
    // private: public/protected members of base class are privat in derived class
        
    // user-supplied parameters
    public:
        POOLTYPE pool_type{NO_POOLING}; // NO_POOLING has priority over pool_size and _stride
        std::vector<int> pool_size{}; // one element for all dimensions allowed; defaults to 1 if empty
        std::vector<int> pool_stride{}; // defaults to pool_size if empty
        PADDING_MODE pool_padding_mode{PADDING_MODE::VALID};
        qparam_t store_shift_right{0};

    // methods 
    public:
        virtual unsigned int getPoolDim() = 0;  // 1 for pool1d, 2 for pool2d, etc.

        virtual void processParams() {

            if (pool_type == NO_POOLING) {
                pool_size.clear();
                pool_stride.clear();
            }

            // allowed number of elements in pool_size inspired by tensorflow:
            // 0: pool_size = 1
            // 1: same value applies to all dimensions
            if (!pool_size.size())
                pool_size.push_back(1);
            assert((pool_size.size() == 1 || pool_stride.size() == getPoolDim()) && "pool_size: invalid number of dimensions");
            pool_size.resize(getPoolDim(), pool_size.front());

            // stride
            // 0: stride = pool_size
            // 1: same value applies to all dimensions
            if (!pool_stride.size())
            pool_stride = pool_size;
            assert((pool_stride.size() == 1 || pool_stride.size() == getPoolDim()) && "pool_stride: invalid number of dimensions");
            pool_stride.resize(getPoolDim(), pool_stride.front()); // replicate single element to match number of dimensions

            bool all1 = true;
            for (int ps: pool_size) {
                if (ps != 1)
                    all1 = false;
                assert(ps == pool_size.front() && "limited implementation (as of 2023-04-24)");
            }
            for (int ps: pool_stride) {
                if (ps != 1)
                    all1 = false;
                assert(ps == pool_stride.front() && "limited implementation (as of 2023-04-24)");
            }
            if (all1)
                pool_type = NO_POOLING;
            assert((pool_size.front() >= 1 && pool_size.front() <= 7) && "Unsupported pooling size");
    
            groups = in_dim(0).ch;

            Layer::processParams();
        }

        // binary export
        virtual void generateBifLayer(BIF::LAYER &bl) {
            Layer::generateBifLayer(bl);

            //bl.pool_type = pool_type;

            bl.pool_size_w = pool_size[0];
            bl.pool_size_h = pool_size[1];
            bl.pool_size_ch = pool_size[2];
            
            bl.pool_stride_w = pool_stride[0];
            bl.pool_stride_h = pool_stride[1];
            bl.pool_stride_ch = pool_stride[2];

            bl.pad = padding.dma;
            bl.pad.value = 0;

            bl.store_shift_right = store_shift_right;
        }

        virtual std::string getLayerInfoText() {
            std::stringstream ss;
            ss << Layer::getLayerInfoText();
            // FIXME ss << "  kernel " << kernel_length << "x" << kernel_length << ", stride " << stride << ", groups " << groups << "\n";
            return ss.str();
        }

    }; // class Pooling

    class Pooling2D: public Pooling{

    public:
        virtual unsigned int getPoolDim() { return 2; }

        virtual void computeOutputDim() {
            out_dim.ch = in_dim(0).ch;

            switch (pool_type) {
                case NO_POOLING:
                    out_dim.x = in_dim(0).x;
                    out_dim.y = in_dim(0).y;
                    break;
                case MAX_POOLING:
                case AVG_POOLING:
                    // see keras doc
                   
                    out_dim.x = (in_dim(0).x - (pool_padding_mode == PADDING_MODE::VALID ? pool_size[0] : 1)) / pool_stride[0] + 1;
                    out_dim.y = (in_dim(0).y - (pool_padding_mode == PADDING_MODE::VALID ? pool_size[1] : 1)) / pool_stride[1] + 1;
                    break;
                default:
                assert(false && "invalid pooling type");
            }
        }

        // in most parts reused from conv layer
        virtual void computeInputPadding() {
            // compute algorithm-view input padding from PADDING_MODE; FIXME this could be moved to nn_quantization
            // algorithm-view:
            // - padding.algo specifies additional pixels around in_dim.(x|y)
            // - convolution input size is in_dim.(x|y) + padding.algo
            // tensorflow, keras etc. use different formulas for padding and hence output size.
            // This is valid for tensorflow:

            


            if (pool_padding_mode == PADDING_MODE::SAME && pool_type != NO_POOLING) { 
                // https://www.pico.net/kb/what-is-the-difference-between-same-and-valid-padding-in-tf-nn-max-pool-of-tensorflow/
                int pad_x = (out_dim.x - 1) * pool_stride[0] + pool_size[0] - in_dim(0).x;
                int pad_y = (out_dim.y - 1) * pool_stride[1] + pool_size[1] - in_dim(0).y;
                padding.algo.left    = pad_x / 2;
                padding.algo.right   = pad_x - padding.algo.left;
                padding.algo.top     = pad_y / 2;
                padding.algo.bottom  = pad_y - padding.algo.top;

            
            }

            
            // padding.dma will be set later after segmentation is known
        }

    }; // class Pooling2D

    class AveragePooling2D: public Pooling2D{
    public:

        qparam_t pool_avg_shiftr{0};
    
        virtual std::string getLayerTypeName() { return "AveragePooling2D"; }
        virtual LAYERTYPE getLayerType() { return LAYERTYPE::AVGPOOL2D; }

        virtual std::string getLayerInfoText() {  
            std::stringstream ss;
            ss << Pooling2D::getLayerInfoText();
            ss << "  AveragePooling2D \n";
        return ss.str();
        }  

        virtual void setSegmentDimensions();
        virtual void generateWeights();

        virtual mm_addr_type getKernelMMAddr(int x=0, int y=0) {
            return getWeightsMMAddr() + sizeof(weight_t) * (y * out_dim.x + x);
        }
        
        virtual int expectedWeightCount() {
            return out_dim.x * out_dim.y;
        }

        virtual void generateBifLayer(BIF::LAYER &bl) {
            Pooling2D::generateBifLayer(bl);
            bl.pool_avg_shiftr = pool_avg_shiftr;
        }

        virtual DMA_COMMANDS::DMA_DESCRIPTOR kernelLoad(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER buffer);
        virtual DMA_COMMANDS::DMA_DESCRIPTOR dataLoad(const SEGMENT &segment, int cluster, int unit, int lane, BUFFER buffer);

        // FIXME load() and compute() are not virtual, i.e. never called from parent classes and not here either. Remove? Fix?
        void load(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer);
        
        void compute(std::vector<SEGMENT *> &segments, int seg_cnt, BUFFER &buffer, BUFFER &store_buffer);
        BIF::COMMAND_SEGMENT load_pool_store(const CNN_LAYER::SEGMENT &segment, BUFFER &buffer, BUFFER &store_buffer);

    }; // class AveragePooling2D

} // namespace CNN_LAYER

#endif
