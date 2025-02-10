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
// Created by gesper on 01.10.19.
//

#include <cmath>
#include <iostream>
#include <tuple>

#include "../../../simulator/ISS.h"
#include "../../../simulator/helper/debugHelper.h"
#include "../../../simulator/helper/typeConversion.h"
#include "../Cluster.h"
#include "VectorLaneLS.h"
#include "VectorUnit.h"

#include "../RegisterFile.h"

namespace Unit {

VectorLaneLS::VectorLaneLS(VectorUnit* vector_unit,
    std::shared_ptr<ArchitectureState> architecture_state)
    : VectorLane(log2(int(LS)), vector_unit, architecture_state) {
    pipeObj = std::make_unique<LSPipeObject>(5 + CommandVPRO::MAX_ALU_DEPTH, 3);
}

//***********************************************************//
//                  Overload of Not Used Function            //
//***********************************************************//
uint8_t* VectorLaneLS::getregister() {
    printf_error("getregister on L/S LANE called! No RF exist!");
    return nullptr;
}
bool* VectorLaneLS::getzeroflag() {
    printf_error("getzeroflag on L/S LANE called! No RF exist!");
    return nullptr;
}
bool* VectorLaneLS::getnegativeflag() {
    printf_error("getnegativeflag on L/S LANE called! No RF exist!");
    return nullptr;
}
void VectorLaneLS::dumpRegisterFile(std::string prefix) {
    printf_error("dumpRegisterFile on L/S LANE called! No RF exist!");
}

}  // namespace Unit
