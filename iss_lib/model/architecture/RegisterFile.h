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
#ifndef HEADER_REGISTERFILE
#define HEADER_REGISTERFILE

#include <fstream>
#include <string>
#include "../../simulator/helper/typeConversion.h"

namespace RegisterFile {

struct RegisterInstructionVal {
    bool nxt = false;
    int addr;
    uint32_t data;
    int size;
    RegisterInstructionVal() {}
};

struct RegisterInstructionFlag {
    int flag;
    bool nxt = false;
    int addr;
    bool data;
    RegisterInstructionFlag(int flag) : flag(flag) {}
};

class Register {
   public:
    Register() = delete;
    Register(
        int cluster_id, int vector_unit_id, int vector_lane_id, int register_file_size, bool isLS);
    Register(Register const& reg);
    ~Register();

    uint8_t* getregister() {
        return rf_data;
    }
    bool* getzeroflag() {
        return rf_flag[0];
    }
    bool* getnegativeflag() {
        return rf_flag[1];
    }

    uint32_t get_rf_data(int addr, int size = 3) const;
    void set_rf_data(int addr, uint32_t data, int size = 3);
    void set_rf_data_nxt(int addr, uint32_t data, int size = 3);
    bool get_rf_flag(int addr, int select) const;
    void set_rf_flag(int addr, int select, bool value);
    void set_rf_flag_nxt(int addr, int select, bool value);
    void update();

   private:
    int cluster_id, vector_unit_id;

    uint8_t*
        rf_data;  // each lane has a register file. 3 elements of this 8-bit array form an element of the rf! [0 - LSB][1][2 - MSB]
    bool* rf_flag[2];  // each register file entry has two additional flags: is_zero, is_negative

    bool* is_uninitialized;  // each register in the begin is not initialized!

    int register_file_size;

    int vector_lane_id;
    bool isLS;

    RegisterInstructionVal rf_inst;

    RegisterInstructionFlag rf0_inst;
    RegisterInstructionFlag rf1_inst;

    bool rf_generate_trace{false};
    std::ofstream trace;

    bool is_disabled(std::string func, bool print_msg = true) const;
    void initArrays();

    void openTraceFile(const char* tracefilename);
};

}  // namespace RegisterFile

#endif
