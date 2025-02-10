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
// #################################################
// # VPRO - Host helper functions                  #
// # --------------------------------------------- #
// # Stephan Nolting, IMS, Uni Hannover, 2017-2018 #
// #################################################

#ifndef vpro_host_aux_h
#define vpro_host_aux_h

#include <stdint.h>

// Function prototypes
void send_executable(char const* file_name);
int  get_debug_fifo_entry();
void bin_file_dump(uint32_t addr, int num_bytes, char const* file_name);
void bin_array_dump(uint32_t addr, int num_bytes, uint8_t* array_pnt);
void bin_array_dump32_2d(uint32_t addr, int x_size, int y_size, int x_stride, uint8_t* array_pnt);
void bin_array_dump16_2d(uint32_t addr, int x_size, int y_size, int x_stride, uint8_t* array_pnt);
void bin_file_send(uint32_t addr, int num_bytes, char const * file_name);
void bin_array_send(uint32_t addr, int num_bytes, uint8_t* array_pnt);
void bin_array_send(uint32_t addr, int num_words, uint32_t* array_pnt);
void set_reset();
void release_reset();
uint64_t get_system_runtime();


#endif // vpro_host_aux_h
