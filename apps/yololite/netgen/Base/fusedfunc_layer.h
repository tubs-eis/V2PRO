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
#ifndef FUSEDFUNC_LAYER_H
#define FUSEDFUNC_LAYER_H

#include "base_layer.h"

namespace CNN_LAYER {

    // base class for layer supporting fused activation and 2x2 maxpooling
    class FusedFunc: public Layer {

    public:
        // user-supplied parameters
        ACTIVATION activation{NO_ACTIVATION};
        uint16_t alpha{0}; // leakyrelu

        POOLTYPE pool_type{NO_POOLING}; // NO_POOLING has priority over pool_size and _stride
        std::vector<int> pool_size = {1, 1}; // one element for all dimensions allowed; defaults to 1 if empty
        std::vector<int> pool_stride = {1, 1}; // defaults to pool_size if empty
        PADDING_MODE pool_padding_mode{PADDING_MODE::VALID}; // FIXME not yet used
        bool pool_after_activation{false}; // first apply activation, then pooling (default: first pooling, then act)

        qparam_t upsampling_scale{1};

        qparam_t store_shift_right{0};
        qparam_t rf_frac_bits{0};
        qparam_t alpha_mulh_shift_right{0};


    public:
        virtual void processParams() {
            if (pool_type == NO_POOLING) {
                pool_size = {1, 1};
                pool_stride = {1, 1};
                pool_padding_mode = PADDING_MODE::VALID;
            }
            assert(pool_size.size() == 2);
            assert(pool_stride.size() == 2);
            assert(pool_size[0] == pool_size[1] && "only square pooling size supported");
            assert(pool_stride[0] == pool_stride[1] && "only square pooling stride supported");
            assert(pool_stride[0] == pool_size[0] && "only pool_size == pool_stride supported");
            assert((pool_size[0] == 1 || pool_size[0] == 2) && "unsupported pooling size"); 
            assert(pool_padding_mode == PADDING_MODE::VALID);

            Layer::processParams();

            // should be in computeOutputDim(), but must be executed after child classes' computeOutputDim()
            assert(out_dim.x % pool_size[0] == 0 && "Pooling requires an even input size");
            assert(out_dim.y % pool_size[1] == 0 && "Pooling requires an even input size");
            out_dim.x /= pool_size[0];
            out_dim.y /= pool_size[1];
            
            out_dim.x *= upsampling_scale;
            out_dim.y *= upsampling_scale;
        }
            
        virtual void generateBifLayer(BIF::LAYER &bl) {
            Layer::generateBifLayer(bl);
            
            bl.store_shift_right = store_shift_right;
            bl.relu_6_shift_left = rf_frac_bits;
            bl.alpha = alpha;
            bl.alpha_mulh_shift_right = alpha_mulh_shift_right;

            bl.activation = activation;
            bl.pool_stride = pool_size[0];
        }

        // VPRO pipeline compensation
        // --------------------------
        // Required for small data blocks
        // _act_*() and _shift_store() read RF linear, address increment 1/cycle, highest read address n
        // assumptions about previous instructions still executing in pipeline:
        // - last write goes to address n (or beyond)
        // - writes to addresses n-i happen at least i cycles earlier (i.e. increasing write addresses)
        // ==> last read (address n) determines the number of required NOPs

        virtual BIF::COMMAND_SEGMENT shiftStoreVPRO(BIF::COMMAND_VPRO &mem_layout, BUFFER &store_buffer) {
            BIF::COMMAND_SEGMENT cmd;
            cmd.vpro = mem_layout;

            // calculate results to a new buffer (switch store buffer)
            store_buffer = (store_buffer == A) ? B : A;
            cmd.vpro.lm_base = int(store_buffer) * VPRO_CFG::LM_SIZE/2 + VPRO_CFG::LM_SIZE / 4;

            if (upsampling_scale != 1){
                cmd.vpro.command = VPRO_TYPE::shift_store_upsample;
            } else {
                cmd.vpro.command = VPRO_TYPE::shift_store;
            }

            mem_layout.xend = (mem_layout.xend + 1) * upsampling_scale - 1;
            mem_layout.yend = (mem_layout.yend + 1) * upsampling_scale - 1;
            mem_layout.lm_ch_stride *= upsampling_scale * upsampling_scale;

            // Pipeline compensation: insert NOPs before _shift_store()
            // number of cycles before last address n is read:
            int implicit_wait_cycles = (cmd.vpro.xend+1) * (cmd.vpro.yend+1) * (cmd.vpro.zend+1) - 1;
            cmd.vpro.nops = std::max(0, W2R_BUBBLE_CYCLES - implicit_wait_cycles);

            cmd.vpro.lm_lane_stride = lm_lane_stride;
            return cmd;
        }

        virtual BIF::COMMAND_SEGMENT maxpool2x2VPRO(BIF::COMMAND_VPRO &mem_layout) {
            BIF::COMMAND_SEGMENT cmd;
            cmd.vpro = mem_layout;
            cmd.vpro.command = VPRO_TYPE::maxpool2x2_fused;

            // VPRO pipeline compensation
            // insert NOPs before _maxpool2x2 (i.e. before 1st max() instruction
            // read addresses increment >= 1/cycle ==> highest address is critical
            int implicit_wait_cycles = ((cmd.vpro.xend>>1)+1) * (cmd.vpro.yend+1) * (cmd.vpro.zend+1) - 1;
            cmd.vpro.nops = std::max(0, W2R_BUBBLE_CYCLES - implicit_wait_cycles);

            // insert wait cycles before 2nd max() instruction by appending garbage computation to 1st max()
            implicit_wait_cycles = ((cmd.vpro.xend+1)>>1) * ((cmd.vpro.yend>>1)+1) * (cmd.vpro.zend+1) - 1;
            int inter_instr_nops = std::max(0, W2R_BUBBLE_CYCLES - implicit_wait_cycles);
            // no explicit NOPs between instructions, add cycles to 1st max (and unintentionally also to 2nd max) instead
            if (inter_instr_nops) {
                // write garbage behind useful data; append as few cycles as possible
                // only happens for small data block -> there should be enough space behind useful data
                if (cmd.vpro.zend) {
                    cmd.vpro.zend += ceil_div(inter_instr_nops, ((cmd.vpro.xend>>1)+1) * (cmd.vpro.yend+1));
                } else if (cmd.vpro.yend) {
                    cmd.vpro.yend += ceil_div(inter_instr_nops, (cmd.vpro.xend>>1)+1);
                } else {
                    cmd.vpro.xend += 2*inter_instr_nops;
                }
                // FIXME wait cycles added at the end of this command; could subtract these cycles from the next command's NOPs
            }

            // _maxpool2x2() changes the memory layout for all following commands
            mem_layout.xend /= 2; // w/2-1 = (w-1)/2
            mem_layout.yend /= 2;
            mem_layout.lm_ch_stride /= 4;

            return cmd;
        }

        virtual BIF::COMMAND_SEGMENT activationVPRO(BIF::COMMAND_VPRO &mem_layout) {
            BIF::COMMAND_SEGMENT cmd;
            cmd.vpro = mem_layout;
            cmd.vpro.command = VPRO_TYPE::activation_fused;

            // prepare shift width for shift_store command
            int sigmoid_frac_bits = std::min(14, (int)rf_frac_bits);
            int output_frac_bits = rf_frac_bits - store_shift_right;
            if (activation == SIGMOID) {
                mem_layout.shift_right = sigmoid_frac_bits - output_frac_bits;
            } else if (activation == SWISH) {
                int lm_shift_right = 24 - 16; // shift width for transfer x RF->LM
                int n_frac_bits = sigmoid_frac_bits + ((int)rf_frac_bits - lm_shift_right); // post-mul fractional bits
                cmd.vpro.shift_right = n_frac_bits - output_frac_bits; // mul_h_bit_shift for last multiplication x*sigmoid(x)
                mem_layout.shift_right = 0; // used by next command shift_store
            } else {
                mem_layout.shift_right = store_shift_right;
            }

            // VPRO pipeline compensation
            // insert NOPs before 1st instruction
            int implicit_wait_cycles = (cmd.vpro.xend+1) * (cmd.vpro.yend+1) * (cmd.vpro.zend+1) - 1;
            cmd.vpro.nops = std::max(0, W2R_BUBBLE_CYCLES - implicit_wait_cycles);

            // same number of wait cycles between instructions within activation
            // relu and leakyrelu use only a single instruction, all others need wait cycles between instructions
            if (cmd.vpro.nops && activation != RECT && activation != LEAKY) {
                // add wait cycles to all instructions
                // append garbage behind useful data; append as few cycles as possible
                if (cmd.vpro.zend) {
                    cmd.vpro.zend += ceil_div(cmd.vpro.nops, (cmd.vpro.xend+1) * (cmd.vpro.yend+1));
                } else if (cmd.vpro.yend) {
                    cmd.vpro.yend += ceil_div(cmd.vpro.nops, cmd.vpro.xend+1);
                } else {
                    cmd.vpro.xend += cmd.vpro.nops;
                }
                // FIXME wait cycles added at the end of this command; could subtract these cycles from the next command's NOPs
            }

            return cmd;
        }
  
        virtual void poolActivationVPRO(BIF::COMMAND_VPRO &mem_layout) {
            if (pool_size[0] > 1 && !pool_after_activation) {
                cmd_cnt.vpro++;
                commands.push_back(maxpool2x2VPRO(mem_layout));
            }
            if (activation != NO_ACTIVATION) {
                cmd_cnt.vpro++;
                commands.push_back(activationVPRO(mem_layout));
            }
            if (pool_size[0] > 1 && pool_after_activation) {
                cmd_cnt.vpro++;
                commands.push_back(maxpool2x2VPRO(mem_layout));
            }
        }

        virtual std::string getLayerInfoText() {
            std::stringstream ss;
            ss << Layer::getLayerInfoText();
            ss << "  pool_size " << pool_size[0] << ", pool_after_activation " << pool_after_activation << ", activation " << to_char(activation) << ", upsampling_scale " << upsampling_scale << "\n";
            return ss.str();
        }

    protected:
        uint32_t RF_RELU_6_BASE;
    }; // class FusedFunc
    
} // namespace CNN_LAYER
#endif // FUSEDFUNC_LAYER_H
