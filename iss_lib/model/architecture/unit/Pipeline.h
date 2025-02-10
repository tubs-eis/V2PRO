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
 * @file Pipeline.h
 * @author Alexander Köhne, Sven Gesper
 * @date 2021
 *
 * Outsourced pipeline for vector lanes
 */

#ifndef HEADER_PIPELINE
#define HEADER_PIPELINE

#include <memory>
#include <vector>

#include "../../commands/CommandVPRO.h"

namespace Unit {

class VectorLane;  //fw declaration

struct PipelineDate {
    // result (got in stage of alu finish)
    uint32_t data;
    // in sim the result is calc in one cycle / loaded in same. stored (tmp) here
    uint32_t pre_data;
    std::shared_ptr<CommandVPRO> cmd;

    uint32_t opa, opb, opc;
    uint32_t res;
    int rf_addr, lm_addr;
    bool move;

    bool flag[2];

    PipelineDate() {
        data = 0;
        opa = 0;
        opb = 0;
        opc = 0;
        rf_addr = 0;
        lm_addr = 0;
        move = false;
        cmd = std::make_shared<CommandVPRO>();
        flag[0] = false;
        flag[1] = false;
        res = 0;
    };
};

class PipeObject {
   public:
    // mac accu
    uint64_t accu;

    // location of min / max instruction
    uint32_t minmax_index;
    uint32_t minmax_value;

    // depth of current pipeline (in which step the wb is executed, may change with new commands...)
    // read in tick stage pipelinedepth + 5 (WB)
    // updated in tick() when a different pipeline depth cmd is received (stage 0/-1)
    int pipelineALUDepth;

    //int pipeline_depth; //@Obsolete

    PipeObject() = delete;
    PipeObject(int size, int pipelineALUDepth);
    PipelineDate& operator[](int index);
    const PipelineDate& operator[](int index) const;
    void resetAccu(int64_t value = 0);
    void resetMinMax(uint32_t value);
    virtual void tick_pipeline(VectorLane& vl, int from, int until);
    void process(std::shared_ptr<CommandVPRO> newElement);
    void processInStall(int from);
    bool isBusy() const;
    void update();
    bool isChaining() const;
    bool isBlocking(int chain_target_stage) const;
    void check_indirect_addressing(CommandVPRO cmd);

   protected:
    // the processing needs PIPELINE_DEPTH cycles. each cycle the last entry from pipeline is written into rf_data!
    // on parsing a new command, a new pipeline entry is inserted with currently read data from rf
    // PipelineDate results_pipeline[11];
    std::vector<PipelineDate> data;

   private:
    virtual void execute_cmd(VectorLane& vl, PipelineDate& pipe);
};

class LSPipeObject : public PipeObject {
   public:
    void tick_pipeline(VectorLane& vl, int from, int until) override;
    LSPipeObject() = delete;
    LSPipeObject(int size, int pipelineALUDepth) : PipeObject(size, pipelineALUDepth) {}

   private:
    void execute_cmd(VectorLane& vl, PipelineDate& pipe) override;
};

}  // namespace Unit

#endif
