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
 * @file setting.h
 *
 * Simulation Settings
 */

#ifndef SIM_SETTINGS
#define SIM_SETTINGS

#include <QDir>
#include "vpro/vpro_globals.h"

/**
 * whether to copy the file (created by cmake) globals.section to mm start (address = 0)
 */
constexpr bool copyGlobalSectionToMMStart = false;

/**
 * whether to wait for a keypress when the debughelper has printed a printf_error message
 */
constexpr bool PAUSE_ON_ERROR_PRINT = false;
constexpr bool PAUSE_ON_WARNING_PRINT = false;

/**
 * In the pipeline WAR conflict can occur
 * if after a Write command with small vector length (e.g. ADD writes to RF [0]: x_end and y_end build vector lenght 1)
 * a reading command uses the register written before. The Write Command is still inside the ALU pipeline.
 * It would read a wrong data word... [ERROR]
 *
 * - if activated, simulation speed drops by ~30%
 */
constexpr bool checkForWARConflict = false;

/**
 * whether to check if x/y is large enough to fill the whole pipeline
 * if not, following instructions may use wrong input data, cause the rf hasnt got the results yet
 */
constexpr bool checkVectorPipelineLength = false;

/**
 * enables resuming of execution trough gui via button
 */
constexpr bool GUI_WAITKEY = true;

/**
 * defines register size for loop values
 */
constexpr int MAX_LOOP_CASCADES = 2;
constexpr int MAX_LOOP_INSTRUCTIONS = 12;

/**
 * defines size of each units command queue
 */
constexpr int UNITS_COMMAND_QUEUE_MAX_SIZE = 32;

/**
 * print number of simulated cycles once per second (e.g.: "Simulated Cycles in the last 989ms: 102205 [103341 Cycles/s]")
 * influenced massively by simulated architecture size!
 * 1C 1U simulates with >300.000 Cycles/s, -> 8C 1U simulates with ~10.000 Cycles/s
 */
constexpr bool simulationSpeedMeasurement = true;

/**
 * Whether to seperate Cluster, Units or Lanes into threads...
 * more sense on high content tick functions (cluster...)
 *
 * constexpr bool THREAD_CLUSTER = true;   // if #cluster  >= 2 && units   >= 4    (more means more tick time)
 *
 * no big influence on simulated cycles / s !??? => maybe some bug somewhere? should speed it up...
 */
//#define THREAD_CLUSTER

/**
 * Log files for CMD history (
 */
constexpr bool CREATE_CMD_HISTORY_FILE = false;
constexpr char CMD_HISTORY_FILE_NAME[] = "../log/sim_cmd_history.log";

constexpr bool CREATE_CMD_PRE_GEN = false;
constexpr char CMD_PRE_GEN_FILE_NAME[] = "../log/pregen.bin";

class QFile;
extern QFile* CMD_HISTORY_FILE;
class QTextStream;
extern QTextStream* CMD_HISTORY_FILE_STREAM;

// sim_exit()
const QString dumpFileSuffix = QString::number(VPRO_CFG::CLUSTERS) + "C" +
                               QString::number(VPRO_CFG::UNITS) + "U" +
                               QString::number(VPRO_CFG::LANES) + "L";
const QString dumpFileName = "../statistics/statistic_detail_" + dumpFileSuffix + ".log";
const QString dumpJSONFileName = "../statistics/statistics_detail_" + dumpFileSuffix + ".json";

extern QFile* PRE_GEN_FILE;
class QDataStream;
extern QDataStream* PRE_GEN_FILE_STREAM;

/**
 * Log files for PRE_GEN history
 * first entry in each struct is an identifier:
 *  0 -> vpro sync  1  * 4 byte
 *  1 -> dma  sync  1  * 4 byte
 *  2 -> vpro cmd   5  * 4 byte
 *  3 -> dma  cmd   11 * 4 byte                     //@TODO use COMMAND_DMA
 */

#ifdef USE_OTHER_DATATYPS
#else
#ifndef __word_t
typedef uint32_t word_t;
#define __word_t uint32_t
#endif

#endif

#endif
