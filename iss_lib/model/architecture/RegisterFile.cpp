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
#include "RegisterFile.h"

#include <sstream>
#include <string>
#include "../../simulator/helper/debugHelper.h"
#include "../../simulator/helper/typeConversion.h"
#include "Cluster.h"

namespace RegisterFile {

void Register::initArrays() {
    if (!is_disabled("initArrays", false)) {
        rf_data = new uint8_t[register_file_size * 3]();
        rf_flag[0] = new bool[register_file_size]();
        rf_flag[1] = new bool[register_file_size]();
        is_uninitialized = new bool[register_file_size]();
        for (int i = 0; i < register_file_size; ++i) {
            is_uninitialized[i] = true;
        }
    }
    if (!isLS && rf_generate_trace) {
        std::stringstream ss;
        ss << "c" << cluster_id << "u" << vector_unit_id << "l" << vector_lane_id << "rf.trace";
        openTraceFile(ss.str().c_str());
    }
}

bool Register::is_disabled(std::string func, bool print_msg) const {
    if (register_file_size <= 0 || isLS) {
        if (print_msg)
            printf_error(
                ">>> RF was disabled (lane: %i) executed by; %s\n", vector_lane_id, func.c_str());
        return true;
    }
    return false;
}

/**
     * on accessing data from the register file. checks adress
     * @param addr rf-address
     * @param size [optional] = 3 for 24-bit
     * @return uint8_t[3] data
     */
uint32_t Register::get_rf_data(int addr, int size) const {
    // check if this accesses a valid register file
    if (is_disabled("get_rf_data")) {
        return 0;
    }
    if (addr * 3 + size > 3 * register_file_size) {
        printf_error(
            "[Get] Access Register File out of range! (lane: %i, addr: %d, access size: %d)\n",
            vector_lane_id,
            addr,
            size);
        return 0;
    }
    if (size != 3) {
        printf_error("RF Read for size != 3 [NOT IMPLEMENTED!]\n");
    }

    if (is_uninitialized[addr]) {
        printf_error("[RF] Uninitialized Access! (RF Addr: %i, Cluster: %i, Unit: %i, Lane: %i)\n",
            addr,
            cluster_id,
            vector_unit_id,
            vector_lane_id);
    }
    assert(!is_uninitialized[addr]);

    auto a = (*((uint32_t*)&rf_data[addr * 3])) & 0x00ffffffu;

    return a;
}

Register::Register(
    int cluster_id, int vector_unit_id, int vector_lane_id, int register_file_size, bool isLS)
    : cluster_id(cluster_id),
      vector_unit_id(vector_unit_id),
      register_file_size(register_file_size),
      vector_lane_id(vector_lane_id),
      isLS(isLS),
      rf0_inst(0),
      rf1_inst(1) {
    if (!isLS) initArrays();
}

Register::~Register() {
    if (!is_disabled("Destructor", false)) {
        delete rf_data;
        delete rf_flag[0];
        delete rf_flag[1];
    }
    if (!isLS && rf_generate_trace) {
        trace.flush();
        trace.close();
    }
}

/**
     * on setting data from the register file. checks adress
     * @param addr rf-address
     * @param data uint8_t[3] data
     * @param size [optional] = 3 for 24-bit
     */
void Register::set_rf_data(int addr, uint32_t data, int size) {
    // check if this accesses a valid register file
    if (is_disabled("set_rf_data")) {
        return;
    }
    if (addr * 3 + size > 3 * register_file_size) {
        printf_error(
            "[Set] Access Register File out of range! (lane: %i, addr: %d, access size: %d)\n",
            vector_lane_id,
            addr,
            size);
        return;
    }

    if (rf_generate_trace) {
        trace << "RF[" << std::dec << addr << "] ";
        if (is_uninitialized[addr]) {
            trace << "uuuuuu";
        } else {
            is_uninitialized[addr] = false;
            trace << std::uppercase << std::setfill('0') << std::setw(6) << std::hex
                  << (get_rf_data(addr, size) & 0xffffff);
        }
        trace << " > ";
        trace << std::uppercase << std::setfill('0') << std::setw(6) << std::hex
              << (data & 0xffffff);
        trace << "\n";
    }

    is_uninitialized[addr] = false;

    for (int i = 0; i < size; i++) {
        rf_data[addr * 3 + i] = uint8_t(data >> (i * 8));
    }
}

bool Register::get_rf_flag(int addr, int select) const {
    if (is_disabled("get_rf_flag")) {
        return false;
    }
    if (addr > register_file_size) {
        printf_error("RF Flag Address access out of range; (addr: %i > %i)[Select; %i]",
            addr,
            register_file_size,
            select);
        return false;
    }
    if (select > 1) {
        printf_error("RF Flag Select access out of range; (addr: %i > %i)[Select; %i]",
            addr,
            register_file_size,
            select);
        return false;
    }
    return rf_flag[select][addr];
}

void Register::set_rf_flag(int addr, int select, bool value) {
    if (is_disabled("set_rf_flag")) {
        return;
    }
    if (addr > register_file_size) {
        printf_error("RF Flag Address access out of range; (addr: %i > %i)[Select; %i]",
            addr,
            register_file_size,
            select);
        return;
    }
    if (select > 1) {
        printf_error("RF Flag Select access out of range; (addr: %i > %i)[Select; %i]",
            addr,
            register_file_size,
            select);
        return;
    }
    rf_flag[select][addr] = value;
}

Register::Register(Register const& reg)
    : Register(reg.cluster_id,
          reg.vector_unit_id,
          reg.vector_lane_id,
          reg.register_file_size,
          reg.isLS) {
    if (!reg.is_disabled("Constructor")) {
        memcpy(rf_data, reg.rf_data, sizeof(int) * reg.register_file_size * 3);
        memcpy(rf_flag[0], reg.rf_flag[0], sizeof(int) * reg.register_file_size);
        memcpy(rf_flag[1], reg.rf_flag[1], sizeof(int) * reg.register_file_size);
    }
}

void Register::update() {
    if (rf_inst.nxt) {
        set_rf_data(rf_inst.addr, rf_inst.data, rf_inst.size);
        rf_inst.nxt = false;
    }
    if (rf0_inst.nxt) {
        set_rf_flag(rf0_inst.addr, rf0_inst.flag, rf0_inst.data);
        rf0_inst.nxt = false;
    }
    if (rf1_inst.nxt) {
        set_rf_flag(rf1_inst.addr, rf1_inst.flag, rf1_inst.data);
        rf1_inst.nxt = false;
    }
}

void Register::set_rf_data_nxt(int addr, uint32_t data, int size) {
    if (rf_inst.nxt) printf_warning("[RegisterFile]: Override RF Nxt Value\n");
    rf_inst.addr = addr;
    rf_inst.data = data;
    rf_inst.size = size;
    rf_inst.nxt = true;
}

void Register::set_rf_flag_nxt(int addr, int select, bool value) {
    if (select == 0) {
        if (rf0_inst.nxt) printf_warning("[RegisterFile]: Override RF0 Nxt Flag\n");
        rf0_inst.addr = addr;
        rf0_inst.data = value;
        rf0_inst.nxt = true;
    } else if (select == 1) {
        if (rf1_inst.nxt) printf_warning("[RegisterFile]: Override RF1 Nxt Flag\n");
        rf1_inst.addr = addr;
        rf1_inst.data = value;
        rf1_inst.nxt = true;
    } else {
        printf_error("[RegisterFile]: Select out of range - %d\n", select);
    }
}

void Register::openTraceFile(const char* tracefilename) {
    trace = std::ofstream(tracefilename);
}

}  // namespace RegisterFile
