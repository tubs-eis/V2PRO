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
// Created by gesper on 24.03.22.
//

#ifndef COMMON_VPRO_COMMANDS_H
#define COMMON_VPRO_COMMANDS_H

#include "../vpro/vpro_defs.h"
#include "../vpro/vpro_cmd_defs.h"

#ifndef SIMULATION
#include "../vpro/vpro_aux.h"
#include "../vpro/vpro_asm.h"
#include "../vpro/dma_asm.h"
#else
// requires gcc-flag: -I TOOLS/VPRO/ISS/isa_intrinsic_lib/
//             cmake: target_include_directories(target PUBLIC ...)
#include <iss_aux.h>
#include <simulator/helper/debugHelper.h> // printf_...
#include <core_wrapper.h> // __vpro()
#endif

#include "vpro_special_register_enums.h"

namespace VPRO {
    extern bool has_printed_load_reverse_warning; /** Print Warning in ISS if VPRO Command LOAD_REVERSE is used */
    extern bool has_printed_divl_warning;         /** Print Warning in ISS if VPRO Command DIVL is used */
    extern bool has_printed_divh_warning;         /** Print Warning in ISS if VPRO Command DIVH is used */
}; // namespace VPRO

#include "VPRO_2D.h"
#include "VPRO_3D.h"

#endif //COMMON_VPRO_COMMANDS_H
