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
// # VPRO instruction & system simulation library         #
// # Sven Gesper, IMS, Uni Hannover, 2019                 #
// ########################################################
// # type conversion between datas in vpro architecure    #
// # e.g., 24-bit 18-bit ...                              #
// #       interpretation as uint/int32...                #
// #       for memory convert to arrays of uint8          #
// ########################################################

#ifndef VPRO_CPP_TYPECONVERSION_H
#define VPRO_CPP_TYPECONVERSION_H

// C std libraries
#include <inttypes.h>
#include <stdint.h>

uint32_t ld(uint32_t x);
uint32_t* __8to24(uint32_t& val);
uint32_t* __8to24signed(uint32_t& val);
uint32_t* __16to24(uint32_t& val);
uint32_t* __16to24signed(uint32_t& val);
uint32_t* __18to32signed(uint32_t& val);
uint32_t* __22to32signed(uint32_t& val);
uint64_t* __24to64signed(uint64_t& val);  // for c ref
uint32_t* __24to32signed(uint32_t& val);
uint32_t* __32to24(uint32_t& val);
uint32_t* __32to10(uint32_t& val);

// ***********************************************************************
// Flag helper
// ***********************************************************************
bool __is_zero(uint32_t& val);
bool __is_zero(uint8_t* val);
bool __is_negative(uint32_t& val);
bool __is_negative(uint8_t* val);

#endif  //VPRO_CPP_TYPECONVERSION_H
