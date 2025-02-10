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

// IO map
#include "vpro_host_aux.h"
#include "vpro_memory_map.h"

// UEMU libraries
#include <sys/stat.h>
#include <dma_common.h>
#include <mcpa.h>
#include <byteswap.h>

#include <cerrno>
#include <cstring>

// ***********************************************************************
// Transfer executable file
// ***********************************************************************
void send_executable(char const* file_name) {

  struct stat fs;
  FILE *pfh = NULL;
  uint8_t *buf;
  int num_bytes, num_bytes_int;

  pfh = fopen(file_name, "r");
  if (pfh == NULL) {
    printf("Unable to open file %s (%d, %s).\n", file_name, errno, strerror(errno));
    return;
  }

  fstat(fileno(pfh), &fs);
  num_bytes = (int)fs.st_size;
  num_bytes_int = num_bytes;

  while ((num_bytes_int & 7) != 0) { // must be a multiple of 8
    num_bytes_int++;
    printf("appending zero byte\n");
  }

  buf = (uint8_t*)malloc(num_bytes_int);
  memset((void*)buf, 0, num_bytes_int);

  int ret_ = fread(buf, 1, num_bytes, pfh);
  fclose(pfh);

  dma_common_write(buf, RAM_BASE_ADDR, num_bytes_int);

  free(buf);
}


// ***********************************************************************
// Simple data dump - get a single (valid) debug FIFO entry
// Blocking function!
// ***********************************************************************
int get_debug_fifo_entry() {

  int tmp = 0;
  unsigned char debug_buf_tmp[8];

  while(1){
    // get single word from fifo
    dma_common_read((char*) debug_buf_tmp, DEBUG_FIFO_ADDR, sizeof(debug_buf_tmp));
    if ((debug_buf_tmp[0] >> 7) != 0) { // valid flag set? (bit #63)
      tmp =       (debug_buf_tmp[4] << 24)  | (debug_buf_tmp[5] << 16);
      tmp = tmp | (debug_buf_tmp[6] <<  8)  | (debug_buf_tmp[7] <<  0);
      return tmp;
    }
  }
}


int get_mips_debug_instr_addr() {

  int tmp = 0;
  unsigned char debug_buf_tmp[8];

  while(1){
    // get single word from fifo
    dma_common_read((char*) debug_buf_tmp, MIPS_INSTR_ADDR, sizeof(debug_buf_tmp));
    if ((debug_buf_tmp[0] >> 7) != 0) { // valid flag set? (bit #63)
      tmp =       (debug_buf_tmp[4] << 24)  | (debug_buf_tmp[5] << 16);
      tmp = tmp | (debug_buf_tmp[6] <<  8)  | (debug_buf_tmp[7] <<  0);
      return tmp;
    }
  }
}

// ***********************************************************************
// Copy MIPS main memory region to binary file
// ***********************************************************************
void bin_file_dump(uint32_t addr, int num_bytes, char const* file_name) {

  int num_bytes_int = num_bytes;
  while ((num_bytes_int & 7) != 0) { // must be a multiple of 8
    num_bytes_int++;
  }

  FILE *pfh = NULL;

  uint8_t *buf;
  buf = (uint8_t*)malloc(num_bytes_int*sizeof(uint8_t));
  dma_common_read(buf, addr, num_bytes_int);

  pfh = fopen(file_name, "wb");
  if (pfh == NULL) {
    printf("Unable to open file %s (%d, %s).\n", file_name, errno, strerror(errno));
    return;
  }

  fwrite(buf, 1, num_bytes, pfh);

  fclose(pfh);
  free(buf);
}


// ***********************************************************************
// Copy MIPS main memory region to byte array
// ***********************************************************************
void bin_array_dump(uint32_t addr, int num_bytes, uint8_t* array_pnt) {

  int num_bytes_int = num_bytes;
  while ((num_bytes_int & 7) != 0) { // must be a multiple of 8
    num_bytes_int++;
  }

  uint8_t *buf;
  buf = (uint8_t*)malloc(num_bytes_int*sizeof(uint8_t));
  memset((void*)buf, 0, num_bytes_int);

  dma_common_read(buf, addr, num_bytes_int);

  memcpy((void*)array_pnt, (void*)buf, num_bytes);
  free(buf);
}


// ***********************************************************************
// Copy MIPS main memory region (2D-block) to byte array
// X, Y and stride refer to 32-bit elements!!!
// ***********************************************************************
void bin_array_dump32_2d(uint32_t addr, int x_size, int y_size, int x_stride, uint8_t* array_pnt) {

  int y_words = y_size;
  while ((y_words & 1) != 0) { // must be a multiple of 2
    y_words++;
  }

  int x_words = x_size;
  while ((x_words & 1) != 0) { // must be a multiple of 2
    x_words++;
  }

  int num_words = x_words*y_words;

  // allocate memory
  uint8_t *buf;
  buf = (uint8_t*)malloc(4*num_words*sizeof(uint8_t));

  // clear allocated memory buffer
  memset((void*)buf, 0, 4*num_words);

  // copy line by line to local buffer
  int y;
  for (y=0; y<y_size; y++){
    dma_common_read(buf+y*4*x_words, addr+y*4*(x_words+x_stride), 4*x_words);
  }

  memcpy((void*)array_pnt, (void*)buf, 4*x_size*y_size);

  free(buf);
}


// ***********************************************************************
// Copy MIPS main memory region (2D-block) to byte array
// X, Y and stride refer to 16-bit elements!!!
// ***********************************************************************
void bin_array_dump16_2d(uint32_t addr, int x_size, int y_size, int x_stride, uint8_t* array_pnt) {

  int y_words = y_size;
  while ((y_words & 1) != 0) { // must be a multiple of 2
    y_words++;
  }

  int x_words = x_size;
  while ((x_words & 1) != 0) { // must be a multiple of 2
    x_words++;
  }

  int num_words = x_words*y_words;

  // allocate memory
  uint8_t *buf;
  buf = (uint8_t*)malloc(2*num_words*sizeof(uint8_t));

  // clear allocated memory buffer
  memset((void*)buf, 0, 2*num_words);

  // copy line by line to local buffer
  int y;
  for (y=0; y<y_size; y++){
    dma_common_read(buf+y*2*x_words, addr+y*2*(x_words+x_stride), 2*x_words);
  }

  memcpy((void*)array_pnt, (void*)buf, 2*x_size*y_size);

  free(buf);
}


// ***********************************************************************
// Copy binary file to MIPS main memory
// ***********************************************************************
void bin_file_send(uint32_t addr, int num_bytes, char const* file_name) {

  FILE *pfh = NULL;
  uint8_t *buf;
  while ((num_bytes & 7) != 0) { // must be a multiple of 8
    num_bytes++;
  }
  buf = (uint8_t*)malloc(num_bytes*sizeof(uint8_t));
  memset((void*)buf, 0, num_bytes);

  pfh = fopen(file_name, "rb");
  if (pfh == NULL) {
    printf("Unable to open file %s (%d, %s).\n", file_name, errno, strerror(errno));
    return;
  }

  int ret_ = fread(buf, 1, num_bytes, pfh);

  fclose(pfh);

//    for (int i = 0; i < num_bytes ; i += 8) {
//        printf("Sending Data (64-bit) %i = 0x%lx\n", i, ((uint64_t *)(&(buf[i])))[0]);
//    }

  dma_common_write(buf, addr, num_bytes);
  free(buf);
}


// ***********************************************************************
// Copy binary file to MIPS main memory
// ***********************************************************************
void bin_array_send(uint32_t addr, int num_bytes, uint8_t* array_pnt) {

  uint8_t *buf;
  while ((num_bytes & 7) != 0) { // must be a multiple of 8
    num_bytes++;
  }
  buf = (uint8_t*)malloc(num_bytes*sizeof(uint8_t));
  memset((void*)buf, 0, num_bytes);
  memcpy((void*)buf, (void*)array_pnt, num_bytes);

  dma_common_write(buf, addr, num_bytes);
  free(buf);
}
void bin_array_send(uint32_t addr, int num_words, uint32_t* array_pnt) {

    uint8_t *buf;
    buf = (uint8_t*)malloc(num_words * sizeof(uint32_t));
    memset((void*)buf, 0, num_words * sizeof(uint32_t));

    for(int i = 0; i < num_words; i++){
        buf[i*4 + 0] = uint8_t(array_pnt[i] >> 24);
        buf[i*4 + 1] = uint8_t(array_pnt[i] >> 16);
        buf[i*4 + 2] = uint8_t(array_pnt[i] >> 8);
        buf[i*4 + 3] = uint8_t(array_pnt[i] >> 0);
    }
    bin_array_send(addr, num_words*sizeof(uint32_t), buf);
    free(buf);
}


// ***********************************************************************
// Set processor in reset state
// ***********************************************************************
void set_reset() {

  uint64_t ctrl_buf;

  ctrl_buf = 0x0000000000000000ULL;
  dma_common_write(&ctrl_buf, CTRL_REG_ADDR, 8);
}


// ***********************************************************************
// Release processor from reset state
// ***********************************************************************
void release_reset() {

  uint64_t ctrl_buf;

  ctrl_buf = 0x0100000000000000ULL;
  dma_common_write(&ctrl_buf, CTRL_REG_ADDR, 8);
}


// ***********************************************************************
// Get runtime from MIPS system ticker (blocking!)
// You need to ensure that the MIPS SYSTEM is moving the SYS_TIME vlaues
// in the correct order into the debug FIFO!
// -> LOW word first!
// ***********************************************************************
uint64_t get_system_runtime() {

  uint32_t lo = get_debug_fifo_entry();
  uint32_t hi = get_debug_fifo_entry();

  return (((uint64_t)hi)<<32) | ((uint64_t)lo);
}


#endif // vpro_host_aux_h
