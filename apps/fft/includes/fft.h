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
// Created by gesper on 08.08.22.
//

#ifndef FIR_FIR_H
#define FIR_FIR_H

#include <complex.h>
#include <cstdint>
#include <map>
#include <vector>

namespace FFT {

    constexpr double PI = M_PI;
    constexpr int fractional_bits = 14;
    constexpr int fft_size = 128;
    constexpr std::size_t log2(std::size_t n) {
        return ( (n<2) ? 1 : 1+log2(n/2));
    }
    constexpr int fft_stages = log2(fft_size);

    void init(int size = fft_size);

//    void fft();
    _Complex double *fft();

    _Complex double twiddle(int s, int i);

    void _fft(_Complex double buf[], _Complex double out[], int n, int step);

    void show(const char *s, _Complex double buf[], int size);

    void fft_i();

    void _fft(int32_t buf_r[], int32_t buf_i[], int32_t out_r[], int32_t out_i[], int n, int step);

    void show(const char *s, const int32_t buf_r[], const int32_t buf_i[], int fractional_bits);
}
#endif //FIR_FIR_H
