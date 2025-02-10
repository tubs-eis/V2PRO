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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <vpro.h>
#include <eisv.h>

// .nobss = uninitialized! (speed up sim), .vpro sections the risc access with dma (uninitialized as well)
volatile int16_t __attribute__ ((section (".vpro"))) test_array_1[1024];
volatile int16_t __attribute__ ((section (".vpro"))) test_array_2[1024];
volatile int16_t __attribute__ ((section (".vpro"))) result_array[1024];
volatile int16_t __attribute__ ((section (".vpro"))) result_array_zeros[1024];
volatile int16_t __attribute__ ((section (".vpro"))) result_array_dead[1024];

// no initialization data for those region!
int16_t kernel[] __attribute__ ((section (".vpro"))) = {1, 2, 1, 2, 4, 2, 1, 2, 1};
int16_t kernel2[] __attribute__ ((section (".vpro"))) = {2, 4, 2, 4, 8, 4, 2, 4, 2};
int16_t bias[] __attribute__ ((section (".vpro"))) = {10};
int16_t bias2[] __attribute__ ((section (".vpro"))) = {20};

bool pad_flags[4] = {false, false, false, false};  // for dma padding

int main(int argc, char *argv[]) {
    sim_init(main, argc, argv);
    sim_min_req(1, 1, 2);

    printf("\nStart\n");

    // reset result array
    {
      int16_t count = 0;
      for (volatile int16_t &i: result_array) {
          i = 0xdead;
      }
      for (volatile int16_t &i: result_array_zeros) {
          i = 0;
      }
      for (volatile int16_t &i: result_array_dead) {
          i = 0xdead;
      }
    }
    /**
    * Create some (random) input data
    */
    {
      int16_t count = 0;
      for (volatile int16_t &i: test_array_1) {
          i = count;
          count = (abs(count) + 1) * (-1);
      }
      count = 1024 - 1;
      for (volatile int16_t &i: test_array_2) {
          i = count;
          count--;
      }

      kernel[0] = 3;
      kernel[1] = 1;
      kernel[2] = -1;
      kernel[3] = -1;
      kernel[4] = 1;
      kernel[5] = 1;
      kernel[6] = -1;
      kernel[7] = -1;
      kernel[8] = -2;
      bias[0] = 10;

  /*  kernel[0] = 623;
      kernel[1] = 555;
      kernel[2] = -51;
      kernel[3] = -59;
      kernel[4] = 52;
      kernel[5] = 5599;
      kernel[6] = -8711;
      kernel[7] = -125;
      kernel[8] = -2117;
      bias[0] = 10;
*/
      kernel2[0] = 2;
      kernel2[1] = 4;
      kernel2[2] = 2;
      kernel2[3] = 4;
      kernel2[4] = 8;
      kernel2[5] = 4;
      kernel2[6] = 2;
      kernel2[7] = 4;
      kernel2[8] = 2;
      bias2[0] = 20;
    }

    /**
    * Initialize / Reset Memories of VPRO
    */
    // set LMs to error value (0xdead) (Unit 0 and 1 if available)
    if (VPRO_CFG::UNITS > 1) {
        for (size_t i = 0; i < 8192; i+=1024) {
          dma_ext1D_to_loc1D(0, intptr_t(&(result_array_dead[0])), LM_BASE_VU(1)+i, 1024);
        }
    }
    for (size_t i = 0; i < 8192; i+=1024) {
      dma_ext1D_to_loc1D(0, intptr_t(&(result_array_dead[0])), LM_BASE_VU(0)+i, 1024);
    }
    // set RFs to error value (0xdead)
    __vpro(L0_1, BLOCKING, NO_CHAIN, FUNC_ADD, NO_FLAG_UPDATE, DST_ADDR(0, 1, 32),
           SRC2_IMM_2D(0), SRC2_IMM_2D(0xdead), 31, 31);

    // defaulting all VPRO configuration registers
    vpro_mac_h_bit_shift(24);
    vpro_mul_h_bit_shift(24);
    vpro_set_mac_init_source(VPRO::MAC_INIT_SOURCE::NONE);

//    vpro_wait_busy(0xffffffff, 0xffffffff);
//    dma_wait_to_finish(0xffffffff);
    vpro_sync();

    // reset cycle counters in subsystem
    aux_reset_all_stats();

    // execute test
    uint32_t kernel_x = 3, kernel_y = 3;
    uint32_t seg_out_h = 28, seg_out_w = 28;
    uint32_t seg_in_w = seg_out_w + (kernel_x - 1);
    //uint32_t seg_in_h = seg_out_h + (kernel_x - 1)

    int kernel_load_shift_right = 1;
    int conv_result_shift_right = 3;
    int bias_shift_right = -1;
    int store_shift_right = 1;

    vpro_mac_h_bit_shift(conv_result_shift_right);
    vpro_mul_h_bit_shift(conv_result_shift_right);

    int buffer = 0; // LM Base input
    int out_buffer = 2048; // LM Base output
    int out_buffer2 = 2048 + 1024; // LM Base output

    // define start addresses of kernels/biases in RF (=LM)
    constexpr uint32_t RF_KERNEL_BASE = 1015;
    constexpr uint32_t RF_BIAS_BASE = 1014;
    constexpr uint32_t RF_KERNEL_BASE2 = 1015 - 10;
    constexpr uint32_t RF_BIAS_BASE2 = 1014 - 10;

    // set LM to contain input data
    dma_ext1D_to_loc1D(0, intptr_t(&(test_array_1[0])), LM_BASE_VU(0) + 0, 1024);
    dma_ext1D_to_loc1D(0, intptr_t(&(test_array_2[0])), LM_BASE_VU(0) + 1024, 1024);

    // set LM to contain kernel 3x3
    // set LM to contain bias
    dma_ext1D_to_loc1D(0, intptr_t(&(kernel[0])), LM_BASE_VU(0) + RF_KERNEL_BASE, 9);
    dma_ext1D_to_loc1D(0, intptr_t(&(bias[0])), LM_BASE_VU(0) + RF_BIAS_BASE, 1);
    dma_ext1D_to_loc1D(0, intptr_t(&(kernel2[0])), LM_BASE_VU(0) + RF_KERNEL_BASE2, 9);
    dma_ext1D_to_loc1D(0, intptr_t(&(bias2[0])), LM_BASE_VU(0) + RF_BIAS_BASE2, 1);


    vpro_sync();

    /**
     *  to both (1)
     */
    // LOAD KERNEL
    VPRO::DIM2::LOADSTORE::loads(RF_KERNEL_BASE2, //1015
                                 0, 1, kernel_y,
                                 kernel_x - 1, kernel_y - 1);

    VPRO::DIM2::PROCESSING::shift_ar(L0_1,
                                     DST_ADDR(RF_KERNEL_BASE, 1, kernel_y),   // 1015
                                     SRC1_LS_2D,
                                     SRC2_IMM_2D(kernel_load_shift_right),
                                     kernel_x - 1,
                                     kernel_y - 1);

    // LOAD BIAS
    VPRO::DIM2::LOADSTORE::loads(RF_BIAS_BASE2,  //1014-10
                                 0, 0, 0,
                                 0, 0);
    VPRO::DIM2::PROCESSING::mull(L0_1,
                                 DST_ADDR(RF_BIAS_BASE, 0, 0),    // 1014
                                 SRC1_IMM_2D(1u << (-bias_shift_right)),
                                 SRC2_LS_2D,
                                 0,
                                 0);

    /**
     *  to L0 (0)
     */
    // LOAD KERNEL
    VPRO::DIM2::LOADSTORE::loads(RF_KERNEL_BASE, // 1015
                                 0, 1, kernel_y,
                                 kernel_x - 1, kernel_y - 1);

    VPRO::DIM2::PROCESSING::shift_ar(L0,
                                     DST_ADDR(RF_KERNEL_BASE, 1, kernel_y),   // 1015
                                     SRC1_LS_2D,
                                     SRC2_IMM_2D(kernel_load_shift_right),
                                     kernel_x - 1,
                                     kernel_y - 1);

    // LOAD BIAS
    VPRO::DIM2::LOADSTORE::loads(RF_BIAS_BASE,   // 1014
                                 0, 0, 0,
                                 0, 0);

    VPRO::DIM2::PROCESSING::mull(L0,
                                 DST_ADDR(RF_BIAS_BASE, 0, 0),    // 1014
                                 SRC1_IMM_2D(1u << (-bias_shift_right)),
                                 SRC2_LS_2D,
                                 0,
                                 0);

    // CONV (+bias)
    assert(seg_out_w - 1 <= MAX_X_END);
    assert(seg_out_h - 1 <= MAX_Y_END);
    assert(seg_in_w <= MAX_BETA);
    assert(seg_out_w <= MAX_BETA);
    if (kernel_x == 3 && kernel_y == 3) {
        uint32_t offset_in = 0;
        uint32_t offset_out = 0;

        vpro_set_mac_reset_mode(VPRO::MAC_RESET_MODE::Z_INCREMENT);
        vpro_set_mac_init_source(VPRO::MAC_INIT_SOURCE::ADDR);

        for (size_t y = 0; y < seg_out_h; ++y) {
            VPRO::DIM3::LOADSTORE::loads(buffer,
                                         offset_in, 1, seg_in_w, 1,
                                         2, 2, seg_out_w - 1);

/*
            // add to previous value (read acc init value from dst register)
            VPRO::DIM3::PROCESSING::mach_init_addr(L0_1,  // shift right by 3
                                                   DST_ADDR(offset_out, 0, 0, 1),
                                                   SRC1_LS_3D,
                                                   SRC2_ADDR(RF_KERNEL_BASE, 1, 3, 0),    // 1015
                                                   2, 2, seg_out_w - 1,
                                                   offset_out, 0, 0, 1,  // previous value ( = dst )
                                                   false, true);
 */

            // add to BIAS (read acc init value from bias register)
            VPRO::DIM3::PROCESSING::mach_init_addr(L0_1,  // shift right by 3
                                                   DST_ADDR(offset_out, 0, 0, 1),
                                                   SRC1_LS_3D,
                                                   SRC2_ADDR(RF_KERNEL_BASE, 1, 3, 0),    // 1015
                                                   2, 2, seg_out_w - 1,
                                                   RF_BIAS_BASE, 0, 0, 0,  // 1014
                                                   false, true);
            offset_in += seg_in_w;
            offset_out += seg_out_w;
        }
    } else {  // kernel_w/h != 3
        assert(kernel_x == 1 && kernel_y == 1);
        __vpro(LS, NONBLOCKING, IS_CHAIN, FUNC_LOADS, NO_FLAG_UPDATE,
               DST_ADDR(0, 0, 0),
               SRC1_ADDR(0, 1, seg_in_w),
               SRC2_IMM_2D(buffer),
               seg_out_w - 1, seg_out_h - 1);
        // mul
        __vpro(L0_1, NONBLOCKING, NO_CHAIN, FUNC_MULH, NO_FLAG_UPDATE,
               DST_ADDR(0, 1, seg_out_w),
               SRC1_LS_2D,
               SRC2_ADDR(RF_KERNEL_BASE, 0, 0),
               seg_out_w - 1, seg_out_h - 1);

        // add bias
        __vpro(L0_1, NONBLOCKING, NO_CHAIN, FUNC_ADD, NO_FLAG_UPDATE,
               DST_ADDR(0, 1, seg_out_w),
               SRC1_ADDR(0, 1, seg_out_w),
               SRC2_ADDR(RF_BIAS_BASE, 0, 0),
               seg_out_w - 1, seg_out_h - 1);
    }

    // STORE L0
    VPRO::DIM2::PROCESSING::shift_ar(L0,
                                     DST_ADDR(0, 1, seg_out_w),
                                     SRC1_ADDR(0, 1, seg_out_w),
                                     SRC2_IMM_2D(store_shift_right),  // 1
                                     seg_out_w - 1,
                                     seg_out_h - 1, true);

    VPRO::DIM2::LOADSTORE::store(out_buffer,
                                 0, 1, seg_out_w,
                                 seg_out_w - 1,
                                 seg_out_h - 1,
                                 L0);

    // STORE L1
    VPRO::DIM2::PROCESSING::shift_ar(L1,
                                     DST_ADDR(0, 1, seg_out_w),
                                     SRC1_ADDR(0, 1, seg_out_w),
                                     SRC2_IMM_2D(store_shift_right),  // 1
                                     seg_out_w - 1,
                                     seg_out_h - 1, true);

    VPRO::DIM2::LOADSTORE::store(out_buffer2,
                                 0, 1, seg_out_w,
                                 seg_out_w - 1,
                                 seg_out_h - 1,
                                 L1);

    vpro_sync();

    // use only unit 0 ... TODO?!
    dma_loc1D_to_ext1D(0, intptr_t(&(result_array[0])), LM_BASE_VU(0) + out_buffer,
                       seg_out_w * seg_out_h);
    assert(seg_out_w * seg_out_h + 180 <= 1024);   // avoid overflow of result_array (size: 1024)
    dma_loc1D_to_ext1D(0, intptr_t(&(result_array[180])), LM_BASE_VU(0) + out_buffer2,
                       seg_out_w * seg_out_h);

    vpro_sync();
    dcma_flush();

    // printf runtime (cycle counters in subsystem)
    auto cnt = (((uint64_t(aux_get_sys_time_hi())) << 32) + uint64_t(aux_get_sys_time_lo()));
    aux_print_statistics();
    printf("[VPRO Conv only] Sys-Time (Risc-V Cycles): %li\n", cnt);
    aux_reset_all_stats();

    int16_t referenceresult[1024];
    {
        int16_t inputdata[2048];
        int16_t outputdata[1024];
        int16_t outputdata2[1024];
        for (int i = 0; i < 1024; ++i) {
            inputdata[i] = int16_t(test_array_1[i]);
            inputdata[i + 1024] = int16_t(test_array_2[i]);
        }
        for (uint ix = 0; ix < seg_out_h; ++ix) {
            for (uint iy = 0; iy < seg_out_h; ++iy) {
                int64_t output_pixel, output_pixel2;
                if (bias_shift_right < 0) {
                    output_pixel = (bias[0] << -bias_shift_right) << conv_result_shift_right;
                    output_pixel2 = (bias2[0] << -bias_shift_right) << conv_result_shift_right;
                } else {
                    output_pixel = (bias[0] >> bias_shift_right) << conv_result_shift_right;
                    output_pixel2 = (bias2[0] >> bias_shift_right) << conv_result_shift_right;
                }
                for (uint kx = 0; kx < kernel_x; ++kx) {
                    for (uint ky = 0; ky < kernel_x; ++ky) {
                        output_pixel += inputdata[ix + (seg_out_h + kernel_x - 1) * iy + kx +
                                                  ky * (seg_out_h + kernel_x - 1)] *
                                        (kernel[kx + ky * kernel_x] >> kernel_load_shift_right);
                        output_pixel2 += inputdata[ix + (seg_out_h + kernel_x - 1) * iy + kx +
                                                   ky * (seg_out_h + kernel_x - 1)] *
                                         (kernel2[kx + ky * kernel_x] >> kernel_load_shift_right);
                    }
                }
                outputdata[ix + iy * seg_out_h] = int16_t((output_pixel >> conv_result_shift_right) >> store_shift_right);
                outputdata2[ix + iy * seg_out_h] = int16_t((output_pixel2 >> conv_result_shift_right) >> store_shift_right);
            }
        }
        for (int i = 0; i < 1024; ++i) {
            referenceresult[i] = 0xdead;
        }
        for (uint i = 0; i < seg_out_h * seg_out_h; ++i) {
            referenceresult[i] = outputdata[i];
        }
        assert(seg_out_h * seg_out_h + 180 <= 1024);   // avoid overflow of result_array (size: 1024)
        for (uint i = 0; i < seg_out_h * seg_out_h; ++i) {
            referenceresult[i + 180] = outputdata2[i];
        }
    }

    cnt = (((uint64_t(aux_get_sys_time_hi())) << 32) + uint64_t(aux_get_sys_time_lo()));
    printf("[Reference calc] Sys-Time (Risc-V Cycles): %lu\n", cnt);
    aux_reset_all_stats();

    int fail = 0;
    for (int i = 0; i < 1024; ++i) {
        if (result_array[i] != referenceresult[i]) {
            printf_error("[Verification Fail!] result[%i] = 0x%08x but reference = 0x%08x\n", i, result_array[i],
                         referenceresult[i]);
            fail++;
//        } else {
//            printf_success("[Verification Success!] result[%i] = 0x%08x and reference = 0x%08x\n", i, result_array[i],
//                         referenceresult[i]);
         }
        if (fail >= 10) break;
    }
    if (fail > 0){
        printf_error("[Verification Fail!] There were errors in executing the conv2d!");
    } else {
        printf_success("[Verification succeeded!] There were no errors in executing the conv2d!");
    }

    cnt = (((uint64_t(aux_get_sys_time_hi())) << 32) + uint64_t(aux_get_sys_time_lo()));
    printf("[Verification] Sys-Time (Risc-V Cycles): %li\n", cnt);

    printf("\nEnd");

    sim_stop();
    return 0;
}
