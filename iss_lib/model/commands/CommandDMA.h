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
// ########################################################
// # VPRO instruction & system simulation library         #
// # Sven Gesper, IMS, Uni Hannover, 2019                 #
// ########################################################
// # a class for commands to the DMA                      #
// ########################################################

#ifndef VPRO_CPP_COMMANDDMA_H
#define VPRO_CPP_COMMANDDMA_H

#include <QList>
#include <QString>
#include "CommandBase.h"

class CommandDMA : public CommandBase {
   public:
    enum PAD {
        TOP = 0,
        RIGHT = 1,
        BOTTOM = 2,
        LEFT = 3
    };

    enum TYPE {
        NONE,
        EXT_1D_TO_LOC_1D,
        EXT_2D_TO_LOC_1D,
        LOC_1D_TO_EXT_2D,
        LOC_1D_TO_EXT_1D,
        WAIT_FINISH,
        enumTypeEnd
    } type;

    uint32_t cluster_mask;
    QList<uint32_t> unit;

    uint64_t ext_base;
    uint32_t loc_base;

    int x, y;

    uint32_t x_size;
    int32_t y_leap;
    uint32_t y_size;

    bool pad[4]{};

    bool done;
    int id;

    CommandDMA();
    CommandDMA(CommandDMA* ref);
    CommandDMA(CommandDMA& ref);

    bool is_done() override;
    void print(FILE* out = stdout) override;

    // String returning functions
    QString get_type() override {
        switch (type) {
            case NONE:
                return {"NONE             "};
            case EXT_1D_TO_LOC_1D:
                return {"EXT_1D_TO_LOC_1D "};
            case EXT_2D_TO_LOC_1D:
                return {"EXT_2D_TO_LOC_1D "};
            case LOC_1D_TO_EXT_2D:
                return {"LOC_1D_TO_EXT_2D "};
            case LOC_1D_TO_EXT_1D:
                return {"LOC_1D_TO_EXT_1D "};
            case WAIT_FINISH:
                return {"WAIT_FINISH      "};
            default:
                return {"Unknown          "};
        }
    }

    static QString getType(CommandDMA::TYPE t);

    QString get_string() override;

    static void printType(CommandDMA::TYPE t, FILE* out = stdout) {
        switch (t) {
            case NONE:
                fprintf(out, "NONE             ");
                break;
            case EXT_1D_TO_LOC_1D:
                fprintf(out, "EXT_1D_TO_LOC_1D ");
                break;
            case EXT_2D_TO_LOC_1D:
                fprintf(out, "EXT_2D_TO_LOC_1D ");
                break;
            case LOC_1D_TO_EXT_2D:
                fprintf(out, "LOC_1D_TO_EXT_2D ");
                break;
            case LOC_1D_TO_EXT_1D:
                fprintf(out, "LOC_1D_TO_EXT_1D ");
                break;
            case WAIT_FINISH:
                fprintf(out, "WAIT_FINISH      ");
                break;
            default:
                fprintf(out, "Unknown          ");
        }
    }

    uint32_t get_direction();
};

#endif  //VPRO_CPP_COMMANDDMA_H
