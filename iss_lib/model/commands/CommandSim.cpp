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
// Created by gesper on 28.03.19.
//

#include "CommandSim.h"

CommandSim::CommandSim() : CommandBase(SIM) {
    type = CommandSim::NONE;

    addr = 0;
    num_bytes = 0;
    file_name = new char[0];
    value = 0;
    data = 0;
    cluster = 0;
    unit = 0;
    lane = 0;
    string = new char[0];
}
CommandSim::CommandSim(CommandSim* ref) : CommandBase(ref->class_type) {
    type = ref->type;

    addr = ref->addr;
    num_bytes = ref->num_bytes;
    file_name = ref->file_name;
    value = ref->value;
    data = ref->data;
    cluster = ref->cluster;
    unit = ref->unit;
    lane = ref->lane;
    string = ref->string;
}
CommandSim::CommandSim(CommandSim& ref) : CommandBase(ref.class_type) {
    type = ref.type;

    addr = ref.addr;
    num_bytes = ref.num_bytes;
    file_name = ref.file_name;
    value = ref.value;
    data = ref.data;
    cluster = ref.cluster;
    unit = ref.unit;
    lane = ref.lane;
    string = ref.string;
}

bool CommandSim::is_done() {
    return true;
}

QString CommandSim::get_string() {
    QString str = QString("");
    str += get_class_type();
    str += QString(", Func: ");
    str += get_type();
    switch (type) {
        case NONE:
            break;
        case BIN_FILE_SEND:
        case BIN_FILE_DUMP:
            str += QString("Addr: %1 (%2 bytes) from file: %3")
                       .arg(addr)
                       .arg(num_bytes)
                       .arg(file_name);
            break;
        case BIN_FILE_RETURN:
            str += QString("Addr: %1 (%2 bytes)").arg(addr).arg(num_bytes);
            break;
        case AUX_MEMSET:
            str += QString("Addr: %1 (%2 bytes) to value %3").arg(addr).arg(num_bytes).arg(value);
            break;
        case DUMP_LOCAL_MEM:
        case DUMP_REGISTER_FILE:
            str += QString("Cluster %1, Unit %2, Lane %3").arg(cluster).arg(unit).arg(lane);
            break;
        case PRINTF:
            str += QString("String: %1").arg(string);
            break;
        case AUX_PRINT_FIFO:
        case AUX_DEV_NULL:
            str += QString("Value: %1").arg(value);
            break;
        default:
            str += QString("Unknown Function");
    }
    return str;
}

void CommandSim::print(FILE* out) {
    print_class_type(out);
    fprintf(out, ", Func: ");
    printType(type, out);
    switch (type) {
        case NONE:
            break;
        case BIN_FILE_SEND:
        case BIN_FILE_DUMP:
            fprintf(out, "Addr: %i (%i bytes) from file: %s", addr, num_bytes, file_name);
            break;
        case BIN_FILE_RETURN:
            fprintf(out, "Addr: %i (%i bytes)", addr, num_bytes);
        case AUX_MEMSET:
            fprintf(out, "Addr: %i (%i bytes) to value %i", addr, num_bytes, value);
            break;
        case DUMP_LOCAL_MEM:
        case DUMP_REGISTER_FILE:
            fprintf(out, "Cluster %i, Unit %i, Lane %i", cluster, unit, lane);
            break;
        case PRINTF:
            fprintf(out, "String: %s", string);
            break;
        case AUX_PRINT_FIFO:
        case AUX_DEV_NULL:
            fprintf(out, "Value: %i", value);
            break;
        default:
            fprintf(out, "Unknown Function");
    }
}
