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
// Created by gesper on 06.03.19.
//

#include "CommandDMA.h"

CommandDMA::CommandDMA() : CommandBase(DMA), unit() {
    type = CommandDMA::NONE;
    x_size = 0;
    y_size = 0;
    y_leap = 0;
    ext_base = 0;
    loc_base = 0;
    x = 0;
    y = 0;
    done = false;
    cluster_mask = 0;
    unit.clear();
    pad[0] = false;
    pad[1] = false;
    pad[2] = false;
    pad[3] = false;
}

CommandDMA::CommandDMA(CommandDMA* ref) : CommandBase(ref->class_type) {
    type = ref->type;
    x_size = ref->x_size;
    y_size = ref->y_size;
    y_leap = ref->y_leap;
    ext_base = ref->ext_base;
    loc_base = ref->loc_base;
    x = ref->x;
    y = ref->y;
    done = ref->done;
    unit = QList<uint32_t>(ref->unit);
    cluster_mask = ref->cluster_mask;
    pad[0] = ref->pad[0];
    pad[1] = ref->pad[1];
    pad[2] = ref->pad[2];
    pad[3] = ref->pad[3];
}

CommandDMA::CommandDMA(CommandDMA& ref) : CommandBase(ref.class_type) {
    type = ref.type;
    x_size = ref.x_size;
    y_size = ref.y_size;
    y_leap = ref.y_leap;
    ext_base = ref.ext_base;
    loc_base = ref.loc_base;
    x = ref.x;
    y = ref.y;
    done = ref.done;
    unit = QList<uint32_t>(ref.unit);
    cluster_mask = ref.cluster_mask;
    pad[0] = ref.pad[0];
    pad[1] = ref.pad[1];
    pad[2] = ref.pad[2];
    pad[3] = ref.pad[3];
}

bool CommandDMA::is_done() {
    return done || (type == NONE);
}

QString CommandDMA::getType(CommandDMA::TYPE t) {
    CommandDMA c;
    c.type = t;
    return c.get_type();
}

uint32_t CommandDMA::get_direction() {
    switch (type) {
        case EXT_1D_TO_LOC_1D:
        case EXT_2D_TO_LOC_1D:
            return 1;
        case LOC_1D_TO_EXT_2D:
        case LOC_1D_TO_EXT_1D:
            return 0;
        default:
            return -1;
    }
}

QString CommandDMA::get_string() {
    QString str;
    str += get_class_type();
    for (auto u : unit) {
        str += QString(", Unit: %1, ").arg(u);
    }
    switch (type) {
        case NONE:
            str += QString("Function: NONE");
            break;
        case EXT_1D_TO_LOC_1D:
            str += QString("Function: EXT_1D_TO_LOC_1D (%1 => %2)").arg(ext_base).arg(loc_base);
            str += QString(", size: %1 ").arg(x_size * y_size);
            break;
        case EXT_2D_TO_LOC_1D:
            str += QString("Function: EXT_2D_TO_LOC_1D (%1 => %2)").arg(ext_base).arg(loc_base);
            str += QString(", x_size: %1 (stride: %2), y_size: %3 ")
                       .arg(x_size)
                       .arg(y_leap)
                       .arg(y_size);
            if (pad[0] || pad[1] || pad[2] || pad[3]) {
                str += QString("Padding: %1 %2 %3 %4 ")
                           .arg((pad[0] ? "TOP" : ""))
                           .arg((pad[1] ? "RIGHT" : ""))
                           .arg((pad[2] ? "BOTTOM" : ""))
                           .arg((pad[3] ? "LEFT" : ""));
            }
            break;
        case LOC_1D_TO_EXT_2D:
            str += QString("Function: LOC_1D_TO_EXT_2D (%1 => %2)").arg(loc_base).arg(ext_base);
            str += QString(", x_size: %1 (stride: %2), y_size: %3 ")
                       .arg(x_size)
                       .arg(y_leap)
                       .arg(y_size);
            break;
        case LOC_1D_TO_EXT_1D:
            str += QString("Function: LOC_1D_TO_EXT_1D (%1 => %2)").arg(loc_base).arg(ext_base);
            str += QString(", size: %1 ").arg(x_size * y_size);
            break;
        case WAIT_FINISH:
            str += QString("Function: WAIT_FINISH");
            break;
        default:
            str += QString("Unknown Function");
    }
    return str;
}

void CommandDMA::print(FILE* out) {
    print_class_type(out);
    for (auto u : unit) {
        fprintf(out, ", Unit: %2i, ", u);
    }
    switch (type) {
        case NONE:
            fprintf(out, "Function: NONE");
            break;
        case EXT_1D_TO_LOC_1D:
            fprintf(out, "Function: EXT_1D_TO_LOC_1D (%li => %i)", ext_base, loc_base);
            fprintf(out, ", size: %i ", x_size * y_size);
            break;
        case EXT_2D_TO_LOC_1D:
            fprintf(out, "Function: EXT_2D_TO_LOC_1D (%li => %i)", ext_base, loc_base);
            fprintf(out, ", x_size: %i (stride: %i), y_size: %i ", x_size, y_leap, y_size);
            fprintf(out,
                ", PAD_TOP: %i, PAD_LEFT: %i, PAD_RIGHT: %i, PAD_BOTTOM: %i ",
                pad[CommandDMA::PAD::TOP],
                pad[CommandDMA::PAD::LEFT],
                pad[CommandDMA::PAD::RIGHT],
                pad[CommandDMA::PAD::BOTTOM]);
            break;
        case LOC_1D_TO_EXT_2D:
            fprintf(out, "Function: LOC_1D_TO_EXT_2D (%i => %li)", loc_base, ext_base);
            fprintf(out, ", x_size: %i (stride: %i), y_size: %i ", x_size, y_leap, y_size);
            break;
        case LOC_1D_TO_EXT_1D:
            fprintf(out, "Function: LOC_1D_TO_EXT_1D (%i => %li)", loc_base, ext_base);
            fprintf(out, ", size: %i ", x_size * y_size);
            break;
        case WAIT_FINISH:
            fprintf(out, "Function: WAIT_FINISH");
            break;
        default:
            fprintf(out, "Unknown Function");
    }
    fprintf(out, "\n");
}
