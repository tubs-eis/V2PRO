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
#ifndef DEFORM_KERNEL_H
#define DEFORM_KERNEL_H

#include "bif.h"
#include "vpro_functions.h"
#include <vpro.h>
#include "eisv.h"

// TODO(Jasper): Remove (Debug)
#include <core_wrapper.h>

// Implicitely uses three lanes (1 LS, 2 ALU), caller has to ensure no other operations are scheduled to those.
// Implements the functionality of `DeformStageExecutor::process_block_hw_opt` in the thesis implementation.
inline void _deform_block_4x4(const BIF::LAYER &layer, const uint16_t input_buffer, const uint16_t offset_buffer, const uint16_t output_buffer, const uint16_t static_offset_buffer) {
    //core_->pauseSim();

    // Inputs:
    // TODO(Jasper):
    //   Figure out how to pass which paramters, treat offsets, constants, sizes differently.
    // TODO(Jasper):
    //   Where to do block address offset calculation, for now assume input addresses are already block local.

    // Constant (in this implementation)
    constexpr uint16_t chunk_width = 8;
    constexpr uint16_t block_width = 4;
    constexpr uint16_t block_height = 4;
    constexpr uint16_t kernel_length = 9;
    constexpr uint16_t frac_bits = 8; // TODO(Jasper): Use one of the shift, parameters


    // Variables by categories:
    // Layer paramters
    uint16_t max_offset_x = layer.deform_max_offset_x;
    //uint16_t max_offset_y = layer.deform_max_offset_y;

    // LM addresses (depend on block):
    uint16_t input_lm = input_buffer;
    uint16_t offset_lm = offset_buffer;
    uint16_t output_lm = output_buffer;
    //uint16_t input_lm_rows; // Needed for input reuse
    uint16_t static_offsets_lm = static_offset_buffer;

    // RF Memory Layout (depends on input dimensions, constant for now):
    constexpr uint16_t block_output_size = block_width * block_height * kernel_length;

    constexpr uint16_t x_offset_l0 = 0 * block_output_size;
    constexpr uint16_t result_l0 = 0 * block_output_size; // Overlaps x_offset_l0
    constexpr uint16_t x_offset_inv_l0 = 1 * block_output_size;
    constexpr uint16_t masks_l0 = 1 * block_output_size; // Overlaps x_offset_inv_l0
    constexpr uint16_t y_offset_l0 = 2 * block_output_size;
    constexpr uint16_t y_offset_inv_l0 = 2 * block_output_size; // Overlaps y_offset_l0

    constexpr uint16_t bilinear_factors_l0 = 3 * block_output_size;

    constexpr uint16_t addresses_l1 = 0 * block_output_size;
    constexpr uint16_t masks_l1 = 0 * block_output_size; // Overlaps addresses_l1
    //constexpr uint16_t addresses_tmp_l1 = 1 * block_output_size;
    constexpr uint16_t y_offset_l1 = 2 * block_output_size;
    // uint16_t input_padding_l1; // Needed for input reuse

    // Dynamic values
    // uint16_t buffer_y_offset; // Needed for input reuse

    // Load x & 0xff to L0 and x >> frac_bits to L1
    VPRO::DIM2::PROCESSING::and_(L0, DST_ADDR(x_offset_l0, 1, kernel_length), SRC_LS_2D, SRC_IMM_2D((1 << frac_bits) - 1), kernel_length - 1, block_height * block_width - 1);
    VPRO::DIM2::PROCESSING::shift_ar(L1, DST_ADDR(addresses_l1, 1, kernel_length), SRC_LS_2D, SRC_IMM_2D(frac_bits), kernel_length - 1, block_height * block_width - 1);
    for (uint16_t local_y = 0; local_y < block_height; local_y++) { // TODO(Jasper): If block_height is constant (which it is in the thesis implementation) this type of loop could be unrolled.
        VPRO::DIM2::LOADSTORE::loads(offset_lm + local_y * chunk_width * 3 * kernel_length + chunk_width, 0, chunk_width * 2, 1, kernel_length - 1, block_width - 1);
    }

    // Load y & 0xff to L0 and y >> frac_bits to L1
    VPRO::DIM2::PROCESSING::and_(L0, DST_ADDR(y_offset_l0, 1, kernel_length), SRC_LS_2D, SRC_IMM_2D((1 << frac_bits) - 1), kernel_length - 1, block_height * block_width - 1);
    VPRO::DIM2::PROCESSING::shift_ar(L1, DST_ADDR(y_offset_l1, 1, kernel_length), SRC_LS_2D, SRC_IMM_2D(frac_bits), kernel_length - 1, block_height * block_width - 1);
    for (uint16_t local_y = 0; local_y < block_height; local_y++) {
        VPRO::DIM2::LOADSTORE::loads(offset_lm + local_y * chunk_width * 3 * kernel_length, 0, chunk_width * 2, 1, kernel_length - 1, block_width - 1);
    }

    // TODO(Jasper) Investigate if interleaving the L0 and L1 operations results in a speed up?

    // Calculate bilinear factors on L0
    VPRO::DIM2::PROCESSING::sub(L0, DST_ADDR(x_offset_inv_l0, 1, kernel_length), SRC1_ADDR(x_offset_l0, 1, kernel_length), SRC2_IMM_2D(1 << frac_bits), kernel_length - 1, block_height * block_width - 1);
    VPRO::DIM2::PROCESSING::mulh(L0, DST_ADDR(bilinear_factors_l0 + 2, 4, 4 * kernel_length), SRC1_ADDR(x_offset_inv_l0, 1, kernel_length), SRC2_ADDR(y_offset_l0, 1, kernel_length), kernel_length - 1, block_height * block_width - 1);
    VPRO::DIM2::PROCESSING::mulh(L0, DST_ADDR(bilinear_factors_l0 + 3, 4, 4 * kernel_length), SRC1_ADDR(x_offset_l0, 1, kernel_length), SRC2_ADDR(y_offset_l0, 1, kernel_length), kernel_length - 1, block_height * block_width - 1);
    VPRO::DIM2::PROCESSING::sub(L0, DST_ADDR(y_offset_inv_l0, 1, kernel_length), SRC1_ADDR(y_offset_l0, 1, kernel_length), SRC2_IMM_2D(1 << frac_bits), kernel_length - 1, block_height * block_width - 1);
    VPRO::DIM2::PROCESSING::mulh(L0, DST_ADDR(bilinear_factors_l0 + 0, 4, 4 * kernel_length), SRC1_ADDR(x_offset_inv_l0, 1, kernel_length), SRC2_ADDR(y_offset_inv_l0, 1, kernel_length), kernel_length - 1, block_height * block_width - 1);
    VPRO::DIM2::PROCESSING::mulh(L0, DST_ADDR(bilinear_factors_l0 + 1, 4, 4 * kernel_length), SRC1_ADDR(x_offset_l0, 1, kernel_length), SRC2_ADDR(y_offset_inv_l0, 1, kernel_length), kernel_length - 1, block_height * block_width - 1);

    // Calculate addresses on L1
    VPRO::DIM3::PROCESSING::max(L1, DST_ADDR(addresses_l1, 1, kernel_length, 0), SRC_IMM_3D(-(max_offset_x - 1)), SRC2_ADDR(addresses_l1, 1, kernel_length, 0), kernel_length - 1, block_height * block_width - 1, 0);
    VPRO::DIM2::PROCESSING::min(L1, DST_ADDR(addresses_l1, 1, kernel_length), SRC_IMM_2D(max_offset_x - 2), SRC2_ADDR(addresses_l1, 1, kernel_length), kernel_length - 1, block_height * block_width - 1);

    VPRO::DIM3::PROCESSING::max(L1, DST_ADDR(y_offset_l1, 1, kernel_length, 0), SRC_IMM_3D(-(max_offset_x - 1)), SRC2_ADDR(y_offset_l1, 1, kernel_length, 0), kernel_length - 1, block_height * block_width - 1, 0);
    VPRO::DIM2::PROCESSING::min(L1, DST_ADDR(y_offset_l1, 1, kernel_length), SRC_IMM_2D(max_offset_x - 2), SRC2_ADDR(y_offset_l1, 1, kernel_length), kernel_length - 1, block_height * block_width - 1);

    VPRO::DIM2::PROCESSING::mull(L1, DST_ADDR(y_offset_l1, 1, kernel_length), SRC_IMM_2D(chunk_width + 2 * max_offset_x), SRC2_ADDR(y_offset_l1, 1, kernel_length), kernel_length - 1, block_height * block_width - 1);
    VPRO::DIM2::PROCESSING::add(L1, DST_ADDR(addresses_l1, 1, kernel_length), SRC1_ADDR(y_offset_l1, 1, kernel_length), SRC2_ADDR(addresses_l1, 1, kernel_length), kernel_length - 1, block_height * block_width - 1);

    VPRO::DIM2::PROCESSING::add(L1, DST_ADDR(addresses_l1, 1, kernel_length), SRC_LS_2D, SRC2_ADDR(addresses_l1, 1, kernel_length), kernel_length - 1, block_height * block_width - 1);
    for (uint16_t local_y = 0; local_y < block_height; local_y++) {
        VPRO::DIM2::LOADSTORE::loads(static_offsets_lm + local_y * kernel_length * chunk_width, 0, 1, kernel_length, kernel_length - 1, block_width - 1);
    }

    // Needed for input reuse
    // if(buffer_y_offset != 0) {
    //     // Adjust addresses to index into ringbuffer
    //     // y = y + buffer_y_offset % buffer_height

    //     // 1. add buffer offset * width to all addresses
    //     VPRO::DIM2::PROCESSING::add(L1, DST_ADDR(addresses_l1, 1, kernel_length), SRC_IMM_2D(buffer_y_offset * chunk_width), SRC2_ADDR(addresses_l1, 1, kernel_length), kernel_length - 1, block_height * block_width - 1);
    //     // 2. subtract total buffer size, store result into tmp array and update flags
    //     VPRO::DIM2::PROCESSING::sub(L1, DST_ADDR(addresses_tmp_l1, 1, kernel_length), SRC_IMM_2D((input_lm_rows + 1) * (chunk_width + 2 * max_offset_x)), SRC2_ADDR(addresses_l1, 1, kernel_length), kernel_length - 1, block_height * block_width - 1, false, true);
    //     // 3. CMove only positive results back
    //     VPRO::DIM2::PROCESSING::mv_non_negative(L1, DST_ADDR(addresses_l1, 1, kernel_length), SRC1_ADDR(addresses_tmp_l1, 1, kernel_length), SRC2_ADDR(addresses_tmp_l1, 1, kernel_length), kernel_length - 1, block_height * block_width - 1);
    // }

    // Compute bilinear filtering
    for (uint16_t local_y = 0; local_y < block_height; local_y++) {
        uint16_t row_addresses_l1 = addresses_l1 + local_y * kernel_length * block_width;
        uint16_t row_result_l0 = result_l0 + local_y * kernel_length * block_width;
        uint16_t row_bilinear_factors_l0 = bilinear_factors_l0 + local_y * 4 * kernel_length * block_width;

        VPRO::DIM3::PROCESSING::add(L1, DST_ADDR(row_addresses_l1, 0, 1, kernel_length), SRC_IMM_3D(0), SRC2_ADDR(row_addresses_l1, 0, 1, kernel_length), 3, kernel_length - 1, block_width - 1, true);
        VPRO::DIM3::LOADSTORE::dynamic_loads(L1, input_lm, 1, chunk_width + 2 * max_offset_x, 0, 1, 1, block_width * kernel_length - 1);
        VPRO::DIM3::PROCESSING::mach_pre(L0, DST_ADDR(row_result_l0, 0, 0, 1), SRC_LS_3D, SRC2_ADDR(row_bilinear_factors_l0, 1, 0, 4), 3, 0, block_width * kernel_length - 1, false, false, local_y == block_height - 1);
    }

    //TODO(Jasper): This could be computed on offsetconv, to save duplicate computations

    // 1. LOAD(LS) -> L0(shift_ar) -> L1(add)
    // 2. L1(min) -> L0(max)
    // 3. L0 mulh -> STORE(LS)

    // Load mask values, divide by 4 (>> 2) and add 1/2
    // shift_ar_chain_imm(ChainSrc::LoadStore, 2, result_bias_regs, L0, true);
    // add_chain_imm(ChainSrc::Arithmetic, (1 << frac_bits) >> 1, result_bias_regs, L1);
    VPRO::DIM3::PROCESSING::shift_ar(L0, DST_ADDR(masks_l0, 1, kernel_length, kernel_length * block_width), SRC_LS_3D, SRC2_IMM_3D(2), kernel_length - 1, block_width - 1, block_height - 1, true);
    VPRO::DIM3::PROCESSING::add(L1, DST_ADDR(masks_l1, 1, kernel_length, kernel_length * block_width), SRC_CHAINING_RIGHT_3D, SRC2_IMM_3D((1 << frac_bits) >> 1), kernel_length - 1, block_width - 1, block_height - 1);
    for (uint16_t local_y = 0; local_y < block_height; local_y++) {
        VPRO::DIM2::LOADSTORE::loads(offset_lm + local_y * chunk_width * 3 * kernel_length + chunk_width * 2 * kernel_length, 0, chunk_width, 1, kernel_length - 1, block_width - 1);
    }

    // Clamp to [0,1]
    // min_imm(result_bias_regs, 1 << frac_bits, result_bias_regs, L1, true);
    // max_chain_imm(ChainSrc::Arithmetic, 0, result_bias_regs, L0, true);
    VPRO::DIM3::PROCESSING::min(L1, DST_ADDR(masks_l1, 1, kernel_length, kernel_length * block_width), SRC1_ADDR(masks_l1, 1, kernel_length, kernel_length * block_width), SRC2_IMM_3D(1 << frac_bits), kernel_length - 1, block_width - 1, block_height - 1, true);
    VPRO::DIM3::PROCESSING::max(L0, DST_ADDR(masks_l0, 1, kernel_length, kernel_length * block_width), SRC_CHAINING_RIGHT_3D, SRC2_IMM_3D(0), kernel_length - 1, block_width - 1, block_height - 1);

    // Multiply with mask and store results to LM
    VPRO::DIM2::PROCESSING::mulh(L0, DST_DISCARD_2D, SRC1_ADDR(masks_l0, 1, kernel_length), SRC2_ADDR(result_l0, 1, kernel_length), kernel_length - 1, block_height * block_width - 1, true);
    for (size_t local_y = 0; local_y < block_height; local_y++) {
        VPRO::DIM2::LOADSTORE::store(output_lm + local_y * chunk_width * kernel_length, 0, 1, 0, block_width * kernel_length - 1, 0, L0);
    }
}

#endif
