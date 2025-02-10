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

#include "fft_vpro.h"
#include <cstring>
#include <cmath>
#include <complex.h>
#include <vpro.h>
#include <eisv.h>

using namespace FFT_VPRO;

int __attribute__ ((section (".vpro"))) FFT_VPRO::bit_reverse_indizes[fft_size];

int16_t __attribute__ ((section (".vpro"))) FFT_VPRO::input_r[fft_size];
int16_t __attribute__ ((section (".vpro"))) FFT_VPRO::input_i[fft_size];

//int16_t __attribute__ ((section (".vpro"))) *FFT_VPRO::weights_r[fft_stages];
//int16_t __attribute__ ((section (".vpro"))) *FFT_VPRO::weights_i[fft_stages];

int16_t __attribute__ ((section (".vpro"))) FFT_VPRO::weights_r_data[fft_size / 2];
int16_t __attribute__ ((section (".vpro"))) FFT_VPRO::weights_i_data[fft_size / 2];

void FFT_VPRO::create_bit_reverse_indizes() {
    for (int i = 0; i < fft_size; ++i) {
        bit_reverse_indizes[i] = reverse(i, fft_stages);
//        printf("bit_reverse_indizes[%d] = %d\n", i, bit_reverse_indizes[i] );
    }
}

void FFT_VPRO::input_reorder() {
//    // Reverse indizes on vpro. Requires change of load. Address will be in bit reverse order.
//    for (int c = 0; c < VPRO_CFG::CLUSTERS; ++c) {
//        for (int u = 0; u < VPRO_CFG::UNITS; ++u) {
//            dma_ext1D_to_loc1D(c, intptr_t(bit_reverse_indizes), LM_BASE_VU(u) + 0, fft_size);
//        }
//    }
//    dma_wait_to_finish(0xffffffff);
//      // TODO: continue

    // RISC will reorder input data.
    int16_t tmp1[fft_size];
    int16_t tmp2[fft_size];
    for (int i = 0; i < fft_size; ++i) {
        tmp1[i] = input_r[bit_reverse_indizes[i]];
        tmp2[i] = input_i[bit_reverse_indizes[i]];
    }
    std::memcpy(input_r, tmp1, fft_size * sizeof(int16_t));
    std::memcpy(input_i, tmp2, fft_size * sizeof(int16_t));
}

void FFT_VPRO::init(int size) {
    create_bit_reverse_indizes();

    if (size / 2 == 4) {
        const int16_t weights_r_data_const[] = {
                256, 181, 0, -181};
        std::memcpy(weights_r_data, weights_r_data_const, size);

        const int16_t weights_i_data_const[] = {
                0, -181, -256, -181};
        std::memcpy(weights_i_data, weights_i_data_const, size);
    } else if (size / 2  == 8) {
        const int16_t weights_r_data_const[] = {
                256, 236, 181, 97, 0, -97, -181, -236};
        std::memcpy(weights_r_data, weights_r_data_const, size);

        const int16_t weights_i_data_const[] = {
                0, -97, -181, -236, -256, -236, -181, -97};
        std::memcpy(weights_i_data, weights_i_data_const, size);
    } else if (size / 2  == 16) {
        const int16_t weights_r_data_const[] = {
                256, 251, 236, 212, 181, 142, 97, 49, 0, -49, -97, -142, -181, -212, -236, -251};
        std::memcpy(weights_r_data, weights_r_data_const, size);

        const int16_t weights_i_data_const[] = {
                0, -49, -97, -142, -181, -212, -236, -251, -256, -251, -236, -212, -181, -142, -97, -49};
        std::memcpy(weights_i_data, weights_i_data_const, size);
    } else if (size / 2  == 32) {
        const int16_t weights_r_data_const[] = {
                256, 254, 251, 244, 236, 225, 212, 197, 181, 162, 142, 120, 97, 74, 49, 25,
                0, -25, -49, -74, -97, -120, -142, -162, -181, -197, -212, -225, -236, -244, -251, -254};
        std::memcpy(weights_r_data, weights_r_data_const, size);

        const int16_t weights_i_data_const[] = {
                0, -25, -49, -74, -97, -120, -142, -162, -181, -197, -212, -225, -236, -244, -251, -254,
                -256, -254, -251, -244, -236, -225, -212, -197, -181, -162, -142, -120, -97, -74, -49, -25};
        std::memcpy(weights_i_data, weights_i_data_const, size);
    } else if (size / 2  == 64) {
        const int16_t weights_r_data_const[] = {
                256, 255, 254, 253, 251, 248, 244, 241, 236, 231, 225, 219, 212, 205, 197, 189,
                181, 171, 162, 152, 142, 131, 120, 109, 97, 86, 74, 62, 49, 37, 25, 12,
                0, -12, -25, -37, -49, -62, -74, -86, -97, -109, -120, -131, -142, -152, -162, -171,
                -181, -189, -197, -205, -212, -219, -225, -231, -236, -241, -244, -248, -251, -253, -254, -255};
        std::memcpy(weights_r_data, weights_r_data_const, size);

        const int16_t weights_i_data_const[] = {
                0, -12, -25, -37, -49, -62, -74, -86, -97, -109, -120, -131, -142, -152, -162, -171,
                -181, -189, -197, -205, -212, -219, -225, -231, -236, -241, -244, -248, -251, -253, -254, -255,
                -256, -255, -254, -253, -251, -248, -244, -241, -236, -231, -225, -219, -212, -205, -197, -189,
                -181, -171, -162, -152, -142, -131, -120, -109, -97, -86, -74, -62, -49, -37, -25, -12};
        std::memcpy(weights_i_data, weights_i_data_const, size);
    } else if (size / 2  == 128) {
        const int16_t weights_r_data_const[] = {
                -256, -255, -255, -255, -254, -254, -253, -252, -251, -249, -248, -246, -244, -243, -241, -238,
                -236, -234, -231, -228, -225, -222, -219, -216, -212, -209, -205, -201, -197, -193, -189, -185,
                -181, -176, -171, -167, -162, -157, -152, -147, -142, -136, -131, -126, -120, -115, -109, -103,
                -97, -92, -86, -80, -74, -68, -62, -56, -49, -43, -37, -31, -25, -18, -12, -6,
                0, -6, -12, -18, -25, -31, -37, -43, -49, -56, -62, -68, -74, -80, -86, -92,
                -97, -103, -109, -115, -120, -126, -131, -136, -142, -147, -152, -157, -162, -167, -171, -176,
                -181, -185, -189, -193, -197, -201, -205, -209, -212, -216, -219, -222, -225, -228, -231, -234,
                -236, -238, -241, -243, -244, -246, -248, -249, -251, -252, -253, -254, -254, -255, -255, -255};
        std::memcpy(weights_r_data, weights_r_data_const, size);

        const int16_t weights_i_data_const[] = {
                0, -6, -12, -18, -25, -31, -37, -43, -49, -56, -62, -68, -74, -80, -86, -92,
                -97, -103, -109, -115, -120, -126, -131, -136, -142, -147, -152, -157, -162, -167, -171, -176,
                -181, -185, -189, -193, -197, -201, -205, -209, -212, -216, -219, -222, -225, -228, -231, -234,
                -236, -238, -241, -243, -244, -246, -248, -249, -251, -252, -253, -254, -254, -255, -255, -255,
                -256, -255, -255, -255, -254, -254, -253, -252, -251, -249, -248, -246, -244, -243, -241, -238,
                -236, -234, -231, -228, -225, -222, -219, -216, -212, -209, -205, -201, -197, -193, -189, -185,
                -181, -176, -171, -167, -162, -157, -152, -147, -142, -136, -131, -126, -120, -115, -109, -103,
                -97, -92, -86, -80, -74, -68, -62, -56, -49, -43, -37, -31, -25, -18, -12, -6};
        std::memcpy(weights_i_data, weights_i_data_const, size);
    } else if (size / 2  == 256) {
        const int16_t weights_r_data_const[] = {
                -181, -183, -185, -187, -189, -191, -193, -195, -197, -199, -201, -203, -205, -207, -209, -211,
                -212, -214, -216, -217, -219, -221, -222, -224, -225, -227, -228, -230, -231, -232, -234, -235,
                -236, -237, -238, -239, -241, -242, -243, -244, -244, -245, -246, -247, -248, -249, -249, -250,
                -251, -251, -252, -252, -253, -253, -254, -254, -254, -255, -255, -255, -255, -255, -255, -255,
                -256, -255, -255, -255, -255, -255, -255, -255, -254, -254, -254, -253, -253, -252, -252, -251,
                -251, -250, -249, -249, -248, -247, -246, -245, -244, -244, -243, -242, -241, -239, -238, -237,
                -236, -235, -234, -232, -231, -230, -228, -227, -225, -224, -222, -221, -219, -217, -216, -214,
                -212, -211, -209, -207, -205, -203, -201, -199, -197, -195, -193, -191, -189, -187, -185, -183,
                -181, -178, -176, -174, -171, -169, -167, -164, -162, -159, -157, -155, -152, -149, -147, -144,
                -142, -139, -136, -134, -131, -128, -126, -123, -120, -117, -115, -112, -109, -106, -103, -100,
                -97, -95, -92, -89, -86, -83, -80, -77, -74, -71, -68, -65, -62, -59, -56, -53,
                -49, -46, -43, -40, -37, -34, -31, -28, -25, -21, -18, -15, -12, -9, -6, -3,
                -181, -183, -185, -187, -189, -191, -193, -195, -197, -199, -201, -203, -205, -207, -209, -211,
                -212, -214, -216, -217, -219, -221, -222, -224, -225, -227, -228, -230, -231, -232, -234, -235,
                -236, -237, -238, -239, -241, -242, -243, -244, -244, -245, -246, -247, -248, -249, -249, -250,
                -251, -251, -252, -252, -253, -253, -254, -254, -254, -255, -255, -255, -255, -255, -255, -255};
        std::memcpy(weights_r_data, weights_r_data_const, size);

        const int16_t weights_i_data_const[] = {
                0, -3, -6, -9, -12, -15, -18, -21, -25, -28, -31, -34, -37, -40, -43, -46,
                -49, -53, -56, -59, -62, -65, -68, -71, -74, -77, -80, -83, -86, -89, -92, -95,
                -97, -100, -103, -106, -109, -112, -115, -117, -120, -123, -126, -128, -131, -134, -136, -139,
                -142, -144, -147, -149, -152, -155, -157, -159, -162, -164, -167, -169, -171, -174, -176, -178,
                -181, -183, -185, -187, -189, -191, -193, -195, -197, -199, -201, -203, -205, -207, -209, -211,
                -212, -214, -216, -217, -219, -221, -222, -224, -225, -227, -228, -230, -231, -232, -234, -235,
                -236, -237, -238, -239, -241, -242, -243, -244, -244, -245, -246, -247, -248, -249, -249, -250,
                -251, -251, -252, -252, -253, -253, -254, -254, -254, -255, -255, -255, -255, -255, -255, -255,
                -256, -255, -255, -255, -255, -255, -255, -255, -254, -254, -254, -253, -253, -252, -252, -251,
                -251, -250, -249, -249, -248, -247, -246, -245, -244, -244, -243, -242, -241, -239, -238, -237,
                -236, -235, -234, -232, -231, -230, -228, -227, -225, -224, -222, -221, -219, -217, -216, -214,
                -212, -211, -209, -207, -205, -203, -201, -199, -197, -195, -193, -191, -189, -187, -185, -183,
                -181, -178, -176, -174, -171, -169, -167, -164, -162, -159, -157, -155, -152, -149, -147, -144,
                -142, -139, -136, -134, -131, -128, -126, -123, -120, -117, -115, -112, -109, -106, -103, -100,
                -97, -95, -92, -89, -86, -83, -80, -77, -74, -71, -68, -65, -62, -59, -56, -53,
                -49, -46, -43, -40, -37, -34, -31, -28, -25, -21, -18, -15, -12, -9, -6, -3};
        std::memcpy(weights_i_data, weights_i_data_const, size);
    } else if (size / 2  == 512) {
        const int16_t weights_r_data_const[] = {
                -97, -99, -100, -102, -103, -105, -106, -108, -109, -110, -112, -113, -115, -116, -117, -119,
                -120, -122, -123, -124, -126, -127, -128, -130, -131, -132, -134, -135, -136, -138, -139, -140,
                -142, -143, -144, -146, -147, -148, -149, -151, -152, -153, -155, -156, -157, -158, -159, -161,
                -162, -163, -164, -166, -167, -168, -169, -170, -171, -173, -174, -175, -176, -177, -178, -179,
                -181, -182, -183, -184, -185, -186, -187, -188, -189, -190, -191, -192, -193, -194, -195, -196,
                -197, -198, -199, -200, -201, -202, -203, -204, -205, -206, -207, -208, -209, -210, -211, -211,
                -212, -213, -214, -215, -216, -217, -217, -218, -219, -220, -221, -221, -222, -223, -224, -225,
                -225, -226, -227, -227, -228, -229, -230, -230, -231, -232, -232, -233, -234, -234, -235, -235,
                -236, -237, -237, -238, -238, -239, -239, -240, -241, -241, -242, -242, -243, -243, -244, -244,
                -244, -245, -245, -246, -246, -247, -247, -247, -248, -248, -249, -249, -249, -250, -250, -250,
                -251, -251, -251, -251, -252, -252, -252, -252, -253, -253, -253, -253, -254, -254, -254, -254,
                -254, -254, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255,
                -256, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -254,
                -254, -254, -254, -254, -254, -253, -253, -253, -253, -252, -252, -252, -252, -251, -251, -251,
                -251, -250, -250, -250, -249, -249, -249, -248, -248, -247, -247, -247, -246, -246, -245, -245,
                -244, -244, -244, -243, -243, -242, -242, -241, -241, -240, -239, -239, -238, -238, -237, -237,
                -236, -235, -235, -234, -234, -233, -232, -232, -231, -230, -230, -229, -228, -227, -227, -226,
                -225, -225, -224, -223, -222, -221, -221, -220, -219, -218, -217, -217, -216, -215, -214, -213,
                -212, -211, -211, -210, -209, -208, -207, -206, -205, -204, -203, -202, -201, -200, -199, -198,
                -197, -196, -195, -194, -193, -192, -191, -190, -189, -188, -187, -186, -185, -184, -183, -182,
                -181, -179, -178, -177, -176, -175, -174, -173, -171, -170, -169, -168, -167, -166, -164, -163,
                -162, -161, -159, -158, -157, -156, -155, -153, -152, -151, -149, -148, -147, -146, -144, -143,
                -142, -140, -139, -138, -136, -135, -134, -132, -131, -130, -128, -127, -126, -124, -123, -122,
                -120, -119, -117, -116, -115, -113, -112, -110, -109, -108, -106, -105, -103, -102, -100, -99,
                -97, -96, -95, -93, -92, -90, -89, -87, -86, -84, -83, -81, -80, -78, -77, -75,
                -74, -72, -71, -69, -68, -66, -65, -63, -62, -60, -59, -57, -56, -54, -53, -51,
                -49, -48, -46, -45, -43, -42, -40, -39, -37, -36, -34, -32, -31, -29, -28, -26,
                -25, -23, -21, -20, -18, -17, -15, -14, -12, -10, -9, -7, -6, -4, -3, -1,
                -236, -237, -237, -238, -238, -239, -239, -240, -241, -241, -242, -242, -243, -243, -244, -244,
                -244, -245, -245, -246, -246, -247, -247, -247, -248, -248, -249, -249, -249, -250, -250, -250,
                -251, -251, -251, -251, -252, -252, -252, -252, -253, -253, -253, -253, -254, -254, -254, -254,
                -254, -254, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255};
        std::memcpy(weights_r_data, weights_r_data_const, size);

        const int16_t weights_i_data_const[] = {
                0, -1, -3, -4, -6, -7, -9, -10, -12, -14, -15, -17, -18, -20, -21, -23,
                -25, -26, -28, -29, -31, -32, -34, -36, -37, -39, -40, -42, -43, -45, -46, -48,
                -49, -51, -53, -54, -56, -57, -59, -60, -62, -63, -65, -66, -68, -69, -71, -72,
                -74, -75, -77, -78, -80, -81, -83, -84, -86, -87, -89, -90, -92, -93, -95, -96,
                -97, -99, -100, -102, -103, -105, -106, -108, -109, -110, -112, -113, -115, -116, -117, -119,
                -120, -122, -123, -124, -126, -127, -128, -130, -131, -132, -134, -135, -136, -138, -139, -140,
                -142, -143, -144, -146, -147, -148, -149, -151, -152, -153, -155, -156, -157, -158, -159, -161,
                -162, -163, -164, -166, -167, -168, -169, -170, -171, -173, -174, -175, -176, -177, -178, -179,
                -181, -182, -183, -184, -185, -186, -187, -188, -189, -190, -191, -192, -193, -194, -195, -196,
                -197, -198, -199, -200, -201, -202, -203, -204, -205, -206, -207, -208, -209, -210, -211, -211,
                -212, -213, -214, -215, -216, -217, -217, -218, -219, -220, -221, -221, -222, -223, -224, -225,
                -225, -226, -227, -227, -228, -229, -230, -230, -231, -232, -232, -233, -234, -234, -235, -235,
                -236, -237, -237, -238, -238, -239, -239, -240, -241, -241, -242, -242, -243, -243, -244, -244,
                -244, -245, -245, -246, -246, -247, -247, -247, -248, -248, -249, -249, -249, -250, -250, -250,
                -251, -251, -251, -251, -252, -252, -252, -252, -253, -253, -253, -253, -254, -254, -254, -254,
                -254, -254, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255,
                -256, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -255, -254,
                -254, -254, -254, -254, -254, -253, -253, -253, -253, -252, -252, -252, -252, -251, -251, -251,
                -251, -250, -250, -250, -249, -249, -249, -248, -248, -247, -247, -247, -246, -246, -245, -245,
                -244, -244, -244, -243, -243, -242, -242, -241, -241, -240, -239, -239, -238, -238, -237, -237,
                -236, -235, -235, -234, -234, -233, -232, -232, -231, -230, -230, -229, -228, -227, -227, -226,
                -225, -225, -224, -223, -222, -221, -221, -220, -219, -218, -217, -217, -216, -215, -214, -213,
                -212, -211, -211, -210, -209, -208, -207, -206, -205, -204, -203, -202, -201, -200, -199, -198,
                -197, -196, -195, -194, -193, -192, -191, -190, -189, -188, -187, -186, -185, -184, -183, -182,
                -181, -179, -178, -177, -176, -175, -174, -173, -171, -170, -169, -168, -167, -166, -164, -163,
                -162, -161, -159, -158, -157, -156, -155, -153, -152, -151, -149, -148, -147, -146, -144, -143,
                -142, -140, -139, -138, -136, -135, -134, -132, -131, -130, -128, -127, -126, -124, -123, -122,
                -120, -119, -117, -116, -115, -113, -112, -110, -109, -108, -106, -105, -103, -102, -100, -99,
                -97, -96, -95, -93, -92, -90, -89, -87, -86, -84, -83, -81, -80, -78, -77, -75,
                -74, -72, -71, -69, -68, -66, -65, -63, -62, -60, -59, -57, -56, -54, -53, -51,
                -49, -48, -46, -45, -43, -42, -40, -39, -37, -36, -34, -32, -31, -29, -28, -26,
                -25, -23, -21, -20, -18, -17, -15, -14, -12, -10, -9, -7, -6, -4, -3, -1};
        std::memcpy(weights_i_data, weights_i_data_const, size);
    } else {
        printf_warning("No const twiddle factors in memory. Need to calculate those (time consuming...!). Please wait...\n");
        for (int index = 0; index < size / 2; ++index) {
            _Complex double twiddle = cexp(-I * M_PI * index * 2 / size);
            weights_r_data[index] = int16_t(creal(twiddle) * pow(2., fractional_bits));
            weights_i_data[index] = int16_t(cimag(twiddle) * pow(2., fractional_bits));
        }
        printf("Done!\n");
    }
}

void FFT_VPRO::print_twiddles(int size) {
    printf("\n// %i\n", size / 2);
    printf("if (size == %i) {\n\tconst int16_t weights_r_data_const[] = {\n\t\t", size / 2);
    for (int index = 0; index < size / 2; ++index) {
        printf("%4i", weights_r_data[index]);
        if (index < size / 2 - 1)
            printf(", ");
        if ((index + 1) % 16 == 0 && (index + 1) < size / 2)
            printf("\n\t\t");
    }
    printf("\t};\n");
    printf("\tconst int16_t weights_i_data_const[] = {\n\t\t");
    for (int index = 0; index < size / 2; ++index) {
        printf("%4i", weights_i_data[index]);
        if (index < size / 2 - 1)
            printf(", ");
        if ((index + 1) % 16 == 0 && (index + 1) < size / 2)
            printf("\n\t\t");
    }
    printf("\t};\n");
    printf("\tstd::memcpy(weights_r_data, weights_r_data_const, size);\n");
    printf("\tstd::memcpy(weights_i_data, weights_i_data_const, size);\n");
    printf("}\n");
}

void FFT_VPRO::fft() {
    for (int i = 0; i < fft_size; ++i) {
        if (i < fft_size / 2) {
            input_r[i] = 1 << fractional_bits;
        } else {
            input_r[i] = 0 << fractional_bits;
        }
        input_i[i] = 0;
    }

//    printf("\nOWN VPRO Data: ");
//    for (int i = 0; i < fft_size; i++)
//        if (!input_i[i])
//            printf("%g ", input_r[i] / pow(2., fractional_bits));
//        else
//            printf("(%g, %g) ", input_r[i] / pow(2., fractional_bits), input_i[i] / pow(2., fractional_bits));
//    printf("\n");

    input_reorder();

    printf("[LM] Layout: \n\tLM_INPUT_R = %d\n\tLM_INPUT_I = %d\n", LM_INPUT_R, LM_INPUT_I);
    printf("\tLM_TMP_R = %d\n\tLM_TMP_I = %d\n", LM_TMP_R, LM_TMP_I);
    printf("\tLM_TWIDDLE_R = %d\n\tLM_TWIDDLE_I = %d\n", LM_TWIDDLE_R, LM_TWIDDLE_I);
    printf("\tLM_TWIDDLE_32_R = %d\n\tLM_TWIDDLE_32_I = %d\n", LM_TWIDDLE_32_R, LM_TWIDDLE_32_I);
    printf("[RF] Layout: \n\tRF_R = %d\n\tRF_I = %d\n", RF_R, RF_I);
    printf("\tRF_TWIDDLE_R = %d\n\tRF_TWIDDLE_I = %d\n", RF_TWIDDLE_R, RF_TWIDDLE_I);

    dcma_reset();

    // load input
    dma_ext1D_to_loc1D(0, intptr_t(input_r), LM_BASE_VU(0) + LM_INPUT_R, fft_size);
    dma_ext1D_to_loc1D(0, intptr_t(input_i), LM_BASE_VU(0) + LM_INPUT_I, fft_size);

    // load twiddles
    dma_ext1D_to_loc1D(0, intptr_t(weights_r_data), LM_BASE_VU(0) + LM_TWIDDLE_R, fft_size / 2);
    dma_ext1D_to_loc1D(0, intptr_t(weights_i_data), LM_BASE_VU(0) + LM_TWIDDLE_I, fft_size / 2);

    dma_ext2D_to_loc1D(0, intptr_t(weights_r_data), LM_BASE_VU(0) + LM_TWIDDLE_32_R, 32 + 1, 1,
                       fft_size / 64);  // stride 32 (only every...)
    dma_ext2D_to_loc1D(0, intptr_t(weights_i_data), LM_BASE_VU(0) + LM_TWIDDLE_32_I, 32 + 1, 1, fft_size / 64);

    dma_wait_to_finish();

    vpro_mac_h_bit_shift(fractional_bits);
    vpro_mul_h_bit_shift(fractional_bits);
    vpro_set_mac_init_source(VPRO::MAC_INIT_SOURCE::ADDR);
    vpro_set_mac_reset_mode(VPRO::MAC_RESET_MODE::X_INCREMENT);

    aux_reset_all_stats();

//#ifdef SIMULATION
//    printf_info("DMA Load of twiddle done!\n");
//    core_->pauseSim();
//#endif

    int step = fft_size / 2;
    for (int stage = 0; stage < fft_stages; ++stage) {
        execute_stage(stage, step);
//#ifdef SIMULATION
//        vpro_wait_busy();
//        printf_info("Stage %i done!\n", stage);
//
//        // store result
//        dma_loc1D_to_ext1D(0, intptr_t(input_r), LM_BASE_VU(0) + LM_INPUT_R, fft_size);
//        dma_loc1D_to_ext1D(0, intptr_t(input_i), LM_BASE_VU(0) + LM_INPUT_I, fft_size);
//        dma_wait_to_finish();
//        dcma_flush();
//        printf("OWN VPRO FFT: ");
//        for (int i = 0; i < fft_size; i++)
//            if (!input_i[i])
//                printf("(%g) ", input_r[i] / pow(2., fractional_bits));
//            else
//                printf("(%g + %gi) ", input_r[i] / pow(2., fractional_bits), input_i[i] / pow(2., fractional_bits));
//        printf("\n");
//
//        core_->pauseSim();
//#endif
        step = step / 2;
    }
    vpro_wait_busy();

    // store result
    dma_loc1D_to_ext1D(0, intptr_t(input_r), LM_BASE_VU(0) + LM_INPUT_R, fft_size);
    dma_loc1D_to_ext1D(0, intptr_t(input_i), LM_BASE_VU(0) + LM_INPUT_I, fft_size);

    dma_wait_to_finish();

    aux_print_statistics();

    dcma_flush();

    printf("DONE!\n");

    printf("OWN VPRO FFT: ");
    for (int i = 0; i < fft_size; i++)
        if (!input_i[i])
            printf("%g ", input_r[i] / pow(2., fractional_bits));
        else
            printf("(%g, %g) ", input_r[i] / pow(2., fractional_bits), input_i[i] / pow(2., fractional_bits));
    printf("\n");
}

void FFT_VPRO::execute_stage(int nr, int twiddle_step) {
    int butterflies = fft_size / (2 << nr);
    int butterfly_size = (2 << nr);
    int butterfly_wing_size = butterfly_size / 2;
//    printf_info("[VPRO] Stage %d start. twiddle_step: %i, butterflies: %i, butterfly_size: %i, wing_size: %i\n",
//           nr, twiddle_step, butterflies, butterfly_size, butterfly_wing_size);

    // twiddles_r to RF.
    // this stage needs butterfly_wing_size twiddle factors
    // they have indizes 2*stage_nr

    if (twiddle_step < 64) {
        if (butterfly_wing_size <= 64) {
            VPRO::DIM2::LOADSTORE::loads(LM_TWIDDLE_R,
                                         0, twiddle_step, 0,
                                         butterfly_wing_size - 1, 0);
            VPRO::DIM2::PROCESSING::add(L0_1,
                                        DST_ADDR(RF_TWIDDLE_R, 1, 0), SRC1_LS_2D, SRC2_IMM_2D(0),
                                        butterfly_wing_size - 1, 0);

            // twiddles_i to RF
            VPRO::DIM2::LOADSTORE::loads(LM_TWIDDLE_I,
                                         0, twiddle_step, 0,
                                         butterfly_wing_size - 1, 0);
            VPRO::DIM2::PROCESSING::add(L0_1,
                                        DST_ADDR(RF_TWIDDLE_I, 1, 0), SRC1_LS_2D, SRC2_IMM_2D(0),
                                        butterfly_wing_size - 1, 0);
        } else {
            VPRO::DIM2::LOADSTORE::loads(LM_TWIDDLE_R,
                                         0, twiddle_step, 0,
                                         butterfly_wing_size / 2 - 1, 0);
            VPRO::DIM2::PROCESSING::add(L0_1,
                                        DST_ADDR(RF_TWIDDLE_R, 1, 0), SRC1_LS_2D, SRC2_IMM_2D(0),
                                        butterfly_wing_size / 2 - 1, 0);

            VPRO::DIM2::LOADSTORE::loads(LM_TWIDDLE_R,
                                         butterfly_wing_size / 2 * twiddle_step, twiddle_step, 0,
                                         butterfly_wing_size / 2 - 1, 0);
            VPRO::DIM2::PROCESSING::add(L0_1,
                                        DST_ADDR(RF_TWIDDLE_R + butterfly_wing_size / 2 * twiddle_step, 1, 0),
                                        SRC1_LS_2D, SRC2_IMM_2D(0),
                                        butterfly_wing_size / 2 - 1, 0);

            // twiddles_i to RF
            VPRO::DIM2::LOADSTORE::loads(LM_TWIDDLE_I,
                                         0, twiddle_step, 0,
                                         butterfly_wing_size / 2 - 1, 0);
            VPRO::DIM2::PROCESSING::add(L0_1,
                                        DST_ADDR(RF_TWIDDLE_I, 1, 0), SRC1_LS_2D, SRC2_IMM_2D(0),
                                        butterfly_wing_size / 2 - 1, 0);
            VPRO::DIM2::LOADSTORE::loads(LM_TWIDDLE_I,
                                         butterfly_wing_size / 2 * twiddle_step, twiddle_step, 0,
                                         butterfly_wing_size / 2 - 1, 0);
            VPRO::DIM2::PROCESSING::add(L0_1,
                                        DST_ADDR(RF_TWIDDLE_I + butterfly_wing_size / 2 * twiddle_step, 1, 0),
                                        SRC1_LS_2D, SRC2_IMM_2D(0),
                                        butterfly_wing_size / 2 - 1, 0);
        }
    } else {
        VPRO::DIM2::LOADSTORE::loads(LM_TWIDDLE_32_R,
                                     0, twiddle_step / 32, 0,
                                     butterfly_wing_size - 1, 0);
        VPRO::DIM2::PROCESSING::add(L0_1,
                                    DST_ADDR(RF_TWIDDLE_R, 1, 0), SRC1_LS_2D, SRC2_IMM_2D(0),
                                    butterfly_wing_size - 1, 0);

        // twiddles_i to RF
        VPRO::DIM2::LOADSTORE::loads(LM_TWIDDLE_32_I,
                                     0, twiddle_step / 32, 0,
                                     butterfly_wing_size - 1, 0);
        VPRO::DIM2::PROCESSING::add(L0_1,
                                    DST_ADDR(RF_TWIDDLE_I, 1, 0), SRC1_LS_2D, SRC2_IMM_2D(0),
                                    butterfly_wing_size - 1, 0);
    }

    if (butterfly_size >= 32) {
//        printf("[VPRO] FFT. butterfly_size >= 32. loop butterflies by risc...\n");
        for (int i = 0; i < butterflies; ++i) {
            int start = i * butterfly_size;
            if (butterfly_wing_size <= 64) {
                _vpro_fft(butterfly_wing_size, 0, 0, 0, 0, start);
            } else { // butterfly_wing_size > 64
//                printf("[VPRO] butterfly_wing_size > 64. split in two calls...\n");
                // TODO: for larger FFTs - check
                _vpro_fft(butterfly_wing_size / 2, 0, 0, 0, 0, start);
                _vpro_fft(butterfly_wing_size / 2, 0, 0, butterfly_wing_size / 2, 0, start);
//                _vpro_fft(butterfly_wing_size, butterflies - 1, butterfly_size, 0);
            }
        }
    } else {
//        printf("[VPRO] FFT. butterfly_size < 32. using y to loop all butterflies...\n");
        if (butterflies >= 64) {
//            printf_info("[VPRO] >= 64 butterflies. split in two calls...\n");
            // TODO: duplicate - check
            _vpro_fft(butterfly_wing_size, butterflies / 2 - 1, butterfly_size, 0, 0, 0);
            _vpro_fft(butterfly_wing_size, butterflies / 2 - 1, butterfly_size, 0, 0, butterflies / 2 * butterfly_size);   // reuse same twiddle values (small wings)
        } else {
            _vpro_fft(butterfly_wing_size, butterflies - 1, butterfly_size, 0);
        }
    }
//        printf("[VPRO] Stage %d done!\n", nr);
}


void
FFT_VPRO::_vpro_fft(uint32_t butterfly_wing_size, uint32_t yend, uint32_t beta, uint32_t item_offset, uint32_t twiddle_offset, uint32_t ls_offset) {

//    printf_info("Wing: butterfly_wing_size: %i, yend: %i, beta: %i, item_offset: %i, twiddle_offset: %i, ls_offset: %i\n",
//                butterfly_wing_size, yend, beta, item_offset, twiddle_offset, ls_offset);
    // lower part ( mult with twiddle )
    // store again to LM [tmp with half size]

    // Load I
    // MUL tw_r * in_i => L0
    // MUL tw_i * in_i => L1
    VPRO::DIM2::PROCESSING::mulh(L0,
                                 DST_ADDR(item_offset, 1, beta), SRC1_LS_2D, SRC2_ADDR(RF_TWIDDLE_R + twiddle_offset, 1, 0),
                                 butterfly_wing_size - 1, yend);
    VPRO::DIM2::PROCESSING::mulh(L1,
                                 DST_ADDR(item_offset, 1, beta), SRC1_LS_2D, SRC2_ADDR(RF_TWIDDLE_I + twiddle_offset, 1, 0),
                                 butterfly_wing_size - 1, yend);
    VPRO::DIM2::LOADSTORE::loads(LM_INPUT_I,
                                 ls_offset + butterfly_wing_size + item_offset, 1, beta,
                                 butterfly_wing_size - 1, yend);

    // Load R
    // MUL tw_i * in_r => L0
    // MUL tw_r * in_r => L1 [Accu]
    VPRO::DIM2::PROCESSING::mulh(L0,
                                 DST_ADDR(fft_size + item_offset, 1, beta), SRC1_LS_2D,
                                 SRC2_ADDR(RF_TWIDDLE_I + twiddle_offset, 1, 0),
                                 butterfly_wing_size - 1, yend);
    VPRO::DIM2::PROCESSING::mulh(L1,
                                 DST_ADDR(fft_size + item_offset, 1, beta), SRC1_LS_2D,
                                 SRC2_ADDR(RF_TWIDDLE_R + twiddle_offset, 1, 0),
                                 butterfly_wing_size - 1, yend);
    VPRO::DIM2::LOADSTORE::loads(LM_INPUT_R,
                                 ls_offset + butterfly_wing_size + item_offset, 1, beta,
                                 butterfly_wing_size - 1, yend);

    // sub in L1
    VPRO::DIM2::PROCESSING::sub(L1,
                                DST_ADDR(item_offset, 1, beta), SRC1_ADDR(item_offset, 1, beta),
                                SRC2_ADDR(fft_size + item_offset, 1, 0),   // SRC2 - SRC1
                                butterfly_wing_size - 1, yend,
                                true);

    // store r (L1), direct after sub
    VPRO::DIM2::LOADSTORE::store(LM_TMP_R,
                                 ls_offset + butterfly_wing_size + item_offset, 1, beta,
                                 butterfly_wing_size - 1, yend,
                                 L1);

    // store i (L0),  + in L0 [done by mac init],
    VPRO::DIM2::PROCESSING::add(L0,
                                DST_ADDR(item_offset, 1, beta), SRC1_ADDR(item_offset, 1, beta),
                                SRC2_ADDR(fft_size + item_offset, 1, beta),
                                butterfly_wing_size - 1, yend,
                                true);

    VPRO::DIM2::LOADSTORE::store(LM_TMP_I,
                                 ls_offset + butterfly_wing_size + item_offset, 1, beta,
                                 butterfly_wing_size - 1, yend,
                                 L0);

    // load input r
    VPRO::DIM2::LOADSTORE::loads(LM_INPUT_R, // no offset
                                 ls_offset + item_offset, 1, beta,
                                 butterfly_wing_size - 1, yend);
    VPRO::DIM2::PROCESSING::add(L0_1,
                                DST_ADDR(item_offset, 1, beta), SRC1_LS_2D, SRC2_IMM_2D(0),
                                butterfly_wing_size - 1, yend);
    // load input i
    VPRO::DIM2::LOADSTORE::loads(LM_INPUT_I, // no offset
                                 ls_offset + item_offset, 1, beta,
                                 butterfly_wing_size - 1, yend);
    VPRO::DIM2::PROCESSING::add(L0_1,
                                DST_ADDR(fft_size + item_offset, 1, beta), SRC1_LS_2D, SRC2_IMM_2D(0),
                                butterfly_wing_size - 1, yend);

    // tmp is mult result or original data now
    // tmp (= input/upper) is inside RF (0/fft_size)
    // r
    VPRO::DIM2::PROCESSING::add(L0,
                                DST_ADDR(item_offset, 1, beta), SRC1_LS_2D, SRC2_ADDR(item_offset, 1, beta),
                                butterfly_wing_size - 1, yend);
    VPRO::DIM2::PROCESSING::sub(L1,
                                DST_ADDR(item_offset, 1, beta), SRC1_LS_2D,
                                SRC2_ADDR(item_offset, 1, beta),  // src2 - src1
                                butterfly_wing_size - 1, yend);
    VPRO::DIM2::LOADSTORE::loads(LM_TMP_R,
                                 ls_offset + butterfly_wing_size + item_offset, 1, beta,
                                 butterfly_wing_size - 1, yend);

    // i
    VPRO::DIM2::PROCESSING::add(L0,
                                DST_ADDR(fft_size + item_offset, 1, beta), SRC1_LS_2D,
                                SRC2_ADDR(fft_size + item_offset, 1, beta),
                                butterfly_wing_size - 1, yend);
    VPRO::DIM2::PROCESSING::sub(L1,
                                DST_ADDR(fft_size + item_offset, 1, beta), SRC1_LS_2D,
                                SRC2_ADDR(fft_size + item_offset, 1, beta),  // src2 - src1
                                butterfly_wing_size - 1, yend);
    VPRO::DIM2::LOADSTORE::loads(LM_TMP_I,
                                 ls_offset + butterfly_wing_size + item_offset, 1, beta,
                                 butterfly_wing_size - 1, yend);

    // L0
    //store r
    VPRO::DIM2::PROCESSING::add(L0,
                                DST_ADDR(item_offset, 1, beta), SRC1_ADDR(item_offset, 1, beta), SRC2_IMM_2D(0),
                                butterfly_wing_size - 1, yend,
                                true);
    VPRO::DIM2::LOADSTORE::store(LM_INPUT_R,
                                 ls_offset + item_offset, 1, beta,
                                 butterfly_wing_size - 1, yend,
                                 L0);

    // store i
    VPRO::DIM2::PROCESSING::add(L0,
                                DST_ADDR(fft_size + item_offset, 1, beta), SRC1_ADDR(fft_size + item_offset, 1, beta),
                                SRC2_IMM_2D(0),
                                butterfly_wing_size - 1, yend,
                                true);
    VPRO::DIM2::LOADSTORE::store(LM_INPUT_I,
                                 ls_offset + item_offset, 1, beta,
                                 butterfly_wing_size - 1, yend,
                                 L0);

    // L1
    //store r
    VPRO::DIM2::PROCESSING::add(L1,
                                DST_ADDR(item_offset, 1, beta), SRC1_ADDR(item_offset, 1, beta), SRC2_IMM_2D(0),
                                butterfly_wing_size - 1, yend,
                                true);
    VPRO::DIM2::LOADSTORE::store(LM_INPUT_R,
                                 ls_offset + butterfly_wing_size + item_offset, 1, beta,
                                 butterfly_wing_size - 1, yend,
                                 L1);

    // store i
    VPRO::DIM2::PROCESSING::add(L1,
                                DST_ADDR(fft_size + item_offset, 1, beta), SRC1_ADDR(fft_size + item_offset, 1, beta),
                                SRC2_IMM_2D(0),
                                butterfly_wing_size - 1, yend,
                                true);
    VPRO::DIM2::LOADSTORE::store(LM_INPUT_I,
                                 ls_offset + butterfly_wing_size + item_offset, 1, beta,
                                 butterfly_wing_size - 1, yend,
                                 L1);
}



