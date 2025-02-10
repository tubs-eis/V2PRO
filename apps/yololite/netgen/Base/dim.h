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
// Created by gesper on 20.07.23.
//

#ifndef NETGEN_DIM_H
#define NETGEN_DIM_H

namespace CNN_LAYER {

    class Dim {
    public:
        Dim() = default;

        Dim(int x, int y, int ch) : x(x), y(y), ch(ch) {}

        // algorithm-view geometry
        int x{}; // width in elements
        int y{}; // height in elements
        int ch{}; // number of channels

        // Implementation-view main memory layout
        // Reason: algorithm-view geometry may not be divisable into segments without remainder
        // ==> memory layout extended to integer multiple of segment size (garbage at the right and bottom)
        // Background: DMA transfers from LM to MM are individual per unit anyway, but can not be used to skip data from LM.
        // LM layout of border segments shall be the same as all other segments to allow instruction broadcasting to all units.
        struct {
            mm_addr_type x{}; // width in elements (including garbage right of image) = y-stride
            mm_addr_type y{}; // height in elements (including garbage below image)
            // number of channels always identical to payload
            mm_addr_type base{}; // byte address of reserved memory, set by memory management via setOutputMMAddr(); useful payload data starts at channel_base[0] (there may be pre-garbage)
            mm_addr_type size{}; // size in bytes to be requested from memory management ; not necessarily payload size
            std::vector <mm_addr_type> channel_base{}; // byte address per channel
            mm_addr_type ch_size{}; // bytes per channel including right/bottom garbage
            bool layout_known{false};
        } mm;

        double fixedpoint_scaling{0}; // divide integer data by this to obtain the floating-point values it represents
        
        bool algoEqual(Dim &ref) {
            return x == ref.x && y == ref.y && ch == ref.ch;
        }

        std::string baseStr() {
            return std::string(mmAddrStr(mm.base));
        }

        std::string chBaseStr(int ch = 0) {
            return std::string(mmAddrStr(mm.channel_base[ch]));
        }

        std::string algoStr() {
            return "whc " + std::to_string(x) + "x" + std::to_string(y) + "x" + std::to_string(ch);
        }

        std::string mmStr() {
            return "whc " + std::to_string(mm.x) + "x" + std::to_string(mm.y) + "x" + std::to_string(ch);
        }

        std::string algoMmStr() {
            // "whc 17x9x3, mem 19x19x3 @ 0x81000000"
            std::string irregular = "";
            for (int i = 1; i < ch; i++) {
                if ((mm.channel_base[i] - mm.channel_base[i-1]) != mm.ch_size) {
                    irregular = " !! IRREGULAR MEM LAYOUT, file I/O will fail !!";
                    break;
                }
            }
            return algoStr() + ", mem " + std::to_string(mm.x) + "x" + std::to_string(mm.y) + "x" + std::to_string(ch) + " @ " + chBaseStr() + irregular;
        }

        std::string detailStr() {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(16) << fixedpoint_scaling;
            return algoMmStr() + ", allocated " + std::to_string(mm.size) + " byte @ " + baseStr() + " .. " + std::string(mmAddrStr(mm.base + mm.size - 1)) + ", fp-scaling " + ss.str();
        }
    }; // class dim

} // namespace CNN_LAYER

#endif //NETGEN_DIM_H
