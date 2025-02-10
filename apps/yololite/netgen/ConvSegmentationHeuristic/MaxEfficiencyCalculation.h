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
// Created by gesper on 12.06.23.
//

#ifndef CNN_CONVERTER_MAXEFFICIENCYCALCULATION_H
#define CNN_CONVERTER_MAXEFFICIENCYCALCULATION_H

#include <stdint.h>
#include <functional>

class MaxEfficiencyCalculation {

public:
    MaxEfficiencyCalculation(uint32_t inx, uint32_t iny, uint32_t inc, uint32_t outc, bool silent = false) :
            inx(inx), iny(iny), inc(inc), outc(outc), silent(silent) {}

    void runCalculation();

    [[nodiscard]] uint32_t get1DInputSize() const {
        return final.in_size;
    }

    [[nodiscard]] uint32_t getBlockCount() const {
        return final.block_count;
    }

    [[nodiscard]] uint32_t getParallelOutChannelsPerLane() const {
        return final.n;
    }

    [[nodiscard]] uint32_t getParallelInChannelsPerLane() const {
        return final.m;
    }

    [[nodiscard]] double getEfficiency() const {
        return final.efficiency;
    }

    [[nodiscard]] uint32_t getOvercalc() const {
        return final.overcalc;
    }

    static int getBlockCount(uint32_t blocksize, uint32_t in_size);

    static int getBlockOverlap(uint32_t blocksize, uint32_t blockcount, uint32_t in_size);

private:

    void runCalculationSegmentation(uint n, uint m, uint block_size, uint blockcount);

    bool PRINT_EVERY_TRY{true};
    bool PRINT_EVERY_TRY_IN_NEW_LINE{true};

    uint32_t inx{};
    uint32_t iny{};
    uint32_t in_size{};
    uint32_t inc{};
    uint32_t outc{};
    bool silent{false};

    uint32_t getBlockSizeUpperBound(uint32_t insize, uint32_t n, uint32_t m);

    uint32_t getBlockSizeLowerBound(uint32_t insize, uint32_t n, uint32_t m);

    struct config_t {
        uint32_t in_size{}; // block size
        uint32_t block_count{}; // block count to process all input pixels with in_size
        uint32_t n{};   // parallel out per lane
        uint32_t m{};
        uint32_t overcalc{};    // too much pixels in out size (when all blockCount * blockSize pixels are produced)
        double efficiency{};
    } final;

    double calc_eff{}, hw_eff{};
};


#endif //CNN_CONVERTER_MAXEFFICIENCYCALCULATION_H
