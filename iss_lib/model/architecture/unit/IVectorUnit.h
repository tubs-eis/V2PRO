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
 * Common Interface for all implementations of vector unit
*/
#ifndef I_VECTOR_UNIT_H
#define I_VECTOR_UNIT_H

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "../../commands/CommandVPRO.h"
#include "VectorLane.h"
#include "VectorLaneLS.h"

namespace Unit {

class IVectorUnit {
   public:
    // Unit general interface
    [[nodiscard]] virtual int id() = 0;
    [[nodiscard]] virtual std::vector<std::shared_ptr<VectorLane>>& getLanes() = 0;
    [[nodiscard]] virtual VectorLaneLS& getLSLane() = 0;
    virtual void tick() = 0;
    virtual void update() = 0;
    [[nodiscard]] virtual bool isBusy() = 0;

    // Command Queue interface
    [[nodiscard]] virtual bool isCmdQueueFull() = 0;
    [[nodiscard]] virtual bool trySendCMD(std::shared_ptr<CommandVPRO> const& cmd) = 0;
    virtual std::deque<std::shared_ptr<CommandVPRO>> getCopyOfCommandQueue() = 0;

    // Local memory interface
    virtual uint8_t* getLocalMemoryPtr() = 0;
    [[nodiscard]] virtual uint32_t getLocalMemoryData(uint32_t addr, int size = 2) = 0;
    virtual void writeLocalMemoryData(uint32_t addr, uint32_t data, int size = 2) = 0;
    virtual void writeLocalMemoryData(const uint32_t& addr, const uint8_t* data, int size = 2) = 0;

    // Debug helpers
    virtual void dumpLocalMemory(const std::string& prefix = "") = 0;
    virtual void dumpRegisterFile(uint32_t lane) = 0;
    virtual void dumpQueue() = 0;
};

}  // namespace Unit

#endif
