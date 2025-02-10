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
#include <cassert>
#include "segment_scheduling.h"
#include <eisv.h>
#include <vpro.h>
#include "calc_cnn.h"

char versionVersion[] = {
        VERSION_MAJOR_INIT, '.', VERSION_MINOR_INIT, '\0'
};
char completeVersion[] = {
        BUILD_YEAR_CH0, BUILD_YEAR_CH1, BUILD_YEAR_CH2, BUILD_YEAR_CH3,
        '-', BUILD_MONTH_CH0, BUILD_MONTH_CH1, '-', BUILD_DAY_CH0, BUILD_DAY_CH1,
        ' ', BUILD_HOUR_CH0, BUILD_HOUR_CH1,
        ':', BUILD_MIN_CH0, BUILD_MIN_CH1, ':', BUILD_SEC_CH0, BUILD_SEC_CH1, '\0'
};

uint64_t calcCnn(BIF::NET *bnet, bool per_layer_stats) {
  printf("=================== CNN execution from binary ===================\n");

  uint64_t totalclock = 0;
  aux_reset_all_stats();

  assert((bnet->magicword == BIF::net_magicword) && "Magicword mismatch");
  // layer execution list
  if (per_layer_stats) {
    printf("layers = %d\n", bnet->layer_execlist_count);
  }
  for (unsigned int xli = 0; xli < bnet->layer_execlist_count; xli++) { // eXecution-List Index
    unsigned int lbi = ((uint32_t*)(((uint8_t*)bnet) + (bnet->layer_execlist_offs)))[xli];
    assert((lbi < bnet->layer_count) && "Layer execution list entry is larger than number of available layers");

    BIF::LAYER* layer = (BIF::LAYER*)(((uint8_t*)bnet) + (bnet->bif_layer_offs[lbi]));
    const BIF::COMMAND_SEGMENT *commandSegments = &layer->command_segments[0];

    if (per_layer_stats) {
      printf("layer_execlist[%3d] = %3d: layer (%3d), type %s, %6d COMMAND_SEGMENTs @ %p\n",
             xli, lbi, layer->number, to_char(layer->type), layer->command_segments_count, (void*)commandSegments);
    }

    // HW: communicate with host
    pre_layer_hook(xli, bnet->layer_execlist_count, layer);
    
    // reset profiling
    if (per_layer_stats) {
      aux_reset_all_stats();
    }

    aux_clr_sys_time();
    calcLayer(*layer, commandSegments, layer->command_segments_count);
    vpro_sync();    // make shure sync is really done (double sync), as sync is not blocking any more (Sync Feature Update, 09.2023)
    uint32_t endclock = aux_get_sys_time_lo();

    // HW: communicate with host
    post_layer_hook(xli, bnet->layer_execlist_count, layer);

    totalclock += endclock;
    if (per_layer_stats) {
      aux_print_statistics();
      printf("Stats have been reset befor this Layer!\n");
      printf("\tRisc Clock\t Layer: %" PRId32 ", \tAccumulated: %" PRId64 "\n", endclock, totalclock);
    }
  }

  return totalclock;
}

void print_cnn_stats(uint64_t totalclock, unsigned int clockfreq_mhz) {
  aux_print_statistics(totalclock);

  unsigned int us_per_run = totalclock / clockfreq_mhz;
  assert(us_per_run > 0);
  unsigned int fps = 1000000 / us_per_run;
  unsigned int fps_frac = (100000000 / us_per_run) - fps * 100;

  unsigned int ms = us_per_run / 1000;
  unsigned int ms_frac = (us_per_run / 10) - ms * 100;

  printf("CNN Inference Completed! \n");
  printf("\tRisc-V Clock Cycles: %" PRId64 ", [%i MHz: %u,%02i ms, %i,%02i FPS]\n", totalclock, clockfreq_mhz,
         ms, ms_frac , fps, fps_frac);
  
}
