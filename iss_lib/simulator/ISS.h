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
// # Main entry for simulation.                           #
// # -> Init                                              #
// # -> call commands for vpro                            #
// # -> stop                                              #
// ########################################################

#ifndef sim_core_class
#define sim_core_class

// C std libraries
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef ISS_STANDALONE
#include <functional>
// FK: Added function object for registering callback
// this is used to route DMA memory accesses outside of the ISS
using callback_function = std::function<void(bool, uint64_t, uint32_t&, uint32_t)>;
using call_systemc_stop_cb = std::function<void(void)>;
#else

#include "../model/architecture/NonBlockingMainMemory.h"

#endif

#include <QCoreApplication>
#include <QDebug>
#include <QMutex>
#include <QThread>
#include <QTime>
#include <QTimer>

#include "../model/architecture/ArchitectureState.h"
#include "../model/architecture/Cluster.h"
#include "../model/architecture/DCMA.h"
#include "../model/architecture/NonBlockingBusSlaveInterface.h"
#include "../model/commands/CommandBase.h"
#include "../model/commands/CommandDMA.h"
#include "../model/commands/CommandSim.h"
#include "../model/commands/CommandVPRO.h"
#include "windows/Commands/commandwindow.h"

#include "VectorMain.h"
#include "helper/mathHelper.h"
#include "model/architecture/DMABlockExtractor.h"

#include <vpro/vpro_cmd_defs.h>
#include <vpro/vpro_special_register_enums.h>

static bool default_flags[4] = {false, false, false, false};

class DMALooper;

class ISS : public QThread {
    // to be able to receive signals from the windows (qt based widgets) & to send signals to them (update of visualized data)
    Q_OBJECT

   public:
    CommandWindow* w;
    QCoreApplication* application;

#ifndef ISS_STANDALONE
    // FK: Added function object for registering callback
    // this is used to route DMA memory accesses outside of the ISS
    callback_function m_cb;
    call_systemc_stop_cb m_cb_systemc_stop;
#endif

    /**
     *  (=clock) counters for simulated io counters accessed by ISS
     *
     * (vpro clock, in riscv-domain: *1/4!) counters for simulated io counters accessed by ISS
     * *_total = clock
     * lane_act, dma_act or both_act = unit busy cycles
     * riscv_enable = riscv not in sync loop cycles
     */
    uint64_t aux_cnt_lane_act{}, aux_cnt_dma_act{}, aux_cnt_both_act{}, aux_cnt_vpro_total{},
        aux_cnt_riscv_total{}, aux_cnt_riscv_enabled{}, aux_sys_time{}, aux_cycle_counter{};

    NonBlockingBusSlaveInterface* bus = NULL;
    DCMA* dcma;

    bool DMA_gen_trace{false};

    std::ofstream dma_sync_trace;

    ISS();

#ifndef ISS_STANDALONE
    void register_memory_access_function(callback_function cb);
    void register_call_systemc_stop(call_systemc_stop_cb cb);
#endif

    int sim_init(int (*main_fkt)(int, char**), int& argc, char* argv[]);

    /**
     * IO Interface (Read and Write)
     * - generate VPRO Commands
     * - generate DMA Commands
     * - read/write sync/busy status, counters, config registers
     */
    uint32_t io_read(uint32_t addr);
    void io_write(uint32_t addr, uint32_t value);

    void risc_counter_tick();

    void setWaitingToFinish(bool waiting) {
        riscv_sync_waiting = waiting;
    }

    int bin_file_send(uint32_t addr, int num_bytes, char const* file_name);

    int bin_file_dump(uint32_t addr, int num_bytes, char const* file_name);

    uint8_t* bin_file_return(uint32_t addr, int num_bytes);

    uint16_t* get_main_memory(uint32_t addr, unsigned int num_bytes);

    uint8_t* bin_lm_return(uint32_t cluster, uint32_t unit);

    uint8_t* bin_rf_return(uint32_t cluster, uint32_t unit, uint32_t lane);

    void aux_memset(uint32_t base_addr, uint8_t value, uint32_t num_bytes);

    // *** Simulator debugging funtions ***
    void sim_dump_local_memory(uint32_t cluster, uint32_t unit);

    void sim_dump_queue(uint32_t cluster, uint32_t unit);

    void sim_dump_register_file(uint32_t cluster, uint32_t unit, uint32_t lane);

    void sim_wait_step(bool finish = false, char const* msg = "");

    void check_vpro_instruction_length(const std::shared_ptr<CommandVPRO>& command);

    std::vector<Cluster*>& getClusters() {
        return clusters;
    };

    [[nodiscard]] DMALooper const* getDmaLooper() const {
        return dmalooper;
    };

    [[nodiscard]] const double& getTime() const {
        return time;
    }

    [[nodiscard]] const double& getAxiClockPeriod() const {
        return axi_clock_period;
    }

    [[nodiscard]] const double& getRiscClockPeriod() const {
        return risc_clock_period;
    }

    [[nodiscard]] const double& getDMAClockPeriod() const {
        return dma_clock_period;
    }

    [[nodiscard]] const double& getDCMAClockPeriod() const {
        return dcma_clock_period;
    }

    [[nodiscard]] const double& getVPROClockPeriod() const {
        return vpro_clock_period;
    }

    void run_vpro_instruction(const std::shared_ptr<CommandVPRO>& command);
    void run_dma_instruction(const std::shared_ptr<CommandDMA>& command, bool skip_tick = false);

    void clk_tick();  // Must be public for Wrapper

    void openTraceFile(const char* tracefilename);

   private:
    bool windowThread;
    QThread simulatorThread;
    VectorMain* simulatorThreadProgram;

    bool isCompletelyInitialized;
    atomic<bool> isPrintResultDone;

    uint32_t dma_access_counter_cluster_pointer = 0;

    struct io_vpro_cmd_register_t {
        uint32_t src1_sel;
        uint32_t src1_offset;
        uint32_t src1_alpha;
        uint32_t src1_beta;
        uint32_t src1_gamma;
        uint32_t x_end;
        uint32_t blocking;
        uint32_t is_chain;

        uint32_t src2_sel;
        uint32_t src2_offset;
        uint32_t src2_alpha;
        uint32_t src2_beta;
        uint32_t src2_gamma;
        uint32_t y_end;
        uint32_t z_end;
        uint32_t f_update;

        uint32_t dst_sel;
        uint32_t dst_offset;
        uint32_t dst_alpha;
        uint32_t dst_beta;
        uint32_t dst_gamma;
        uint32_t func;
        uint32_t id;

        uint32_t dst_IMM;
        uint32_t src1_IMM;
        uint32_t src2_IMM;
    } io_vpro_cmd_register;

    struct io_dma_cmd_register_t {
        intptr_t ext{NULL};

        uint32_t ext_addr{0};
        uint32_t loc_addr{0};
        uint32_t x_size{0};
        uint32_t y_size{1};
        int32_t x_stride{1};

        uint32_t cluster_mask{0};
        uint32_t unit_mask{0};

        bool pad_flags[4]{false};
    } io_dma_cmd_register;

    uint32_t* general_purpose_register;

    // flag if gui should be opened
    bool windowless;
    // flag if simulation is running/paused
    bool sim_running;
    // flag to show finish/quit of sim
    bool sim_finished;

    // FK: Moved to public modules to be accessible by Wrapper
    // calls updates for all architecture parts / clock
    // name of output configuration file
    QString outputcfg = "../exit/output.cfg";
    QString inputcfg = "../init/input.cfg";
    QString initscript = "../init/init.sh";
    QString exitscript = "../exit/exit.sh";
    QString dataDirectory = "../data";
    QString statoutfilename = dataDirectory + "/statistic_out.csv";

    /**
     * RISCV counter need this information to detect if riscv is still waiting to finish to count corresponding cycles
     * (dma_wait_to_finish() or vpro_wait_busy() will set/reset this)
     */
    bool riscv_sync_waiting = false;
    // name of executable (to be exchanged in output files "<EXE>" string) (e.g. if output.cfg contains <EXE>.bin -> sim.bin)
    QString ExeName;
    // external symbols (global const variables)
    // address, (size, name -- only one entry in this map)
    QMap<uint64_t, QMap<int, QString>> external_variables;

    // global time (ns)
    double time;
    // timer for stepping (ui)
    double goal_time;

    // time for different clock domains
    double risc_time, axi_time, dcma_time;

    static constexpr double risc_clock_period = 5;
    static constexpr double risc_io_access_cycles =
        3;  // how many cycles does EIS-V need for issuing an io-access (from hw sim)
    static constexpr double axi_clock_period = 5;
    static constexpr double dcma_clock_period = 5;
    static constexpr double vpro_clock_period = 2.5;
    static constexpr double dma_clock_period = dcma_clock_period;

    static constexpr double iss_clock_tick_period = findGCD(dcma_clock_period,
        axi_clock_period,
        risc_clock_period,
        vpro_clock_period,
        dma_clock_period);

    // architecture. top level are clusters
    std::vector<Cluster*> clusters;

    std::shared_ptr<ArchitectureState> architecture_state;

    DMABlockExtractor* dmablock;
    DMALooper* dmalooper;

    QTime performanceMeasurementStart;
    QTimer* performanceMeasurement;
    uint64_t performance_clock_last_second;

    // when closing application, interpret exit script/config ... use sim_stop to run until all clusters done; will call this exit function!
    void printExitStats(bool silent = false);

    std::shared_ptr<CommandVPRO> gen_io_vpro_command() const;

    // internal functions to control the simulation process, called from gui (e.g.)
    void simPause() {
        if (simulationSpeedMeasurement) {
            if (performanceMeasurement->isActive()) emit performanceMeasurement_stop();
            performanceMeasureTimeout();
        }
        sim_running = false;
        emit simIsPaused();
    }

    void simResume() {
        if (simulationSpeedMeasurement) {
            emit performanceMeasurement_start(1000);
            performanceMeasurementStart = QTime::currentTime();
        }
        sim_running = true;
        emit simIsResumed();
    }

    void simToggle() {
        if (sim_running)
            simPause();
        else
            simResume();
    }

    [[nodiscard]] bool isSimRunning() const {
        return sim_running;
    }

   signals:
    // indicates end of sim
    void simFinish();

    // transfers all commands (sim and vpro queue) to gui
    void dataUpdate(CommandWindow::Data);

    // basic data about arch
    void simInit(int cluster_cnt, int unit_cnt, int lane_cnt);

    // current clock cycle and issued instructions
    void simUpdate(long clock);

    // status of sim pause/running
    void simIsPaused();

    void simIsResumed();

    /**
     * gui to lock further runing
     **/
    void simIsFinished();

    // give pointer to qt
    void sendmainmemory(uint8_t*);

    void sendRegister(uint8_t*, QVector<bool>);

    void sendLocalmemory(uint8_t*);

    void sendGuiwaitkeypress(bool*);

    void runMain();

    void performanceMeasurement_stop();

    void performanceMeasurement_start(int);

    void stat_reset(double, QString);

    void updatePerformanceTime(float);

   public slots:

    /**
     * pause or resume simulation loop
     */
    void pauseSim();

    /**
     * exit signal
     */
    void quitSim();

    /**
     * run a specified amount of cycles. then pause
     * @param steps
     */
    void runSteps(int steps);

    /**
     * basis function of simulation
     * start work on command queue until empty
     * after finish emits simFinish to print SimResults.
     * is called in separate thread...
     */
    void run() override;

    /**
     * run until cluster can receive a new command
     * simulate difference in riscv and vpro clocks (100 vs. 400 MHz)
     */
    void runUntilReadyForCmd();

    /**
     * simulate difference in riscv and vpro clocks (100 vs. 400 MHz)
     */
    void runUntilRiscReadyForCmd();

    /**
     * creates a copy of all cmds in list and emits the update to visualization.
     */
    void visualizeDataUpdate();

    /**
     * dumps LM to console
     * @param c Cluster
     * @param u Unit
     */
    void dumpLM(int c, int u);

    /**
     * dumps RF to console
     * @param c Cluster
     * @param u Unit
     * @param l Lane (Processing Lane only)
     */
    void dumpRF(int c, int u, int l);

    /**
     * dumps Queue (Cmd Fifo) to console
     * @param c Cluster
     * @param u Unit
     */
    void dumpQ(int c, int u);

    /**
     * send current cycle/commands in lanes to gui
     */
    void sendSimUpdate();

    /**
     * run until all clusters have finished.
     * do statistic print/dump
     * call printExitStats (exit script/cfg)
     */
    void sim_stop(bool silent = false);

    /**
     * ISS performance measuring.
     * This Timeout will print cycles of last period (Cycles / second)
     */
    void performanceMeasureTimeout();

    // debug read/write data to/from bus/external memory
    void dbgMemWrite(intptr_t dst_addr, uint8_t* data_ptr);

    void dbgMemRead(intptr_t dst_addr, uint8_t* data_ptr) const;

    void sim_stats_reset();

    void gen_direct_dma_instruction(const uint8_t dcache_data_struct[32]);
};

#endif  // sim_core_class
