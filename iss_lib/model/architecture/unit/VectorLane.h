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
/**
 * @file VectorLane.h
 * @author Alexander Köhne, Sven Gesper, IMS, Uni Hannover, 2019
 * @date Created by Gesper, 2019
 *
 * Architecture vector lane
 */

#ifndef VPRO_CPP_VECTORLANE_H
#define VPRO_CPP_VECTORLANE_H

// C std libraries
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <string>
#include <vector>

#include <iostream>
#include <tuple>
#include "../../../simulator/setting.h"
#include "../../commands/CommandVPRO.h"
#include "../ArchitectureState.h"
#include "../HFIFO.h"
#include "../RegisterFile.h"
#include "Pipeline.h"

namespace Unit {

// to be included in cpp
class VectorUnit;

class VectorLane {
   public:
    HFIFO<std::tuple<word_t, bool, bool>> fifo =
        HFIFO<std::tuple<word_t, bool, bool>>(2u);  //Final: 2

    int vector_lane_id;
    std::shared_ptr<ArchitectureState> architecture_state;

    RegisterFile::Register regFile;

    bool updated;

    VectorLane(int id,
        VectorUnit* vector_unit,
        std::shared_ptr<ArchitectureState> architecture_state);

    virtual void tick();
    // take _nxt values and put into register
    void update();

    void check_src_lane_stall();
    void check_dst_lane_stall();
    void check_adr_lane_stall();
    void check_stall_conditions();

    //todo dont know if the next line is correct
    bool is_lane_chain_fifo_ready() const {
        return not fifo._is_empty();
    }

    bool is_dst_lane_stalling() const {
        return dst_lane_stall;
    }
    bool is_src_lane_stalling() const {
        return src_lane_stall;
    }

    virtual void dumpRegisterFile(std::string prefix = "");

    std::shared_ptr<CommandVPRO> getCmd();

    bool isBusy() const;

    bool isBlocking() const;

    VectorLane& getLeftNeighbor() const {
        return *left_lane;
    }
    VectorLane& getRightNeighbor() const {
        return *right_lane;
    }
    VectorLane& getLSLane() const {
        return *ls_lane;
    }

    void setNeighbors(std::shared_ptr<VectorLane> left,
        std::shared_ptr<VectorLane> right,
        std::shared_ptr<VectorLane> ls) {
        left_lane = left;
        right_lane = right;
        ls_lane = ls;
    };

    virtual uint8_t* getregister() {
        return regFile.getregister();
    }

    virtual bool* getzeroflag() {
        return regFile.getzeroflag();
    }

    virtual bool* getnegativeflag() {
        return regFile.getnegativeflag();
    }

    bool is_src_chaining(addr_field_t src) const;
    bool is_indirect_addr(addr_field_t adr) const;

    virtual bool isLSLane() const {
        return false;
    };

    void print_pipeline(const QString prefix = "");
    void fetchCMD();
    void processCMD();

    uint64_t* getAccu() {
        return &(pipeObj->accu);
    }

   protected:
    long clock_cycle;

    int consecutive_DST_stall_counter;
    int consecutive_SRC_stall_counter;
    int consecutive_ADR_stall_counter;

    // reference to parent vector unit (needed for local mem and cmd queue acccess)
    VectorUnit* vector_unit;
    std::shared_ptr<CommandVPRO> current_cmd, new_cmd, noneCmd;

    bool blocking, blocking_nxt;

    // Neighbors
    std::shared_ptr<VectorLane> left_lane;
    std::shared_ptr<VectorLane> right_lane;
    std::shared_ptr<VectorLane> ls_lane;

    std::unique_ptr<PipeObject> pipeObj;

    // flag whether to stall, waiting for src lane chaining output to become rdy
    bool src_lane_stall;
    // flag whether to stall, waiting for src lane chaining offset to become rdy
    bool adr_lane_stall;

    // stalls until dst is read, released by getOperand/get_pipeline_chain_data.
    // two entries for sim. if sim of other lane is one clock cycle behind. stall is eval for [1].
    // access depending on cycle offset
    bool dst_lane_stall;  // tmp for use during tick (parallel)
    // bool chain_DSTisRdy, chain_DSTisRdy_nxt; // tmp for use during tick (parallel)

    bool lane_chaining{}, lane_chaining_nxt{};
    bool dst_lane_ready;

    // in this stage, the chain input is read
    int chain_target_stage = 3;

    // resolve address into data (3*8 bit = 24 bit datawidth)
    virtual std::tuple<uint32_t, bool, bool> get_operand(
        addr_field_t src, uint32_t x, uint32_t y, uint32_t z);

    VectorLane& get_lane_from_src(addr_field_t src);

   private:
    bool additional_check();

    void updateDebugMsg();
    void check_stall_conditions_msg(const int pipeline_src_read_stage);
    void check_stall_conditions_msg1() const;
    void check_stall_conditions_msg2(addr_field_t const& src);
    void get_operand_debug_chaining_msg(VectorLane const& src) const;

    VectorLane* who();
    void whoami() const;
    void whoami(VectorLane const* v) const;
    void whoami(addr_field_t const& src);
    void print_fifo() const;

    friend class PipeObject;
    friend class LSPipeObject;
};

}  // namespace Unit

#endif  // VPRO_CPP_VECTORLANE_H
