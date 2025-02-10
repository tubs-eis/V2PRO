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

#ifndef VPRO_PROJECT_DEFAULT_NAME_VECTORLANELS_H
#define VPRO_PROJECT_DEFAULT_NAME_VECTORLANELS_H

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

#include "../../commands/CommandVPRO.h"
#include "VectorLane.h"

namespace Unit {

// to be included in cpp
class VectorUnit;

class VectorLaneLS : public VectorLane {
   public:
    explicit VectorLaneLS(VectorUnit* vector_unit,
        std::shared_ptr<ArchitectureState> architecture_state);

    bool isLSLane() const override {
        return true;
    };

    bool* getzeroflag() override;
    bool* getnegativeflag() override;
    uint8_t* getregister() override;

    void dumpRegisterFile(std::string prefix) override;
};

}  // namespace Unit

#endif  //VPRO_PROJECT_DEFAULT_NAME_VECTORLANELS_H
