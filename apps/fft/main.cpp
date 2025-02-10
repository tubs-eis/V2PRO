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
// ########################################################
// # Tutorial for the VPRO Instruction usage              #
// #                                                      #
// # Sven Gesper, EIS, Tu Braunschweig, 2022              #
// ########################################################

#include <cstdint>
#include <vpro.h>
#include <eisv.h>
#include <cstdio>      /* printf, scanf, puts, NULL */
#include <random>     /* srand, rand */
#include <ctime>       /* time */
#include <limits>

#include "fft.h"
#include "fft_vpro.h"

/**
 * Main
 */
int main(int argc, char *argv[]) {
    sim_init(main, argc, argv);
    sim_printf("FIR App\n");
    sim_printf("Clusters: %i, units: %i, lanes: %i\n", VPRO_CFG::CLUSTERS, VPRO_CFG::UNITS, VPRO_CFG::LANES);

    FFT::init();
    _Complex double * buf = FFT::fft();

//    FFT::fft_i(); // quantized

    // print the const arrays
//    for (int i = 8; i <= 1024; i*=2) {
//        FFT_VPRO::init(i);
//        FFT_VPRO::print_twiddles(i);
//    }

    FFT_VPRO::init();
    FFT_VPRO::fft();

    double max_r_diff = 0;
    double max_i_diff = 0;

    for (int i = 0; i < FFT_VPRO::fft_size; i++){
        double r_vpro = FFT_VPRO::input_r[i] / pow(2., FFT_VPRO::fractional_bits);
        double i_vpro = FFT_VPRO::input_i[i] / pow(2., FFT_VPRO::fractional_bits);

        double r_host = creal(buf[i]);
        double i_host = cimag(buf[i]);

        double r_diff = abs(r_host-r_vpro);
        double i_diff = abs(i_host-i_vpro);

        if (r_diff > max_r_diff)
            max_r_diff = r_diff;

        if (i_diff > max_i_diff)
            max_i_diff = i_diff;
    }
    printf_warning("MAX R_DIFF: %f\n", max_r_diff);
    printf_warning("MAX I_DIFF: %f\n", max_i_diff);

    for (int i = 0; i < FFT_VPRO::fft_size; i++) {
        printf("%08X\n", FFT_VPRO::input_r[i]);
        printf("%08X\n", FFT_VPRO::input_i[i]);
    }
    sim_stop();
    return 0;
}
