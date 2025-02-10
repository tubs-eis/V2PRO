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

#include "fft.h"
#include <cmath>
#include <vpro.h>

using namespace FFT;

int32_t twiddle_i(int s, int i){
    return  int32_t(cimag(FFT::twiddle(s, i)) * pow(2., fractional_bits));
}
int32_t twiddle_r(int s, int i){
    return  int32_t(creal(FFT::twiddle(s, i)) * pow(2., fractional_bits));
}

void FFT::fft_i(){
    int32_t buf_r[fft_size]{0};
    int32_t buf_i[fft_size]{0};
    int32_t out_r[fft_size]{0};
    int32_t out_i[fft_size]{0};

    for (int i = 0; i < fft_size/2; ++i) {
        buf_r[i] = 1;
        out_r[i] = 1;
    }

    for (int i = 0; i < fft_size; ++i) {
        buf_r[i] *= pow(2, fractional_bits);
        buf_i[i] *= pow(2, fractional_bits);
        out_r[i] *= pow(2, fractional_bits);
        out_i[i] *= pow(2, fractional_bits);
    }

    show("\nData Quant: ", buf_r, buf_i, fractional_bits);
    _fft(buf_r, buf_i, out_r, out_i, fft_size, 1);
    show("\nFFT Quant: ", buf_r, buf_i, fractional_bits);
}

void FFT::_fft(int32_t buf_r[], int32_t buf_i[], int32_t out_r[], int32_t out_i[], int n, int step){
    if (step < n) {
        _fft(out_r, out_i, buf_r, buf_i, n, step * 2);
        _fft(out_r + step, out_i + step, buf_r + step, buf_i + step, n, step * 2);

        // step is 1, 2, 4, 8, 16, 32, .. n/2.
        for (int i = 0; i < n; i += 2 * step) {
            int32_t t_r = twiddle_r(n, i) * out_r[i + step] - twiddle_i(n, i) * out_i[i + step];
            int32_t t_i = twiddle_r(n, i) * out_i[i + step] + twiddle_i(n, i) * out_r[i + step];

            t_r >>= 14;
            t_i >>= 14;

            buf_r[i / 2]     = out_r[i] + t_r;
            buf_i[i / 2]     = out_i[i] + t_i;

            buf_r[(i + n)/2] = out_r[i] - t_r;
            buf_i[(i + n)/2] = out_i[i] - t_i;
        }
    }
}

void FFT::show(const char * s, const int32_t buf_r[], const int32_t buf_i[], int fractional_bits){
    printf("%s", s);
    for (int i = 0; i < fft_size; i++)
        if (!buf_i[i])
            printf("%g ", buf_r[i]/pow(2., fractional_bits));
        else
            printf("(%g, %g) ", buf_r[i]/pow(2., fractional_bits), buf_i[i]/pow(2., fractional_bits));
}


