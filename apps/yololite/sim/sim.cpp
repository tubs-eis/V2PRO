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
#include <string>
#include "riscv/eisV_hardware_info.hpp"
#include "vpro_functions.h"
#include "segment_scheduling.h"
#include "calc_cnn.h"

/**
 * Print the cycle counter after each layer
 * -> slows down (not sim)
 */
#define RV_PRINT_LAYER_CYCLE_DETAILS 0

/**
 * do some evaluation + printing in the end of inference (efficiency calc, per layer eval)
 * -> slows down (not sim)
 */
#define RV_EVAL 0
#if RV_EVAL==1
#include "extended_stats.hpp"
#endif


void pre_layer_hook(int layer_exec_idx, int total_layers, const BIF::LAYER *layer) {
#if RV_EVAL==1
    pre_layer_stat_update(layer_exec_idx, total_layers, layer, RV_PRINT_LAYER_CYCLE_DETAILS);
#endif
}

void post_layer_hook(int layer_exec_idx, int total_layers, const BIF::LAYER *layer) {
#if RV_EVAL==1
    post_layer_stat_update(layer_exec_idx, total_layers, layer, RV_PRINT_LAYER_CYCLE_DETAILS);
#endif
}


//----------------------------------------------------------------------------------
//----------------------------------Main--------------------------------------------
//----------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    sim_init(main, argc, argv);

    aux_print_hardware_info("CNN sim", versionVersion, completeVersion);

    // check HW config of this cnn's App config
    //sim_min_req(VPRO_CFG::CLUSTERS, VPRO_CFG::UNITS, VPRO_CFG::LANES); @DEPRECATED

    BIF::NET *net;
    {
      // copy eisvblob from simulator memory into host (x86) memory
      uint32_t eisvblob_addr = 0x06000000;
      int copysize = 0;
      {
        BIF::NET net;
        
        for (int i = 0; i < 8; i++) {
          core_->dbgMemRead(eisvblob_addr + i, ((uint8_t*)&net) + i);
        }
        std::cout << "net.magicword = " << net.magicword << "\n";
        std::cout << "net.blobsize = " << net.blobsize << "\n";
        assert(net.magicword == BIF::net_magicword && "Magicword mismatch");
        copysize = net.blobsize;
      }

      net = (BIF::NET *)malloc(copysize);
      for (int i = 0; i < copysize; i++) {
        core_->dbgMemRead(eisvblob_addr + i, ((uint8_t*)net) + i);
      }
      
    }

    vpro_set_cluster_mask(0xFFFFFFFF);
    vpro_set_unit_mask(0xFFFFFFFF);

    initOvercalcMemSim();

    // reset DCMA to load new input into cache
    dcma_reset();
        
    uint64_t totalclock = calcCnn(net, RV_PRINT_LAYER_CYCLE_DETAILS);

    // include dcma flush cycles in profiling
    dcma_flush();
    
    unsigned int clockfreq_mhz = int(1000 / core_->getRiscClockPeriod());
    print_cnn_stats(totalclock, clockfreq_mhz);
    

    aux_print_debugfifo(0xbeefdead);
    aux_print_debugfifo(0xbeef0000); // really dead!
    sim_stop();
    
    exit(0);

}
