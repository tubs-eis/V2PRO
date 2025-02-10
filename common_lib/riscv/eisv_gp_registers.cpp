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
//
// Created by gesper on 06.04.22.
//

#include "eisv_gp_registers.h"
#include <stdint.h>
#include <stdio.h>

#ifdef SIMULATION
#include "core_wrapper.h"
#endif

bool GPR::write32(uint32_t register_nr, uint32_t data) {
    if (register_nr >= 256 or (register_nr & 0b11) != 0) {
        // need to be in GP Register Range
        // need to address one 32-bit register (dividable by 4 address)
        printf("GPR::write32 out of range! Given: %u\n", (unsigned int)register_nr);
        return false;
    }
#ifdef SIMULATION
    io_write(GP_REGISTERS_ADDR+register_nr, data);
#else
    (*((volatile uint32_t *) ((uint32_t) (intptr_t) (&GP_REGISTERS) + register_nr))) = data;
#endif
    return true;
}

bool GPR::write16(uint32_t register_nr, uint16_t data) {
    printf("GPR::write16 not yet implemented. TODO: read and write 16-bit\n");
    return false;
}

bool GPR::write8(uint32_t register_nr, uint8_t data) {
    printf("GPR::write8 not yet implemented. TODO: read and write 8-bit\n");
    return false;
}

uint32_t GPR::read32(uint32_t register_nr) {
    if (register_nr >= 256 or (register_nr & 0b11) != 0) {
        printf("GPR::READ32 out of range! Given: %u\n", (unsigned int)register_nr);
        return 0;
    }

#ifdef SIMULATION
    return io_read(GP_REGISTERS_ADDR+register_nr);
#else
    return (*((volatile uint32_t *) ((uint32_t) (intptr_t) (&GP_REGISTERS) + register_nr)));
#endif
//        printf("Read from: 0x%x\n", ((uint32_t)(intptr_t)(&GP_REGISTERS) + register_nr));
//        return (*((volatile uint32_t*) &GP_REGISTERS + register_nr/4));
}
