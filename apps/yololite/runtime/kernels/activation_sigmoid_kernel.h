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
//
// Created by gesper on 02.03.23.
//

#ifndef SIGMOID_PIECEWISE_APPROX_SIGMOID_H
#define SIGMOID_PIECEWISE_APPROX_SIGMOID_H

#include "bif.h"
#include <vpro.h>
#include "eisv.h"
#include <cmath>

/**
 *  Calculation of the SIGMOID function
 *      based on piecewise approximation with polynoms
 *      uses the symmetric feature of the sigmoid
 *      3 version differ in accuracy and calculation speed
 *      pooled version skips every 2nd RF entries in x and y direction by reordering data in first process step
 *
 * sigmoid_fast (symmetric feature exploited [negatives]; one 2nd-order poly [0-limit], const regions [> limit])
 *
 * sigmoid_medium
 *      5 different input regions. uses 3x third order polyonom to approx. const 0/1 in begin/end
 *      symmetric: middle region is split into 2. 2 regions for third order calc left + 2 constants
 *          entry_points: _sigmoid_medium(...),  _sigmoid_medium_pool(...)
 *      third order requires temporary data (x², x³, ...) RF is split into smaller regions -> temporary data stored in LM
 *
 * sigmoid_precise (TODO: not implemented! maybe unused)
 *
 * Implementation splits the rf into several segments to store intermediate values
 *  maximal block size is 128
 * To process large blocks, the data is partly stored in LM (intermediate) in beginning (lm_output_offset region is used)
 * Fixed point precision is fixed to .11 for input and .18 (RF) for output of the sigmoid
 * Output will be shifted to .11 when stored in LM (TODO: adapt other parameters)
 * Different input requires additional shift (TODO: adapt other parameters)
 */

constexpr int32_t float_to_fix(float value, int precision){
    return int32_t(value * std::pow(2, precision));
}

/**
 * Fast implementation of an sigmoid approximation. Using a quadratic approximation point-mirrored around x=0.
 * approximation:
 *     f(x) = a * (x - b)² + c
 *     if x > limit:
 *         f(x) = f(limit)
 *
 * symmetric attribute of sigmoid is used:
 *      approx for origin symmetric)
 *      negative * (-1)
 *      +0.5 (shift for sigmoid -> not origin symmetric)
 */
template<int mem_input_fractional_bits>
inline void _sigmoid_fast(uint32_t xend, uint32_t yend, uint32_t zend, uint32_t ch_stride, uint32_t base = 0, LANE lanes = L0_1) {
  
    constexpr int32_t a = float_to_fix(-0.03125, mem_input_fractional_bits);
    constexpr int32_t b = float_to_fix(3.890625, mem_input_fractional_bits);
    constexpr int32_t c = float_to_fix(0.984375 - 0.5, mem_input_fractional_bits);
    constexpr int32_t limit = float_to_fix(3.829, mem_input_fractional_bits);

    // sim_printf("vpro_mul_h_bit_shift() before: %d", core_->io_read(VPRO_MUL_SHIFT_REG_ADDR));

    // for multiplications
    //  square: x*x
    //  *a
    vpro_lane_sync();
    vpro_mul_h_bit_shift(mem_input_fractional_bits);
    // sim_printf("vpro_mul_h_bit_shift() after: %d", core_->io_read(VPRO_MUL_SHIFT_REG_ADDR));

    int w = xend+1;
    // set negative flag, do not modify data
    VPRO::DIM3::PROCESSING::add(lanes,
                                DST_ADDR (base, 1, w, ch_stride),
                                SRC1_ADDR(base, 1, w, ch_stride),
                                SRC2_IMM_3D(0),
                                xend, yend, zend,
                                false, true);

    // move all to positive region (using neg flag)
    VPRO::DIM3::PROCESSING::mull_neg(lanes,
                                     DST_ADDR (base, 1, w, ch_stride),
                                     SRC1_ADDR(base, 1, w, ch_stride),
                                     SRC2_IMM_3D(-1),
                                     xend, yend, zend);

    // min ( < -limit ) => 0 = f(-limit)
    // max ( > +limit ) => 1 = f(+limit)
    VPRO::DIM3::PROCESSING::min(lanes,
                                DST_ADDR (base, 1, w, ch_stride),
                                SRC1_ADDR(base, 1, w, ch_stride),
                                SRC2_IMM_3D(limit),
                                xend, yend, zend);

    // x - b
    VPRO::DIM3::PROCESSING::sub(lanes,
                                DST_ADDR (base, 1, w, ch_stride),
                                SRC1_IMM_3D(b),
                                SRC2_ADDR(base, 1, w, ch_stride),
                                xend, yend, zend);
    // square
    VPRO::DIM3::PROCESSING::mulh(lanes,
                                 DST_ADDR (base, 1, w, ch_stride),
                                 SRC1_ADDR(base, 1, w, ch_stride),
                                 SRC1_ADDR(base, 1, w, ch_stride),
                                 xend, yend, zend);

    // mul a
    VPRO::DIM3::PROCESSING::mulh(lanes,
                                 DST_ADDR (base, 1, w, ch_stride),
                                 SRC1_ADDR(base, 1, w, ch_stride),
                                 SRC2_IMM_3D(a),
                                 xend, yend, zend);

    // add c = 0.984375 - 0.5 (origin symmetric)
    VPRO::DIM3::PROCESSING::add(lanes,
                                DST_ADDR (base, 1, w, ch_stride),
                                SRC1_ADDR(base, 1, w, ch_stride),
                                SRC2_IMM_3D(c),
                                xend, yend, zend);

    // mul_neg (back to original negatives)
    VPRO::DIM3::PROCESSING::mull_neg(lanes,
                                     DST_ADDR (base, 1, w, ch_stride),
                                     SRC1_ADDR(base, 1, w, ch_stride),
                                     SRC2_IMM_3D(-1),
                                     xend, yend, zend);

    // add 0.5 (sigmoid)
    VPRO::DIM3::PROCESSING::add(lanes,
                                DST_ADDR (base, 1, w, ch_stride),
                                SRC1_ADDR(base, 1, w, ch_stride),
                                SRC2_IMM_3D(float_to_fix(0.5, mem_input_fractional_bits)),
                                xend, yend, zend);
}

namespace REGION_5 {
    constexpr int pieces = 3;
    constexpr int static_fractional_bits = 18;
    constexpr int static_input_fractional_bits = 11;

    constexpr int32_t fixed32_to_fixed22_(int32_t fixed_32_param) {
        int32_t fixed_22_param = fixed_32_param & 0x3FFFFF;
        //Set sign for param
        if (fixed_32_param & 0x80000000) {
            fixed_22_param = int32_t(uint32_t(fixed_32_param) | 0xffc00000);
        }
        return fixed_22_param;
    }

    template<int N>
    struct Static_fraction_fix {
        constexpr Static_fraction_fix(const float input[N], const int fract_bits) {
            for (auto i = 0; i != N; ++i) {
                fixed_point[i] = fixed32_to_fixed22_(
                        int32_t(input[i] * (float) (1 << fract_bits)));  //(int32_t)((x) * (float)(1<<scale_para_imm))
                float_point[i] = input[i];
            }
        }

        constexpr Static_fraction_fix(const std::array<float, N> input, const int fract_bits) {
            for (auto i = 0; i != N; ++i) {
                fixed_point[i] = fixed32_to_fixed22_(
                        int32_t(input[i] * (float) (1 << fract_bits)));  //(int32_t)((x) * (float)(1<<scale_para_imm))
                float_point[i] = input[i];
            }
        }

        int32_t fixed_point[N]{};
        float float_point[N]{};
    };

    namespace THIRD_LINEAR_MIX {
        // input segment piece limits (using x-symmetric attributes of sigmoid)
        const auto x_min_fix = Static_fraction_fix<pieces>({0, 0.8, 6},
                                                           static_input_fractional_bits);
        const auto x_max_fix = Static_fraction_fix<pieces>({0.8, 6, 16},
                                                           static_input_fractional_bits);

        // regular polynom calc
        const auto a_fix = Static_fraction_fix<pieces>({0.23746810140951563, 0.004002586492382253, 0},
                                                       static_fractional_bits);
        const auto b_fix = Static_fraction_fix<pieces>({0.5018896947438222, -0.059155801998514175, 0},
                                                       static_fractional_bits);
        const auto c_fix = Static_fraction_fix<pieces>({0, 0.2955370569832206, 0},
                                                       static_fractional_bits);
        const auto d_fix = Static_fraction_fix<pieces>({0, 0.49193086199557196, 1},
                                                       static_fractional_bits);

        // speed up polynom calc by math reorder (reduce x*x calcs)
        const auto a_fix_fast = Static_fraction_fix<pieces>({0.23746810140951563, 0.004002586492382253, 0},
                                                            static_fractional_bits);
        const auto k_fix_fast = Static_fraction_fix<pieces>({0.5018896947438222, -4.926464600752526, 0},
                                                            static_input_fractional_bits);
        const auto e_fix_fast = Static_fraction_fix<pieces>({0, 0.004108092508414929, 0},
                                                            static_fractional_bits);
        const auto h_fix_fast = Static_fraction_fix<pieces>({0, 0.9907407266879265, 1},
                                                            static_fractional_bits);
    }
}

constexpr uint32_t get_block_size(uint32_t input_block_size, uint32_t max_block_size = 128) {
//    assert((input_block_size & 1) == 0); // dividable by 2
    if (input_block_size > max_block_size) {
        return get_block_size(input_block_size / 2, max_block_size);
    }
    return input_block_size;
}

/**
 * internal helper to call the vpro process of calculation of one batch of linear input data
 * uses severl regions of the RF
 */
inline void _sigmoid_medium_batch_calc(const uint32_t &size_of_batch,
                                       const uint32_t &unit_one, const uint32_t &symmetric_value,
                                       const uint32_t &RF_INPUT_ORG_BASE, const uint32_t &RF_ABS_BASE,
                                       const uint32_t &RF_FIRST_ORD_BASE, const uint32_t &RF_THIRD_ORD_BASE,
                                       const uint32_t &RF_NEG_BUFFER_BASE, const uint32_t &RF_X_DIFF_BASE,
                                       const uint32_t &RF_X_UPP_BASE, const uint32_t &RF_RESULT_BASE){
    // Set RF Result to 0
    VPRO::DIM3::PROCESSING::add(L0_1,
                                DST_ADDR(RF_RESULT_BASE, 0, 0, 1),
                                SRC1_IMM_3D(0),
                                SRC2_IMM_3D(0),
                                0, 0, size_of_batch - 1, false, true);

    // Symmetric Feature: calc absolutes
    VPRO::DIM3::PROCESSING::abs(L0_1,
                                DST_ADDR(RF_ABS_BASE, 0, 0, 1),
                                SRC1_ADDR(RF_INPUT_ORG_BASE, 0, 0, 1),
                                SRC2_ADDR(RF_INPUT_ORG_BASE, 0, 0, 1),
                                0, 0, size_of_batch - 1, false, true);

    VPRO::DIM3::PROCESSING::sub(L0_1,
                                DST_ADDR(RF_X_UPP_BASE, 0, 0, 1),
                                SRC1_IMM_3D((REGION_5::THIRD_LINEAR_MIX::x_min_fix.fixed_point[1])),
                                SRC2_ADDR(RF_ABS_BASE, 0, 0, 1),
                                0, 0, size_of_batch - 1, false, true);

    //-------------------------------------------first pieces---------------------------
    VPRO::DIM3::PROCESSING::mulh(L0_1,
                                 DST_ADDR(RF_FIRST_ORD_BASE, 0, 0, 1),
                                 SRC1_IMM_3D(REGION_5::THIRD_LINEAR_MIX::a_fix_fast.fixed_point[0]),
                                 SRC2_ADDR(RF_ABS_BASE, 0, 0, 1),
                                 0, 0, size_of_batch - 1, false, true);

    VPRO::DIM3::PROCESSING::add(L0_1,
                                DST_ADDR(RF_FIRST_ORD_BASE, 0, 0, 1),
                                SRC1_ADDR(RF_FIRST_ORD_BASE, 0, 0, 1),
                                SRC2_IMM_3D(REGION_5::THIRD_LINEAR_MIX::b_fix.fixed_point[0]),
                                0, 0, size_of_batch - 1, false, true);

    VPRO::DIM3::PROCESSING::mv_non_negative(L0_1,
                                            DST_ADDR(RF_FIRST_ORD_BASE, 0, 0, 1),
                                            SRC1_ADDR(RF_X_UPP_BASE, 0, 0, 1),
                                            SRC2_IMM_3D(0),
                                            0, 0, size_of_batch - 1, false, true);

    VPRO::DIM3::PROCESSING::add(L0_1,
                                DST_ADDR(RF_RESULT_BASE, 0, 0, 1),
                                SRC1_ADDR(RF_RESULT_BASE, 0, 0, 1),
                                SRC2_ADDR(RF_FIRST_ORD_BASE, 0, 0, 1),
                                0, 0, size_of_batch - 1, false, true);

    //-------------------------------------------second pieces---------------------------
    VPRO::DIM3::PROCESSING::add(L0_1,
                                DST_ADDR(RF_FIRST_ORD_BASE, 0, 0, 1),
                                SRC1_IMM_3D(REGION_5::THIRD_LINEAR_MIX::k_fix_fast.fixed_point[1]),
                                SRC2_ADDR(RF_ABS_BASE, 0, 0, 1),
                                0, 0, size_of_batch - 1, false, true);

    VPRO::DIM3::PROCESSING::mulh(L0_1,
                                 DST_ADDR(RF_THIRD_ORD_BASE, 0, 0, 1),
                                 SRC1_ADDR(RF_FIRST_ORD_BASE, 0, 0, 1),
                                 SRC2_ADDR(RF_FIRST_ORD_BASE, 0, 0, 1),
                                 0, 0, size_of_batch - 1, false, true);

    VPRO::DIM3::PROCESSING::mulh(L0_1,
                                 DST_ADDR(RF_THIRD_ORD_BASE, 0, 0, 1),
                                 SRC1_ADDR(RF_THIRD_ORD_BASE, 0, 0, 1),
                                 SRC2_ADDR(RF_FIRST_ORD_BASE, 0, 0, 1),
                                 0, 0, size_of_batch - 1, false, true);

    VPRO::DIM3::PROCESSING::mulh(L0_1,
                                 DST_ADDR(RF_FIRST_ORD_BASE, 0, 0, 1),
                                 SRC1_ADDR(RF_FIRST_ORD_BASE, 0, 0, 1),
                                 SRC2_IMM_3D(REGION_5::THIRD_LINEAR_MIX::e_fix_fast.fixed_point[1]),
                                 0, 0, size_of_batch - 1, false, true);

    VPRO::DIM3::PROCESSING::mulh(L0_1,
                                 DST_ADDR(RF_THIRD_ORD_BASE, 0, 0, 1),
                                 SRC1_ADDR(RF_THIRD_ORD_BASE, 0, 0, 1),
                                 SRC2_IMM_3D(REGION_5::THIRD_LINEAR_MIX::a_fix_fast.fixed_point[1]),
                                 0, 0, size_of_batch - 1, false, true);

    //-------------------------------------------------------------------------------------------
    VPRO::DIM3::PROCESSING::add(L0_1,
                                DST_ADDR(RF_FIRST_ORD_BASE, 0, 0, 1),
                                SRC1_ADDR(RF_FIRST_ORD_BASE, 0, 0, 1),
                                SRC2_ADDR(RF_THIRD_ORD_BASE, 0, 0, 1),
                                0, 0, size_of_batch - 1, false, true);

    VPRO::DIM3::PROCESSING::add(L0_1,
                                DST_ADDR(RF_FIRST_ORD_BASE, 0, 0, 1),
                                SRC1_IMM_3D(REGION_5::THIRD_LINEAR_MIX::h_fix_fast.fixed_point[1]),
                                SRC2_ADDR(RF_FIRST_ORD_BASE, 0, 0, 1),
                                0, 0, size_of_batch - 1, false, true);

    //------------------------------------------------------------------------------------
    VPRO::DIM3::PROCESSING::sub(L0_1,
                                DST_ADDR(RF_X_DIFF_BASE, 0, 0, 1),
                                SRC1_IMM_3D(REGION_5::THIRD_LINEAR_MIX::x_min_fix.fixed_point[1]),
                                SRC2_ADDR(RF_ABS_BASE, 0, 0, 1),
                                0, 0, size_of_batch - 1, false, true);

    VPRO::DIM3::PROCESSING::mv_negative(L0_1,
                                        DST_ADDR(RF_FIRST_ORD_BASE, 0, 0, 1),
                                        SRC1_ADDR(RF_X_DIFF_BASE, 0, 0, 1),
                                        SRC2_IMM_3D(0),
                                        0, 0, size_of_batch - 1, false, true);

    VPRO::DIM3::PROCESSING::sub(L0_1,
                                DST_ADDR(RF_X_UPP_BASE, 0, 0, 1),
                                SRC1_IMM_3D((REGION_5::THIRD_LINEAR_MIX::x_max_fix.fixed_point[1] -
                                             REGION_5::THIRD_LINEAR_MIX::x_min_fix.fixed_point[1])),
                                SRC2_ADDR(RF_X_DIFF_BASE, 0, 0, 1),
                                0, 0, size_of_batch - 1, false, true);

    VPRO::DIM3::PROCESSING::mv_non_negative(L0_1,
                                            DST_ADDR(RF_FIRST_ORD_BASE, 0, 0, 1),
                                            SRC1_ADDR(RF_X_UPP_BASE, 0, 0, 1),
                                            SRC2_IMM_3D(unit_one),
                                            0, 0, size_of_batch - 1, false, true);

    VPRO::DIM3::PROCESSING::add(L0_1,
                                DST_ADDR(RF_RESULT_BASE, 0, 0, 1),
                                SRC1_ADDR(RF_RESULT_BASE, 0, 0, 1),
                                SRC2_ADDR(RF_FIRST_ORD_BASE, 0, 0, 1),
                                0, 0, size_of_batch - 1, false, true);

    //------------------calc negative part----------------------
    VPRO::DIM3::PROCESSING::sub(L0_1,
                                DST_ADDR(RF_NEG_BUFFER_BASE, 0, 0, 1),
                                SRC1_ADDR(RF_RESULT_BASE, 0, 0, 1),
                                SRC2_IMM_3D(symmetric_value),
                                0, 0, size_of_batch - 1, false, true);

    VPRO::DIM3::PROCESSING::mv_negative(L0_1,
                                        DST_ADDR(RF_RESULT_BASE, 0, 0, 1),
                                        SRC1_ADDR(RF_INPUT_ORG_BASE, 0, 0, 1),
                                        SRC2_IMM_3D(0),
                                        0, 0, size_of_batch - 1, false, true);

    VPRO::DIM3::PROCESSING::mv_non_negative(L0_1,
                                            DST_ADDR(RF_NEG_BUFFER_BASE, 0, 0, 1),
                                            SRC1_ADDR(RF_INPUT_ORG_BASE, 0, 0, 1),
                                            SRC2_IMM_3D(0),
                                            0, 0, size_of_batch - 1, false, true);

    VPRO::DIM3::PROCESSING::add(L0_1,
                                DST_ADDR(RF_RESULT_BASE, 0, 0, 1),
                                SRC1_ADDR(RF_RESULT_BASE, 0, 0, 1),
                                SRC2_ADDR(RF_NEG_BUFFER_BASE, 0, 0, 1),
                                0, 0, size_of_batch - 1, false, true);
}

/**
 * assumes data in RF
 * stores data to LM
 */
inline void _sigmoid_medium(const BIF::LAYER &layer, const int32_t & lm_output_buffer,
                            const int16_t & mem_input_fractional_bits = 11, const int16_t & mem_store_fractional_bits = 11,
                            bool pooled2x2 = false) {

    // TODO: RF input is required to be in this fixed-point format.
    //  Change (by function parameter) not yet implemented: need shift in registerfile first to match format
    assert(mem_input_fractional_bits == REGION_5::static_input_fractional_bits);

    // FIXME lane_sync()?
    vpro_mul_h_bit_shift(REGION_5::static_input_fractional_bits);
    int32_t store_shift_right = REGION_5::static_fractional_bits - mem_store_fractional_bits;
//    int32_t load_shift_right = mem_input_fractional_bits - REGION_5::static_input_fractional_bits;
    uint32_t unit_one = 1 << REGION_5::static_fractional_bits; //Used to set the constant interval
    uint32_t symmetric_value = unit_one; // for tanh -> f(-x) = 0 - f(x) -> the symmetric_value set to zero; for sigmoid -> f(-x) = 1 - f(x) -> the symmetric_value set to one

    uint32_t input_count = layer.seg_out_w * layer.seg_out_h;
    if (pooled2x2){
        assert((layer.seg_out_w * layer.seg_out_h) % 4 == 0); // dimensions must be dividable by 2x2 (pooling)
        input_count = layer.seg_out_w/2 * layer.seg_out_h/2;
    }

    constexpr int max_batch_size = 128;

    uint32_t size_of_batch;
    if (input_count > max_batch_size && (input_count & 0b1) == 0)
        size_of_batch = get_block_size(input_count, max_batch_size);
    else
        size_of_batch = layer.seg_out_w;

    assert(input_count % size_of_batch == 0);   // input_count must be dividable by batch count
    uint32_t batches = input_count / size_of_batch;

    // define start addresses of segments in RF (intermediate data)
    const uint32_t RF_INPUT_ORG_BASE = 0;
    const uint32_t RF_ABS_BASE = 7 * size_of_batch; // first step -> symmetric requires abs input values
    const uint32_t RF_FIRST_ORD_BASE = 1 * size_of_batch;   // abs * a + b
    const uint32_t RF_THIRD_ORD_BASE = 3 * size_of_batch;
    const uint32_t RF_NEG_BUFFER_BASE = 2 * size_of_batch;
    const uint32_t RF_X_DIFF_BASE = 4 * size_of_batch;
    const uint32_t RF_X_UPP_BASE = 5 * size_of_batch;
    const uint32_t RF_RESULT_BASE = 6 * size_of_batch;

    uint32_t lm_output_buffer_l0 = lm_output_buffer + 0;
    uint32_t lm_output_buffer_l1 = lm_output_buffer + 1024;

    if (batches == 1){    // calc + store
        _sigmoid_medium_batch_calc(size_of_batch, unit_one, symmetric_value,
                                   RF_INPUT_ORG_BASE, RF_ABS_BASE, RF_FIRST_ORD_BASE, RF_THIRD_ORD_BASE,
                                   RF_NEG_BUFFER_BASE, RF_X_DIFF_BASE, RF_X_UPP_BASE, RF_RESULT_BASE);

        // STORE L0
        VPRO::DIM3::PROCESSING::shift_ar(L0,
                                         DST_ADDR(RF_RESULT_BASE, 0, 0, 1),
                                         SRC1_ADDR(RF_RESULT_BASE, 0, 0, 1),
                                         SRC2_IMM_3D(store_shift_right),
                                         0, 0, size_of_batch - 1, true, true);

        VPRO::DIM3::LOADSTORE::store(lm_output_buffer_l0,
                                     (2 * 0) * size_of_batch, 0, 0, 1,
                                     0, 0, size_of_batch - 1,
                                     L0);

        // STORE L1
        VPRO::DIM3::PROCESSING::shift_ar(L1,
                                         DST_ADDR(RF_RESULT_BASE, 0, 0, 1),
                                         SRC1_ADDR(RF_RESULT_BASE, 0, 0, 1),
                                         SRC2_IMM_3D(store_shift_right),
                                         0, 0, size_of_batch - 1, true, true);

        VPRO::DIM3::LOADSTORE::store(lm_output_buffer_l1,
                                     (2 * 0) * size_of_batch, 0, 0, 1,
                                     0, 0, size_of_batch - 1,
                                     L1);
    } else {    // store remaining batches in lm, calc, store, load, iterate
        // TODO shift rf fractions -> lm fractions
        // STORE input L0
        VPRO::DIM3::PROCESSING::shift_ar(L0,
                                         DST_ADDR(size_of_batch, 0, 0, 1),
                                         SRC1_ADDR(size_of_batch, 0, 0, 1),
                                         SRC2_IMM_3D(0),
                                         0, 0, ((batches-1) * size_of_batch) - 1, true, true);

        VPRO::DIM3::LOADSTORE::store(lm_output_buffer_l0,
                                     size_of_batch, 0, 0, 1,
                                     0, 0, ((batches-1) * size_of_batch) - 1,
                                     L0);

        // STORE input L1
        VPRO::DIM3::PROCESSING::shift_ar(L1,
                                         DST_ADDR(size_of_batch, 0, 0, 1),
                                         SRC1_ADDR(size_of_batch, 0, 0, 1),
                                         SRC2_IMM_3D(0),
                                         0, 0, ((batches-1) * size_of_batch) - 1, true, true);

        VPRO::DIM3::LOADSTORE::store(lm_output_buffer_l1,
                                     size_of_batch, 0, 0, 1,
                                     0, 0, ((batches-1) * size_of_batch) - 1,
                                     L1);

        // TODO update load shift lm fractions -> rf fractions
        uint32_t lm_batch_offset = 0;
        for (uint32_t batch = 0; batch < batches; ++batch) {
            if (batch > 0){  // Load previously stored Input to RF
                VPRO::DIM3::LOADSTORE::loads(lm_output_buffer_l0,
                                             lm_batch_offset, 0, 0, 1,
                                             0, 0, size_of_batch - 1, true);

                VPRO::DIM3::PROCESSING::shift_ar(L0_1,
                                                 DST_ADDR(RF_INPUT_ORG_BASE, 0, 0, 1),
                                                 SRC1_LS_3D,
                                                 SRC2_IMM_3D(0),
                                                 0, 0, size_of_batch - 1, false, true);

                VPRO::DIM3::LOADSTORE::loads(lm_output_buffer_l1,
                                             lm_batch_offset, 0, 0, 1,
                                             0, 0, size_of_batch - 1, true);

                VPRO::DIM3::PROCESSING::shift_ar(L1,
                                                 DST_ADDR(RF_INPUT_ORG_BASE, 0, 0, 1),
                                                 SRC1_LS_3D,
                                                 SRC2_IMM_3D(0),
                                                 0, 0, size_of_batch - 1, false, true);
            }
            _sigmoid_medium_batch_calc(size_of_batch, unit_one, symmetric_value,
                                       RF_INPUT_ORG_BASE, RF_ABS_BASE, RF_FIRST_ORD_BASE, RF_THIRD_ORD_BASE,
                                       RF_NEG_BUFFER_BASE, RF_X_DIFF_BASE, RF_X_UPP_BASE, RF_RESULT_BASE);

            // STORE L0
            VPRO::DIM3::PROCESSING::shift_ar(L0,
                                             DST_ADDR(RF_RESULT_BASE, 0, 0, 1),
                                             SRC1_ADDR(RF_RESULT_BASE, 0, 0, 1),
                                             SRC2_IMM_3D(store_shift_right),
                                             0, 0, size_of_batch - 1, true, true);

            VPRO::DIM3::LOADSTORE::store(lm_output_buffer_l0,
                                         lm_batch_offset, 0, 0, 1,
                                         0, 0, size_of_batch - 1,
                                         L0);

            // STORE L1
            VPRO::DIM3::PROCESSING::shift_ar(L1,
                                             DST_ADDR(RF_RESULT_BASE, 0, 0, 1),
                                             SRC1_ADDR(RF_RESULT_BASE, 0, 0, 1),
                                             SRC2_IMM_3D(store_shift_right),
                                             0, 0, size_of_batch - 1, true, true);

            VPRO::DIM3::LOADSTORE::store(lm_output_buffer_l1,
                                         lm_batch_offset, 0, 0, 1,
                                         0, 0, size_of_batch - 1,
                                         L1);
            lm_batch_offset += size_of_batch;
        }
    }
}

#endif //SIGMOID_PIECEWISE_APPROX_SIGMOID_H
