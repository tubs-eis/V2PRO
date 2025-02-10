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
#ifndef SIMULATION // hack: sim builds all files from runtime/

#include <stdint.h>
#include <eisv.h>
#include <vpro.h>
#include "calc_cnn.h"

#include "riscv/riscv-csr.hpp"
using namespace riscv::csr;

/**
 * Print Hardware/Software Date + Config when app starts
 * -> slows down initial startup (especially simulation)
 */
#include "riscv/eisV_hardware_info.hpp"
#define PRINT_HEADER

/**
 * Print the cycle counter after each layer
 * -> slows down
 */
#define RV_PRINT_LAYER_CYCLE_DETAILS 0

/**
 * Execute one inference.
 * Default ARM parameters (General purpose register) allow immediate start of this inference.
 * Exit is called after one inference; exit(0xcafe).
 * -> Should only be set for hardware simulation/Virtual Prototype
 */
#define run_only_one_iteration 1

/**
 * let risc-v do some evaluation + printing in the end of inference (efficiency calc, per layer eval)
 * -> slows down
 */
#define RV_EVAL 1
#if RV_EVAL==1
#include "extended_stats.hpp"
#endif

// policy: all communication with ARM takes place here (and not in calc_cnn etc.)

void pre_layer_hook(int layer_exec_idx, int total_layers, const BIF::LAYER *layer) {
    // wait until ARM does not need output of last run before overwriting it
    if (layer->first_layer_producing_output) {
        while (GPR::read32(arm_output_parsed) == 0 && GPR::read32(rv_running) != 0) {
            // wait for output to be parsed from arm, aux_wait_cycles(2);
        }
        GPR::write32(arm_output_parsed, 0); // reset it
    }
#if RV_EVAL==1
    pre_layer_stat_update(layer_exec_idx, total_layers, layer, RV_PRINT_LAYER_CYCLE_DETAILS);
#endif
}

void post_layer_hook(int layer_exec_idx, int total_layers, const BIF::LAYER *layer) {
#if RV_EVAL==1
    post_layer_stat_update(layer_exec_idx, total_layers, layer, RV_PRINT_LAYER_CYCLE_DETAILS);
#endif
    if (layer->last_layer_using_input) {
        // first layer done -> allow overwriting of input data
        // FIXME only valid if input data only required by first layer
        GPR::write32(rv_input_parsed, 1);
    }
}

//----------------------------------------------------------------------------------
//----------------------------------Main--------------------------------------------
//----------------------------------------------------------------------------------

// reserve space for eisvblob; address fixed in linker script
uint8_t __attribute__ ((aligned (32))) __attribute__ ((section (".eisvblob"))) net;

int main(int argc, char *argv[]) {
#ifdef PRINT_HEADER
    aux_print_hardware_info("CNN runtime"); // FIXME uses std::string -> ld: undefined reference to `__dso_handle'
#endif

    // check HW config of this cnn's App config
    sim_min_req(VPRO_CFG::CLUSTERS, VPRO_CFG::UNITS, VPRO_CFG::LANES);

    vpro_set_cluster_mask(0xFFFFFFFF);
    vpro_set_unit_mask(0xFFFFFFFF);

    unsigned int clockfreq_mhz = get_gpr_risc_freq() / 1000 / 1000;

    /**
     * MAIN LOOP to process images
     *
     * risc-v waits until arm_input_ready is set
     *  arm_input_ready set by arm
     *   arm_input_ready gets reset by eis-v
     *   rv_input_parsed set by eis-v
     *    rv_input_parsed gets reset by arm
     *
     * eis-v waits until arm_output_parsed is set
     *  arm_output_parsed gets reset by eis-v
     *  eis-v sets rv_output_ready
     *   rv_output_ready gets reset by arm
     *    arm_output_parsed set by arm
     */
#if run_only_one_iteration == 0
    while (GPR::read32(rv_running) != 0) {
#endif

    mcountinhibit_ops::write(0xffff);
    printf("\nMain loop is running. waiting for input ready...\n");
    mcountinhibit_ops::write(0x0000);

    // wait for input data, aux_wait_cycles(2);
    while (GPR::read32(arm_input_ready) == 0 && GPR::read32(rv_running) != 0) {}
    GPR::write32(arm_input_ready, 0);

    // reset DCMA to load new input into cache
    dcma_reset();

    // start CNN
    uint64_t totalclock = calcCnn((BIF::NET *) &net, RV_PRINT_LAYER_CYCLE_DETAILS);

    // include dcma flush cycles in profiling
    dcma_flush();

    print_cnn_stats(totalclock, clockfreq_mhz);

    GPR::write32(rv_output_ready, 1);

#if run_only_one_iteration == 0
//    GPR::write32(arm_input_ready, 1);   // force another round
//    GPR::write32(arm_output_parsed, 1);

    /**
     * continue with next input
     */
  } // rv running endless loop
#else
// for hardware simulation
// dump results
// dump 1103578 bytes from 0x81000000
    *((volatile uint32_t *) (0xbeef0000)) = 0x81000000;
    *((volatile uint32_t *) (0xbeef1000)) = 0x10D6DA; // = 1103578 bytes
#endif

    aux_print_debugfifo(0xbeefdead);
    aux_print_debugfifo(0xbeef0000); // really dead!
    printf("############################\n[END] CNN Finished. Exiting Application.. [Pls Return to me a 0xcafe].\n");

    // sim trigger dump
    // using block ram's dump feature
    //    *((volatile uint32_t *) (0x3fff0000)) = 0x11000000; // base
    //    *((volatile uint32_t *) (0x3fff1000)) = 0x01000000; // size + trigger
    exit(0xcafe); // return to crt0.asm and loop forever
}


#endif // SIMULATION


