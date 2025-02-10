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

const int16_t kernel_sharpen[9]={0, -1,0, -1, 5, -1, 0, -1, 0};
const int16_t kernel_outline[9]={-1,-1,-1,-1,8,-1,-1, -1, -1};
const int16_t kernel_laplace[9]={0,1,0,1,-4,1,0,1,0};
const int16_t kernel_sobel_x[9]={-1,0,1,-2,0,2,-1,0,1};

/**
 * Main
 */
int main(int argc, char *argv[]) {
    sim_init(main, argc, argv);
    sim_printf("Tutorial App");
    sim_printf("Clusters: %i, units: %i, lanes: %i", VPRO_CFG::CLUSTERS, VPRO_CFG::UNITS, VPRO_CFG::LANES);

    // broadcast to all
    vpro_set_cluster_mask(0xFFFFFFFF);
    vpro_set_unit_mask(0xFFFFFFFF);

    // DMA Instructions
//    dma_ext2D_to_loc1D(0, 0, LM_BASE_VU(0) + 0, 224-8+1, 8, 8);
    dma_wait_to_finish(0xffffffff);

    // VPRO Instructions
//    VPRO::DIM2::LOADSTORE::loads(0,
//                                 0, 1, 8,
//                                 7, 7);
//
//    VPRO::DIM2::PROCESSING::add(L0,
//                                DST_ADDR(0, 1, 8), SRC1_LS_2D, SRC2_IMM_2D(0),
//                                7, 7);
//
//    VPRO::DIM2::PROCESSING::add(L0,
//                                DST_ADDR(0, 1, 8), SRC1_ADDR(0, 1, 8), SRC2_IMM_2D(0),
//                                7, 7, true);
//
//    VPRO::DIM2::LOADSTORE::store(0,
//                                 0, 1, 8,
//                                 7, 7,
//                                 L0);

    vpro_wait_busy(0xffffffff, 0xffffffff);

    // DMA Instructions
//    dma_loc1D_to_ext2D(0, 0, LM_BASE_VU(0) + 0, 224-8+1, 8, 8);
    dma_wait_to_finish(0xffffffff);

    // finish
    dcma_flush();
    sim_stop();
    return 0;
}
