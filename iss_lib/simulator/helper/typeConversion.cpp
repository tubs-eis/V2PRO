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

#include "typeConversion.h"
#include "debugHelper.h"

// ***********************************************************************
// Dual logarithm (unsigned integer!)
// ***********************************************************************
uint32_t ld(uint32_t x) {
    if (x == 0) {
        printf_error("VPRO_SIM ERROR: Invalid log2 argument! (%d)\n", x);
        return 0;
    }

    uint32_t i = 0;

    x = x >> 1;
    while (x != 0) {
        x = x >> 1;
        i++;
    }

    return i;
}

// ***********************************************************************
// Size/format conversions
// ***********************************************************************
uint32_t* __8to24(uint32_t& val) {
    val = val & 0xFF;
    return &val;
}

uint32_t* __8to24signed(uint32_t& val) {
    val = val & 0xFF;

    // if negative
    if (val & 0x00000080) val = val | 0xFFFFFF00;

    val = val & 0x00FFFFFF;
    return &val;
}

uint32_t* __16to24(uint32_t& val) {
    val = val & 0x00FFFF;
    return &val;
}

uint32_t* __16to24signed(uint32_t& val) {
    // if negative
    if (val & 0x00008000) val = val | 0xffff0000;

    val = val & 0x00FFFFFF;
    return &val;
}

uint32_t* __18to32signed(uint32_t& val) {
    val = val & 0x0003ffff;

    // if negative
    if (val & 0x00020000) val = val | 0xfffc0000;

    return &val;
}

// for immediates
uint32_t* __22to32signed(uint32_t& val) {
    val = val & 0x003fffff;

    // if negative
    if (val & 0x00200000) val = val | 0xffc00000;

    return &val;
}

uint64_t* __24to64signed(uint64_t& val) {
    val = val & 0x0000000000ffffff;

    // if negative
    if (val & 0x00800000) val = val | 0xffffffffff000000;

    return &val;
}

uint32_t* __24to32signed(uint32_t& val) {
    val = val & 0x00ffffff;

    // if negative
    if (val & 0x00800000) val = val | 0xff000000;

    return &val;
}

uint32_t* __32to24(uint32_t& val) {
    val = val & 0x00ffffff;
    return &val;
}

uint32_t* __32to10(uint32_t& val) {
    val = val & 0x000003ff;
    return &val;
}

// ***********************************************************************
// Flag helper
// ***********************************************************************
bool __is_zero(uint32_t& val) {
    if ((val & 0x00ffffff) == 0)
        return 1;
    else
        return 0;
}

bool __is_zero(uint8_t* val) {
    return (val[0] == 0 && val[1] == 0 && val[2] == 0);
}

bool __is_negative(uint32_t& val) {
    if ((val & 0x00800000) == 0)
        return 0;
    else
        return 1;
}

bool __is_negative(uint8_t* val) {
    if ((val[2] & 0x80) == 0)
        return 0;
    else
        return 1;
}
