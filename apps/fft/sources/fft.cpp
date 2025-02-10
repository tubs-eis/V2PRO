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
#include <math.h>
#include <vpro.h>


using namespace FFT;

_Complex double FFT::twiddle(int s, int i){
    return cexp(-I * PI * i / s);
}

void FFT::show(const char * s, _Complex double buf[], int size) {
    printf("%s", s);
    for (int i = 0; i < size; i++)
        if (!cimag(buf[i]))
            printf("%g ", creal(buf[i]));
        else
            printf("(%g, %g) ", creal(buf[i]), cimag(buf[i]));
}

_Complex double *FFT::fft(){
    auto *buf = new _Complex double[fft_size];
    _Complex double out[fft_size];
    for (int i = 0; i < fft_size; ++i) {
        if (i < fft_size / 2) {
            buf[i] = 1;
            out[i] = 1;
        } else {
            buf[i] = 0;
            out[i] = 0;
        }
    }
    show("Data: ", buf, fft_size);
    _fft(buf, out, fft_size, 1);
    show("\nFFT : ", buf, fft_size);

    return buf;
}

void FFT::_fft(_Complex double buf[], _Complex double out[], int n, int step){
    //
    // in: 0...511 [real, imag]
    // out: 512...1023 [real, imag]
    //
    if (step < n) {
        _fft(out, buf, n, step * 2);
        _fft(out + step, buf + step, n, step * 2);

        for (int i = 0; i < n; i += 2 * step) {
            _Complex double t = twiddle(n, i) * out[i + step];
            buf[i / 2]     = out[i] + t;
            buf[(i + n)/2] = out[i] - t;
        }
    }
}

void FFT::init(int size){
//    double imin = 100, imax = -100;
//    double rmin = 100, rmax = -100;
//    for(const auto& ts: twiddles){
//        for(auto t: ts.second){
//            //printf("Twiddle: %lf + %lfi\n", cimag(t), creal(t));
//            imin = std::min(imin, cimag(t));
//            rmin = std::min(rmin, creal(t));
//
//            imax = std::max(imax, cimag(t));
//            rmax = std::max(rmax, creal(t));
//        }
//    }
//    printf("Min I Twiddle: %lf\n", imin);
//    printf("Min R Twiddle: %lf\n", rmin);
//    printf("Max I Twiddle: %lf\n", imax);
//    printf("Max R Twiddle: %lf\n", rmax);
}


