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
// # VPRO types                                           #
// ########################################################

#ifndef VPRO_CPP_STRUCTTYPES_HPP
#define VPRO_CPP_STRUCTTYPES_HPP

#define DEBUG_PRINT_STHPP 0

// C std libraries
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include <QString>

#include "vpro/vpro_cmd_defs.h"

struct addr_field_t {
    uint8_t sel;
    uint32_t offset;
    uint32_t alpha;
    uint32_t beta;
    uint32_t gamma;
    bool chain_left;
    bool chain_right;
    bool chain_ls;

    addr_field_t() {
        sel = SRC_SEL_ADDR;
        offset = 0;
        alpha = 0;
        beta = 0;
        gamma = 0;
        // chain_id = 0;
        chain_left = false;
        chain_right = false;
        chain_ls = false;
    }

    uint32_t getImm() {
        return (alpha << ISA_ALPHA_SHIFT_3D) + (beta << ISA_BETA_SHIFT_3D) +
               (gamma << ISA_GAMMA_SHIFT_3D) + (offset << ISA_OFFSET_SHIFT_3D);
    }

    uint32_t create_IMM(uint32_t src2_off = 0) {
        if (DEBUG_PRINT_STHPP) {
            printf("(addr_field_t) sel: %u, offset: %u, alpha: %u, beta: %u, gamma: %u\n",
                sel,
                offset,
                alpha,
                beta,
                gamma);
        }
        switch (sel) {
            case SRC_SEL_ADDR:
                return complex_ADDR_3D(SRC_SEL_ADDR, offset, alpha, beta, gamma);
            case SRC_SEL_IMM:
                // return SRC_IMM_3D(offset);
                return src2_off;
            case SRC_SEL_LS:
                return SRC_LS_3D;
            case SRC_SEL_LEFT:
                return SRC_CHAINING_3D(0);  // chain_id not important?!?!?!
            case SRC_SEL_RIGHT:
                printf("ERROR addr_field_t: create_IMM, sel unknown (%u)\n", sel);
                return -1;
            case SRC_SEL_INDIRECT_LS:
                return complex_ADDR_3D((uint32_t)SRC_SEL_INDIRECT_LS, 0, alpha, beta, gamma);
            case SRC_SEL_INDIRECT_LEFT:
                return complex_ADDR_3D((uint32_t)SRC_SEL_INDIRECT_LEFT, 0, alpha, beta, gamma);
            case SRC_SEL_INDIRECT_RIGHT:
                return complex_ADDR_3D((uint32_t)SRC_SEL_INDIRECT_RIGHT, 0, alpha, beta, gamma);
            default:
                printf("ERROR addr_field_t: create_IMM, sel unknown (%u)\n", sel);
                return -1;
        }
    }

    QString __toString() {
        return QString::number(sel, 2).leftJustified(3, ' ') + ", Off " +
               QString::number(offset).leftJustified(4, ' ') + ", a " +
               QString::number(alpha).leftJustified(2, ' ') + ", b " +
               QString::number(beta).leftJustified(2, ' ') + ", g " +
               QString::number(gamma).leftJustified(2, ' ') + ", chain left " +
               QString::number(chain_left) + ", right" + QString::number(chain_right) + ", ls" +
               QString::number(chain_ls);
    }
};

#endif  //VPRO_CPP_STRUCTTYPES_HPP
