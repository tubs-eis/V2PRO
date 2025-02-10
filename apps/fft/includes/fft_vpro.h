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
// Created by gesper on 10.08.22.
//

#ifndef FFT_FFT_VPRO_H
#define FFT_FFT_VPRO_H

#include <cstdint>
#include <limits>
#include <stdio.h>
#include <array>
#include <assert.h>

#include "fft.h"

namespace FFT_VPRO {

    constexpr int fft_size = FFT::fft_size;
    constexpr std::size_t log2(std::size_t n) {
        return ( (n<2) ? 1 : 1+log2(n/2));
    }
    constexpr int fft_stages = log2(fft_size)-1;

    extern int __attribute__ ((section (".vpro"))) bit_reverse_indizes[fft_size];

    constexpr int fractional_bits = 15-fft_stages;

    extern int16_t input_r[fft_size];
    extern int16_t input_i[fft_size];

    void create_bit_reverse_indizes();
    void input_reorder();

    void init(int size = fft_size);

    void print_twiddles(int size = fft_size);

    void fft();
    void execute_stage(int nr, int twiddle_step);
    void _vpro_fft(uint32_t butterfly_wing_size, uint32_t yend, uint32_t beta, uint32_t item_offset, uint32_t twiddle_offset = 0, uint32_t ls_offset = 0);
    //_vpro_fft(uint32_t butterfly_wing_size, uint32_t yend, uint32_t beta, uint32_t offset, uint32_t ls_offset = 0);

//    extern int16_t *weights_r[fft_stages];
//    extern int16_t *weights_i[fft_stages];

    extern int16_t weights_r_data[fft_size/2];
    extern int16_t weights_i_data[fft_size/2];

    namespace CONST_HELPER{
        template<class Function, std::size_t... Indices>
        constexpr auto make_array_helper(Function f, std::index_sequence<Indices...>)
        -> std::array<typename std::result_of<Function(std::size_t)>::type, sizeof...(Indices)>
        {
            return {{ f(Indices)... }};
        }

        template<int N, class Function>
        constexpr auto make_array(Function f)
        -> std::array<typename std::result_of<Function(std::size_t)>::type, N>
        {
            return make_array_helper(f, std::make_index_sequence<N>{});
        }

        constexpr double lm_twiddle_i_address(int stage) {
            return 0 + 4 * fft_size + (1 << stage);
        }
    }

    // LM Layout

    constexpr uint32_t  LM_INPUT_R = 0 * fft_size;
    constexpr uint32_t  LM_INPUT_I = 1 * fft_size;

    constexpr uint32_t  LM_TMP_R = 2 * fft_size;
    constexpr uint32_t  LM_TMP_I = 3 * fft_size;

    constexpr uint32_t LM_TWIDDLE_R = 4 * fft_size;
    constexpr uint32_t LM_TWIDDLE_I = 4 * fft_size + (1 << fft_stages); //CONST_HELPER::make_array<fft_stages>(CONST_HELPER::lm_twiddle_i_address);

    constexpr uint32_t LM_TWIDDLE_32_R = 4 * fft_size + 2*(1 << fft_stages);
    constexpr uint32_t LM_TWIDDLE_32_I = 4 * fft_size + 2*(1 << fft_stages)+(1 << fft_stages)/32; //CONST_HELPER::make_array<fft_stages>(CONST_HELPER::lm_twiddle_i_address);

    // maximum entry
    static_assert( 4 * fft_size + 2*(1 << fft_stages)+(1 << fft_stages)/32*2 < 8192);

    // RF Layout

    constexpr uint32_t RF_I = 0 * fft_size;
    constexpr uint32_t RF_R = 1 * fft_size;

    constexpr uint32_t RF_TWIDDLE_R = 1024 - (1 << fft_stages);
    constexpr uint32_t RF_TWIDDLE_I = 1024 - 2*(1 << fft_stages);

    template<typename T>
    T reverse(T n, std::size_t b = sizeof(T) * 8) {
        assert(b <= std::numeric_limits<T>::digits);
        T rv = 0;
        for (std::size_t i = 0; i < b; ++i, n >>= 1) {
            rv = (rv << 1) | (n & 0x01);
        }
        return rv;
    }

};

#endif //FFT_FFT_VPRO_H
