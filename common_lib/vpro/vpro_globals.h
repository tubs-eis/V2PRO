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
/**
 * @file config.h
 * @author Alexander Köhne
 * @date June 2023
 *
 * This header shall contain if given a mapping from Makro to constexpr functions otherwise it shall provide a default value
 *
 * Targets mostly the SIMULATION
 */
#ifndef GUARD_VPRO_CONFIG
#define GUARD_VPRO_CONFIG

#include <cstdint>
/*
 * Keep in mind
 * Won't Read the definitions because cmake wont pass these arguments
 * Maybe use CMake for File Creation
 */

// General Config
#ifndef CONF_CLUSTERS
#define CONF_CLUSTERS 1
#endif

#ifndef CONF_UNITS
#define CONF_UNITS 1
#endif

#ifndef CONF_LANES
#define CONF_LANES 2
#endif

//
#ifndef CONF_MM_SIZE
// 1024*1024*1024*4u
#define CONF_MM_SIZE 4294967296lu
#endif

#ifndef CONF_LM_SIZE
#define CONF_LM_SIZE 8192
#endif

#ifndef CONF_RF_SIZE
#define CONF_RF_SIZE 1024
#endif

// DCMA Config
#ifndef CONF_DCMA_LINE_SIZE
#define CONF_DCMA_LINE_SIZE 4096
#endif

#ifndef CONF_DCMA_ASSOCIATIVITY
#define CONF_DCMA_ASSOCIATIVITY 4
#endif

#ifndef CONF_DCMA_NR_RAMS
#define CONF_DCMA_NR_RAMS 8
#endif

#ifndef CONF_DCMA_RAM_SIZE
#define CONF_DCMA_RAM_SIZE 524288
#endif

#ifndef CONF_ALU_WIDTH
#define CONF_ALU_WIDTH 24
#endif


namespace VPRO_CFG {
    constexpr unsigned int CLUSTERS = CONF_CLUSTERS;
    constexpr unsigned int UNITS = CONF_UNITS;
    constexpr unsigned int LANES = CONF_LANES;
    constexpr unsigned int parallel_Lanes = CLUSTERS*UNITS*LANES;
    constexpr uint64_t     MM_SIZE = CONF_MM_SIZE;
    constexpr unsigned int LM_SIZE = CONF_LM_SIZE;
    constexpr unsigned int RF_SIZE = CONF_RF_SIZE;

    constexpr unsigned int DCMA_LINE_SIZE = CONF_DCMA_LINE_SIZE; // in bytes
    constexpr unsigned int DCMA_ASSOCIATIVITY = CONF_DCMA_ASSOCIATIVITY;
    constexpr unsigned int DCMA_NR_BRAMS = CONF_DCMA_NR_RAMS;
    constexpr unsigned int DCMA_BRAM_SIZE = CONF_DCMA_RAM_SIZE; // in bytes

    // Configuration related to how the HW is simulated
    namespace SIM {
        constexpr bool STRICT = true;
    }

    // Till here HW

    /*
    namespace ALU {
        constexpr unsigned int WIDTH = CONF_ALU_WIDTH;
        constexpr uint32_t MASK = (uint32_t)((1 << WIDTH + 1) - 1);
        constexpr unsigned int OPB_MAC_MUL_LEN = 18;
        constexpr uint32_t OPB_MASK = (uint32_t) ((1 << OPB_MAC_MUL_LEN )-1);
    }
    */

}

/*
struct HW_CONFIG{
    int CLUSTERS{NUM_CLUSTERS};
    int UNITS{NUM_VU_PER_CLUSTER};
    int LANES{NUM_VECTORLANES};
    int parallel_Lanes{NUM_CLUSTERS*NUM_VU_PER_CLUSTER*NUM_VECTORLANES};
    int MM_SIZE{CONF_MM_SIZE};
    int LM_SIZE{CONF_LM_SIZE};
    int RF_SIZE{CONF_RF_SIZE};


    unsigned int DMA_DATAWORD_LENGTH_BYTE  {16 / 8};
    unsigned int DCMA_DATAWORD_LENGTH_BYTE {64 / 8};
    unsigned int BUS_DATAWORD_LENGTH_BYTE  {512 / 8};

    int DCMA_LINE_SIZE{CONF_DCMA_LINE_SIZE}; // in bytes
    int DCMA_ASSOCIATIVITY{CONF_DCMA_ASSOCIATIVITY};
    int DCMA_NR_BRAMS{CONF_DCMA_NR_RAMS};
    int DCMA_BRAM_SIZE{CONF_DCMA_BRAM_SIZE}; // in bytes
    unsigned int RF_WIDTH{24};
    unsigned int LM_WIDTH{16}; //MAX 32

    double RISC_CLOCK_PERIOD{6.66};
    double RISC_IO_ACCESS_CYCLES{6};
    double AXI_CLOCK_PERIOD{10};
    double DCMA_CLOCK_PERIOD{10};
    double VPRO_CLOCK_PERIOD{4};
    double DMA_CLOCK_PERIOD{10};

};

inline constexpr HW_CONFIG HW;*/

#endif
