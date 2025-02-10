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
// # simulator related functions (start/stop/...)         #
// ########################################################

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QString>
#include <QThread>
#include <QtConcurrent/QtConcurrent>
#include <QtCore/QFuture>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#ifndef ISS_STANDALONE
// FK
#include <thread>
#endif

#include "../model/architecture/DMALooper.h"
#include "../model/architecture/stats/Statistics.h"
#include "ISS.h"
#include "VectorMain.h"
#include "helper/debugHelper.h"

QFile* CMD_HISTORY_FILE;
QFile* PRE_GEN_FILE;
QTextStream* CMD_HISTORY_FILE_STREAM;
QDataStream* PRE_GEN_FILE_STREAM;

#ifndef ISS_STANDALONE
// FK: Added function object for registering callback
// this is used to route DMA memory accesses outside of the ISS
void ISS::register_memory_access_function(callback_function cb) {
    m_cb = cb;
};

void ISS::register_call_systemc_stop(call_systemc_stop_cb cb) {
    m_cb_systemc_stop = cb;
};
#endif

struct Sync {
    byte intValue;
};

ISS::ISS()
    : architecture_state(std::make_shared<ArchitectureState>()) {
    sim_finished = false;
    sim_running = false;
    windowless = false;
#ifndef ISS_STANDALONE
    bus = new NonBlockingBusSlaveImpl();
#endif
    time = 0, goal_time = 0, risc_time = 0, axi_time = 0, dcma_time = 0;
    performance_clock_last_second = 0;

    windowThread = true;

    isCompletelyInitialized = false;
    isPrintResultDone = false;

    performanceMeasurement = new QTimer();
    performanceMeasurementStart = QTime::currentTime();

    general_purpose_register = new uint32_t[0xff / 4]();

    if (DMA_gen_trace) {
        std::stringstream ss;
        ss << "DMA_SYNC.trace";
        openTraceFile(ss.str().c_str());
    }

    simPause();
}

void ISS::openTraceFile(const char* tracefilename) {
    dma_sync_trace = std::ofstream(tracefilename);
}

// ###############################################################################################################################
// Simulator Environment
// ###############################################################################################################################

// ---------------------------------------------------------------------------------
// Initialize simulation environment
// ---------------------------------------------------------------------------------
int ISS::sim_init(int (*main_fkt)(int, char**), int& argc, char* argv[]) {
    if (windowThread) {
        for (int i = 1; i < argc; ++i) {
            if (!qstrcmp(argv[i], "--windowless") || !qstrcmp(argv[i], "--silent")) {
                printf_warning("Gonna run silent, without windows [--windowless]\n");
                windowless = true;
                simResume();
            }
        }
    }

    if (!windowThread) {
        // second call skip - parallel thread - simulate program now

        // Disable buffering on std out
        setbuf(stdout, NULL);
        
        printf(
            "# "
            "---------------------------------------------------------------------------------\n");
        printf("# VPRO instruction set & system simulator \n");
        printf(
            "# "
            "---------------------------------------------------------------------------------\n");
        printf("# VPRO Array Configuration:\n");
        printf("#  Clusters:  %3d, Units (per Cl): %3d, Processing Lanes (per VU): %3d\n",
            VPRO_CFG::CLUSTERS,
            VPRO_CFG::UNITS,
            VPRO_CFG::LANES);
        printf("#                       Total VUs: %3d, Total Processing Lanes:    %3d \n",
            VPRO_CFG::UNITS * VPRO_CFG::CLUSTERS,
            VPRO_CFG::CLUSTERS * VPRO_CFG::UNITS * VPRO_CFG::LANES);
        printf("#\n");
        printf("# DCMA Configuration:\n");
        printf("#  #Rams:  %3d, Cache Line Size in Bytes:  %3d, \n", VPRO_CFG::DCMA_NR_BRAMS, VPRO_CFG::DCMA_LINE_SIZE);
        printf("#                       VPRO_CFG::DCMA_ASSOCIATIVITY: %3d, Ram Size in Bytes:    %3d \n",
            VPRO_CFG::DCMA_ASSOCIATIVITY,
            VPRO_CFG::DCMA_BRAM_SIZE);
        printf("#\n");
        printf("# ISS Memories:\n");
#ifdef ISS_STANDALONE
        string str;
        double msize = VPRO_CFG::MM_SIZE;
        if (VPRO_CFG::MM_SIZE <= 1024) {
            str = "B ";
            msize = VPRO_CFG::MM_SIZE;
        } else if (VPRO_CFG::MM_SIZE <= 1024 * 1024) {
            str = "KB";
            msize = VPRO_CFG::MM_SIZE / 1024.;
        } else if (VPRO_CFG::MM_SIZE <= 1024 * 1024 * 1024) {
            str = "MB";
            msize = VPRO_CFG::MM_SIZE / 1024. / 1024.;
        } else {
            str = "GB";
            msize = VPRO_CFG::MM_SIZE / 1024. / 1024. / 1024.;
        }
        printf("#  Main Memory (Non Blocking):    8-bit x %lu (%3.f %s)\n",
            VPRO_CFG::MM_SIZE,
            msize,
            str.c_str());
#endif
        printf("#  Local Memory:  16-bit x %6lu\n", VPRO_CFG::LM_SIZE);
        printf("#  Register File: 24-bit x %6lu\n", VPRO_CFG::RF_SIZE);
        printf("#\n");
        printf("# Clocks:\n");
        printf("#   Risc-V (only ISS functions calls): %s%04.2lf MHz%s (%02.2lf ns Period)\n",
            MAGENTA,
            1000 / risc_clock_period,
            RESET_COLOR,
            risc_clock_period);
        printf("#   AXI (Memory Accesses):             %s%04.2lf MHz%s (%02.2lf ns Period)\n",
            MAGENTA,
            1000 / axi_clock_period,
            RESET_COLOR,
            axi_clock_period);
        printf("#   DCMA:                              %s%04.2lf MHz%s (%02.2lf ns Period)\n",
            MAGENTA,
            1000 / dcma_clock_period,
            RESET_COLOR,
            dcma_clock_period);
        printf("#   DMA:                               %s%04.2lf MHz%s (%02.2lf ns Period)\n",
            MAGENTA,
            1000 / dma_clock_period,
            RESET_COLOR,
            dma_clock_period);
        printf("#   VPRO:                              %s%04.2lf MHz%s (%02.2lf ns Period)\n",
            MAGENTA,
            1000 / vpro_clock_period,
            RESET_COLOR,
            vpro_clock_period);
        printf("#\n");
        printf("# ISS Tick Time Period: %04.2lf ns\n", iss_clock_tick_period);
        printf(
            "# "
            "---------------------------------------------------------------------------------\n");

#ifdef ISS_STANDALONE
        bus = new NonBlockingMainMemory(VPRO_CFG::MM_SIZE);
#endif

        dcma = new DCMA(this, bus, VPRO_CFG::CLUSTERS, VPRO_CFG::DCMA_LINE_SIZE, VPRO_CFG::DCMA_ASSOCIATIVITY, VPRO_CFG::DCMA_NR_BRAMS, VPRO_CFG::DCMA_BRAM_SIZE);

        printf_info("# Calling initialization Script %s ... ", initscript.toStdString().c_str());
        std::ifstream init_script(initscript.toStdString().c_str());
        if (!init_script) {
            printf_warning("[File '%s' not found!]\n", initscript.toStdString().c_str());
        } else {
            int status = system(QString("bash ").append(initscript).toStdString().c_str());
            if (status != 0) printf("[executed, returned %i]\n", status);
        }

        // ########################################################################
        // Read input to MM (.cfg)
        // ########################################################################
        if (argc > 2 && windowless) {
            inputcfg = QString(argv[2]);
        } else if (argc >= 2 && !windowless) {
            inputcfg = QString(argv[1]);
        }
        if (argc >= 4 && windowless) {
            outputcfg = QString(argv[3]);
        } else if (argc >= 3) {
            outputcfg = QString(argv[2]);
        }

        printf_info("# Input settings from %s\n", inputcfg.toStdString().c_str());
        QFile configfile(inputcfg);
        configfile.open(QIODevice::ReadOnly);
        while (!configfile.atEnd()) {
            // line format: filename address [size [skip_pos, skip_len]*]
            QString line = configfile.readLine();
            line = line.simplified();
            if (line.startsWith(";") or line.startsWith("#") or line.isEmpty()) continue;
            auto input_items = line.split(" ");
            if (input_items.size() < 2) {
                printf_warning(
                    "Input file not containing enough items [Required: File, Address]\n");
                continue;
            }
            QString input_file = input_items[0];
            uint64_t address = input_items[1].toULong(nullptr, 0);
            int64_t size = -1;
            if (input_items.size() > 2) {
                size = input_items[2].toLong(nullptr, 0);
            }
            // insert gaps during load to MM: skip_len bytes every skip_pos bytes
            QVector<int64_t> skip_pos;
            QVector<int64_t> skip_len;
            for (int si = 3; si + 1 < input_items.size(); si += 2) {
                skip_pos.append(input_items[si].toLong(nullptr, 0));
                skip_len.append(input_items[si + 1].toLong(nullptr, 0));
            }
            QFile input(input_file);
            if (!input.open(QIODevice::ReadOnly)) {
                printf_warning(
                    "Input could not be opened! [File: %s]\n", input_file.toStdString().c_str());
                continue;
            }
            QByteArray input_data;
            if (size <= 0) {
                input_data = input.readAll();
            } else {
                input_data = input.read(size);
            }
            uint64_t mm_addr = address;
            for (int i = 0; i < input_data.size(); i++) {
                auto writedata = uint8_t(input_data.at(i));
                bus->dbgWrite(mm_addr, &writedata);
                mm_addr++;
                for (int si = 0; si < skip_pos.size(); si++) {
                    if ((i + 1) % skip_pos[si] == 0) {
                        mm_addr += skip_len[si];
                    }
                }
            }
            if (if_debug(DEBUG_DUMP_FLAGS))
                printf_info("\tFile %s was read to MM [%u (Dez) / 0x%x (Hex)]\t Size: %i\n",
                    input_file.toStdString().c_str(),
                    address,
                    address,
                    input_data.size());
        }

        // ########################################################################
        // check global variables . their addresses have to be outside MM
        // ########################################################################
        size_t length = 0;
        if (copyGlobalSectionToMMStart) {
            std::ifstream globals("globals.section");
            if (!globals) {
                printf_warning(
                    "#SIM: file globals.section not found! Won't copy content to MM start.\n");
            } else {
                globals.seekg(0, globals.end);  //get length of file
                length = globals.tellg();
                globals.seekg(0, globals.beg);
                char* buffer = new char[length];  // allocate buffer
                globals.read(buffer, length);     //read file
                for (int i = 0; i < length; i++) {
                    uint8_t writedata = uint8_t(buffer[i]);
                    bus->dbgWrite(i, &writedata);
                }
                printf(
                    "#SIM: Loaded all Global Variables to main_memory start [address: 0] (Size: "
                    "%i)\n",
                    (int)length);  // (__attribute__ ((section ("glob"))))
            }
        }
        if (debug & DEBUG_GLOBAL_VARIABLE_CHECK) {
            printf(
                "#SIM: For Simulation the global Variables are accessed by their address. "
                "Checking...:\n");

            FILE* fp;
            char path[1035];
            std::string buf("nm --print-size ");
            buf.append(argv[0]);
            buf.append(
                " | grep '[0-9a-f]* r _ZL'");  // nm prints all symbols + addresses + size, filter for r _ZL (global variables beginn with this with gcc...)
            fp = popen(buf.c_str(), "r");
            if (fp == NULL) {
                printf_error("Failed to run \"nm\" command\n");
                //exit(1);
            }
            char delimiter[] = " \n";
            char* ptr; /* Read the output a line at a time - output it. */

            while (fgets(path, sizeof(path) - 1, fp) != NULL) {
                char* name;
                int address, size;
                ptr = strtok(path, delimiter);  // split with delimiter
                if (ptr != NULL) {
                    address = (int)strtol(ptr, NULL, 16);
                    ptr = strtok(NULL, delimiter);
                    size = (int)strtol(ptr, NULL, 16);
                    ptr = strtok(NULL, delimiter);
                    ptr = strtok(NULL, delimiter);
                    name = ptr;
                }
                if (address < VPRO_CFG::MM_SIZE) {
                    printf_error(
                        "\tObject Failed: external Object inside main memory [0 ... %li] (not "
                        "addressable) -- name: %s @%i [size: %i]\n",
                        VPRO_CFG::MM_SIZE,
                        name,
                        address,
                        size);
                } else if (address >= VPRO_CFG::MM_SIZE && (debug & DEBUG_GLOBAL_VARIABLE_CHECK)) {
                    QMap<int, QString> object;
                    object[size] = name;
                    // TODO
                    //                    external_variables[address + (uint64_t) (&main_memory)] = object;
                    printf_info(
                        "\tObject Passed: external Object outside main memory [0 ... %li] "
                        "(addressable)     -- name: %s @%i [size: %i]\n",
                        VPRO_CFG::MM_SIZE,
                        name,
                        address,
                        size);
                }
            }
            pclose(fp);
        }

        // ########################################################################
        // Create Cluster
        // ########################################################################
        printf("\n");
        for (int i = 0; i < VPRO_CFG::CLUSTERS; i++) {
            clusters.push_back(new Cluster(this,
                i,
                architecture_state,
                dcma,
                time));
            clusters.back()->dma->setExternalVariableInfo(&external_variables);
        }
        dmalooper = new DMALooper(this);
        dmablock = new DMABlockExtractor(this);

        if (!windowless) {
            int c = 0;
            for (auto cluster : clusters) {
                int u = 0;
                for (auto unit : cluster->getUnits()) {
                    w->setLocalMemory(unit->id(), unit->getLocalMemoryPtr(), c);
                    for (auto lane : unit->getLanes()) {
                        if (lane->vector_lane_id != log2(int(LS)))
                            w->setRegisterMemory(lane->vector_lane_id, lane->getregister(), c, u);
                    }
                    u++;
                }
                c++;
            }

            CommandWindow::VproSpecialRegister v{.architecture_state = architecture_state};

            for (auto c : clusters) {
                v.accus.append(QVector<QVector<uint64_t*>>());
                for (auto u : c->getUnits()) {
                    v.accus.last().append(QVector<uint64_t*>());
                    for (auto l : u->getLanes()) {
                        if (!l->isLSLane()) v.accus.last().last().append(l->getAccu());
                    }
                }
            }
            w->setVproSpecialRegisters(v);
        }

#ifdef THREAD_CLUSTER
        //        busythreads.release(VPRO_CFG::CLUSTERS);
        if (VPRO_CFG::CLUSTERS >= 2 && VPRO_CFG::UNITS >= 2) {
            for (Cluster* cluster : clusters) {
                cluster->tick_en.lock();
                cluster->tick_done.lock();
                cluster->start();
            }
        }
#endif
        if (CREATE_CMD_HISTORY_FILE) {
            QFileInfo fi(CMD_HISTORY_FILE_NAME);
            CMD_HISTORY_FILE = new QFile(CMD_HISTORY_FILE_NAME);

            if (!fi.absoluteDir().exists()) {
                QDir().mkdir(fi.absoluteDir().absolutePath());
                printf_info("[CMD_HISTORY_FILE] Directory for file created: %s\n",
                    fi.absoluteDir().absolutePath().toStdString().c_str());
            }
            if (!CMD_HISTORY_FILE->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                printf_error(
                    "CMD History file could not be opened! [FILE : %s]\n", CMD_HISTORY_FILE_NAME);
            }
            if (CMD_HISTORY_FILE->isWritable()) {
                CMD_HISTORY_FILE_STREAM = new QTextStream(CMD_HISTORY_FILE);
            } else {
                printf_error(
                    "CMD History File is not writeable! [FILE : %s]\n", CMD_HISTORY_FILE_NAME);
            }
        }

        if (CREATE_CMD_PRE_GEN) {
            QFileInfo fi(CMD_PRE_GEN_FILE_NAME);
            PRE_GEN_FILE = new QFile(CMD_PRE_GEN_FILE_NAME);

            if (!fi.absoluteDir().exists()) {
                QDir().mkdir(fi.absoluteDir().absolutePath());
                printf_info("[PRE_GEN_FILE] Directory for file created: %s\n",
                    fi.absoluteDir().absolutePath().toStdString().c_str());
            }
            if (!PRE_GEN_FILE->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                printf_error(
                    "CMD History file could not be opened! [FILE : %s]\n", CMD_PRE_GEN_FILE_NAME);
            }
            if (PRE_GEN_FILE->isWritable()) {
                PRE_GEN_FILE_STREAM = new QDataStream(PRE_GEN_FILE);
            } else {
                printf_error(
                    "CMD History File is not writeable! [FILE : %s]\n", CMD_PRE_GEN_FILE_NAME);
            }
        }

        printf("# ISS initialized. Starting Application.\n");
        printf(
            "# "
            "---------------------------------------------------------------------------------\n");
        Statistics::get().initialize(this);
        isCompletelyInitialized = true;
        // return to main and simulate program in this thread
        return 0;
    }
    //GUI Thread (Sim-Init, qApp + Window)
    windowThread = false;

    if (!windowless) {
        application = new QApplication(argc, argv);  //createApplication(argc, argv);
    } else {
        application = new QCoreApplication(argc, argv);
    }

    qRegisterMetaType<CommandWindow::Data>("CommandWindow::Data");
    qRegisterMetaType<CommandWindow::Data>("CommandWindow::VproSpecialRegister");
    qRegisterMetaType<QVector<int>>("QVectorint");

#ifdef ISS_STANDALONE
    // start main again (as simulator thread)
    simulatorThreadProgram = new VectorMain(main_fkt, argc, argv);
    simulatorThreadProgram->moveToThread(&simulatorThread);
    connect(this,
        &ISS::runMain,
        simulatorThreadProgram,
        &VectorMain::doWork);  // change to vector_main()
    simulatorThread.start();

    // this is the try to close all threads upon gui/this thread ends
    connect(this, &QThread::finished, this, &QObject::deleteLater);
    connect(this, SIGNAL(finished()), &simulatorThread, SLOT(quit()));
    connect(this, SIGNAL(finished()), &simulatorThread, SLOT(deleteLater()));
    connect(this, SIGNAL(deleteLater()), &simulatorThread, SLOT(exitIssThread()));

#endif
    // FK: Removed since it wont be needed anymore 21-06-15

    // regular finish of simulated program
    //    connect(this, &SimCore::simFinish, this, &SimCore::printSimResults);
    //    connect(this, &SimCore::simFinish, this, &SimCore::sendSimUpdate);
    //    connect(this, SIGNAL(simFinish()), &simulatorThread, SLOT(quit()));

    // status signals of simulation to send update to GUI
    connect(this, &ISS::simIsPaused, this, &ISS::sendSimUpdate);
    connect(this, &ISS::simIsResumed, this, &ISS::sendSimUpdate);

    if (!windowless) {
        w = new CommandWindow(VPRO_CFG::CLUSTERS, VPRO_CFG::UNITS, VPRO_CFG::LANES);
        connect(this, SIGNAL(dataUpdate(CommandWindow::Data)),
            w, SLOT(dataUpdate(CommandWindow::Data)));
        connect(this, SIGNAL(simIsFinished()), w, SLOT(simIsFinished()));
        connect(this, SIGNAL(simUpdate(long)), w, SLOT(simUpdate(long)));
        connect(this, SIGNAL(simIsPaused()), w, SLOT(simIsPaused()));
        connect(this, SIGNAL(simIsResumed()), w, SLOT(simIsResumed()));
        connect(w, SIGNAL(runSteps(int)), this, SLOT(runSteps(int)), Qt::DirectConnection);
        connect(w, SIGNAL(pauseSim()), this, SLOT(pauseSim()), Qt::DirectConnection);
        connect(w, SIGNAL(destroyed()), this, SLOT(quitSim()), Qt::DirectConnection);
        connect(w, SIGNAL(exit(int)), this, SLOT(exit(int)));
        connect(w,  SIGNAL(dumpLocalMemory(int, int)),
            this, SLOT(dumpLM(int, int)),
            Qt::DirectConnection);
        //connect(w, SIGNAL(dumpQueue(int, int)), this, SLOT(dumpQ(int, int)), Qt::DirectConnection);
        connect(w, SIGNAL(dumpRegisterFile(int, int, int)),
            this, SLOT(dumpRF(int, int, int)),
            Qt::DirectConnection);
        connect(this, SIGNAL(sendGuiwaitkeypress(bool*)), w, SLOT(recieveguiwaitpress(bool*)));
        connect(this, SIGNAL(sendmainmemory(uint8_t*)), w, SLOT(getmainmemory(uint8_t*)));
        connect(w, SIGNAL(getSimupdate()), this, SLOT(sendSimUpdate()), Qt::DirectConnection);
        //connect(this, SIGNAL(sendLocalmemory(uint8_t * )), w, SLOT(saveLocalmemory(uint8_t * )));
        connect(this, SIGNAL(stat_reset(double, QString)), w, SLOT(stats_reset(double, QString)));

        // connect 1sec timer to measure simulator speed each second
        connect(performanceMeasurement, SIGNAL(timeout()),
            this, SLOT(performanceMeasureTimeout()),
            Qt::DirectConnection);
        connect(this, SIGNAL(performanceMeasurement_stop()),
            performanceMeasurement, SLOT(stop()),
            Qt::DirectConnection);
        connect(this, SIGNAL(performanceMeasurement_start(int)),
            performanceMeasurement, SLOT(start(int)),
            Qt::DirectConnection);
        connect(this, SIGNAL(finished()),
            performanceMeasurement, SLOT(deleteLater()),
            Qt::DirectConnection);
        connect(this, SIGNAL(updatePerformanceTime(float)),
            w, SLOT(updatePerformanceTime(float)),
            Qt::DirectConnection);

        w->show();
        emit stat_reset(time, QString(""));
        emit simIsPaused();
        emit simUpdate(long(time));

    } else {
        //        qDebug() << "windowless!";
        emit performanceMeasurement_stop();
    }

    // FK: Removed, no need to start the main thread anymore 21-06-15
    emit runMain();  // run main/vpro in other thread

    // FK: Moved to wrapper by calling ProcessEvents() 21-06-15
    // FK: Currently removed due to freezing simulation 21-06-15
#ifdef ISS_STANDALONE
    QCoreApplication::exec();  // GUI Loop. Waiting for QApplication to exit (e.g. called from gui button or by sim_exit();)

    emit performanceMeasurement_stop();
    sim_finished = true;
    simulatorThread.quit();
    simulatorThread.deleteLater();
    delete performanceMeasurement;
    performanceMeasurement = nullptr;

    //  performed from other thread in run()...; printSimResults(); sim_exit();
    while (!isPrintResultDone) {
        msleep(50);
    }
    msleep(50);
#endif
    return 0;
}

void ISS::sim_stats_reset() {
    QString stat_log;
    Statistics::get().print(stat_log);
    Statistics::get().reset();
    emit stat_reset(time, stat_log);
}

void ISS::sim_stop(bool silent) {
    printf_info("Sim stop!\n");
    while (!sim_finished) {
        bool cluster_busy = false;
        for (auto cluster : clusters) {
            cluster_busy |= cluster->isBusy();
        }
        if (cluster_busy) {
            run();
            continue;
        }
        break;
    }
    sendSimUpdate();
    simPause();

    printExitStats(silent);   // stat to file/console
    if (CREATE_CMD_HISTORY_FILE) {
        CMD_HISTORY_FILE_STREAM->flush();
        CMD_HISTORY_FILE->close();
    }
    if (CREATE_CMD_PRE_GEN) {
        PRE_GEN_FILE->close();
    }

    emit simIsFinished();
    printf("Simulation Finished Successful!\n");

#ifndef ISS_STANDALONE
    // Calling SystemC exit
    //std::cout << "FK stop SystemC" << endl;
    m_cb_systemc_stop();
    // exit(EXIT_SUCCESS);
#endif

    // AK: Don't close GUI on finished SIM
    if (windowless) {
        QCoreApplication::quit();
        sim_finished = true;
        isPrintResultDone = true;  // let other main exit
        msleep(60);
        emit exit(0);
        std::exit(0);
    } else {
        sim_finished = true;
        isPrintResultDone = true;  // let other main exit
        msleep(60);
        emit exit(0);
        std::exit(0);
    }
    // main will continue with exit(...);
}

void ISS::performanceMeasureTimeout() {
    // called once per second or on pause
    // if sim is running
    // get time and # of processed clock cycles

    auto now = QTime::currentTime();
    int rtime = performanceMeasurementStart.msecsTo(now);
    auto cycles = time - performance_clock_last_second;

    if (rtime > 0 && cycles > 0) {
        float currentRate = ((float)cycles / (float)rtime * 1000.0);
        emit updatePerformanceTime(currentRate);
    }

    performance_clock_last_second = time;
    performanceMeasurementStart = now;
}

// ---------------------------------------------------------------------------------
// Run simulation environment for one cycle (until finished)
// ---------------------------------------------------------------------------------
void ISS::run() {
    if (!isCompletelyInitialized) {
        return;
    }
    // time loop
    if (!sim_finished) {
        // FK: THIS IS BLOCKING!
        while (!isSimRunning()) {
            // std::cout << "FK: I am inside SimCore::run()::!isSimRunning()" << std::endl;
            usleep(1);
#ifndef ISS_STANDALONE
            this->application->processEvents();
#endif
        }

        if (!windowless) {                             // update GUI?
            if (goal_time > 0 && goal_time <= time) {  // running until goal_time done
                goal_time = -1;
                simPause();  // set pause mode
                sendSimUpdate();
                this->application->processEvents();
                run();
                return;
            }
            if (((VPRO_CFG::CLUSTERS * VPRO_CFG::UNITS) < 10 && long(time) % 50000 == 0) ||
                long(time) % 10000 == 0) {  // update gui clock every x ns
                sendSimUpdate();
            }
        }

        // clock tick
        clk_tick();
        return;
    }

    // finished button from GUI . cleanup?
    visualizeDataUpdate();
    printf(
        "#SIM: Run-Loop exit. GUI got destroyed while sim running. Simulation exit (Time: %lf "
        "ns)\n",
        time);
    printf_warning("Results haven't been written into results.csv!");
    isPrintResultDone = true;  // let other main exit

    exit(EXIT_SUCCESS);
    std::_Exit(EXIT_SUCCESS);
}

/**
 * used for vpro / dma instructions. fifo needs to be free / accepting new commands
 */
void ISS::runUntilReadyForCmd() {
#ifndef ISS_STANDALONE
    printf_error("[ERROR] runUntilReadyForCmd should never be called from SystemC VP!\n");
#endif
    bool clusterClocking = false;
    while (true) {
        run();

        // check if any cluster is rdy for a cmd (queue not full, no dma|vpro wait_busy, loop not blocking)
        // TODO: maybe wrong behaviour if different cluster process different commands (complex)
        clusterClocking = false;
        for (auto c : clusters) {
            clusterClocking |= !c->isReadyForCommand();
        }

        // if risc not yet ticking
        if (risc_time > time)
            continue;
        else {
            if (debug & DEBUG_TICK)
                printf("RISC Clock Cycle %.2lf (RISC Time: %.2lf ns)\n",
                    risc_time / risc_clock_period,
                    risc_time);
            risc_time += risc_clock_period;
            Statistics::get().tick(Statistics::clock_domains::RISC);
            dmalooper->tick();
            dmablock->tick();
            risc_counter_tick();
            if (!clusterClocking)
                // if no cluster ready or risc not rdy for new cmd => another time tick
                break;
        }
    }
}

/**
 * used for aux functions (no need of cluster to be ready)
 */
void ISS::runUntilRiscReadyForCmd() {
#ifndef ISS_STANDALONE
    printf_error("[ERROR] runUntilRiscReadyForCmd should never be called from SystemC VP!\n");
#endif
    uint32_t io_cycle_counter = 0;
    while (true) {
        run();
        if (risc_time > time) {
            continue;
        } else {
            if (debug & DEBUG_TICK)
                printf("RISC Clock Cycle %.2lf (RISC Time: %.2lf ns)\n",
                    risc_time / risc_clock_period,
                    risc_time);
            risc_time += risc_clock_period;
            Statistics::get().tick(Statistics::clock_domains::RISC);
            dmalooper->tick();
            dmablock->tick();
            risc_counter_tick();
            io_cycle_counter++;
            if (io_cycle_counter == risc_io_access_cycles) break;
        }
    }
}

void ISS::risc_counter_tick() {
    // this is a clock tick in risc domain
    aux_cnt_vpro_total++;
    bool any_lane_busy = false, any_dma_busy = false;
    for (auto c : clusters) {
        for (auto unit : c->getUnits()) {
            if (unit->isBusy()) any_lane_busy = true;
        }
        if (c->dma->isBusy()) any_dma_busy = true;
    }
    if (any_lane_busy) {
        aux_cnt_lane_act++;
    }
    if (any_dma_busy) {
        aux_cnt_dma_act++;
    }
    if (any_lane_busy && any_dma_busy) {
        aux_cnt_both_act++;
    }
    aux_cnt_riscv_total++;
    aux_sys_time++;
    aux_cycle_counter++;
    if (!riscv_sync_waiting) aux_cnt_riscv_enabled++;
}

// ---------------------------------------------------------------------------------
// Run simulation environment (one clock tick)
// ---------------------------------------------------------------------------------
void ISS::clk_tick() {
    // should not be called directly! Use run() -> send gui updates, check if running active, initialized...

    time += iss_clock_tick_period;  // kgt

#ifndef ISS_STANDALONE  // done in run_until_rdy_for_cmd -> if Standalone
    while (risc_time < time) {
        if (debug & DEBUG_TICK)
            printf("RISC Clock Cycle %.2lf (RISC Time: %.2lf ns)\n",
                risc_time / risc_clock_period,
                risc_time);
        risc_time += risc_clock_period;
        Statistics::get().tick(Statistics::clock_domains::RISC);
        dmalooper->tick();
        //        dmablock->tick();   // not required in VP -> using SystemC Dcache + FSM
        risc_counter_tick();
    }
#endif

    if (debug & DEBUG_GLOBAL_TICK) printf("Time: %.2lf\n", time);

#ifdef THREAD_CLUSTER
    if (VPRO_CFG::CLUSTERS >= 2 && VPRO_CFG::UNITS >= 2) {
        for (auto cluster : clusters) {
            cluster->tick_en.unlock();
        }
        for (auto cluster : clusters) {
            cluster->tick_done.lock();
        }
    } else {
        for (auto cluster : clusters) {
            cluster->tick();
        }
    }
#else

#ifndef ISS_STANDALONE
    while (1) {
        if (!sim_running) {
            this->application->processEvents();
        } else {
            this->application->processEvents();
            break;
        };
    }
#endif
    // cascade to Lanes and DMAs
    for (auto cluster : clusters) {
        cluster->tick();
    }
#endif
    // check if time for clock tick is up
    if (dcma_time <= time) {
        if (debug & DEBUG_TICK)
            printf("DCMA Clock Cycle %.2lf (DCMA Time: %.2lf ns)\n",
                dcma_time / dcma_clock_period,
                dcma_time);
        dcma_time += dcma_clock_period;
        dcma->tick();
        Statistics::get().tick(Statistics::clock_domains::DCMA);
    }
#ifdef ISS_STANDALONE
    if (axi_time <= time) {
        if (debug & DEBUG_TICK)
            printf("AXI Clock Cycle %.2lf (AXI Time: %.2lf ns)\n",
                axi_time / axi_clock_period,
                axi_time);
        axi_time += axi_clock_period;
        reinterpret_cast<NonBlockingMainMemory*>(bus)->tick();
        Statistics::get().tick(Statistics::clock_domains::AXI);
    }
#endif
}

void ISS::pauseSim() {
    simToggle();
    sendSimUpdate();
}

void ISS::sendSimUpdate() {
    if (!isCompletelyInitialized) return;
    visualizeDataUpdate();
    emit simUpdate(long(time));
}

void ISS::runSteps(int steps) {
    goal_time = time + steps;
    printf("gonna run %i cycles...", steps);
    printf("from %.2lf to %.2lf\n", time, goal_time);
    this->application->processEvents();
    simResume();
}

void ISS::dumpLM(int c, int u) {
    this->clusters[c]->dumpLocalMemory(u);
    int cc = 0, cu = 0;
    for (auto cluster : clusters) {
        cu = 0;
        for (auto unit : cluster->getUnits()) {
            if (cc == c && cu == u) {
                uint8_t* memory = unit->getLocalMemoryPtr();

                emit sendLocalmemory(memory);
                break;
            }
            cu++;
        }
        cc++;
    }
}

void ISS::dumpQ(int c, int u) {
    this->clusters[c]->dumpQueue(u);
}

void ISS::dumpRF(int c, int u, int l) {
    this->clusters[c]->dumpRegisterFile(u, l);
    QVector<bool> flag(2);
    int cc = 0, cu = 0, cl = 0;
    for (auto cluster : clusters) {
        cu = 0;
        for (auto unit : cluster->getUnits()) {
            cl = 0;
            for (auto lane : unit->getLanes()) {
                if (cl == l && cc == c && cu == u) {
                    uint8_t* regist = lane->getregister();

                    flag[0] = lane->getzeroflag();
                    flag[1] = lane->getnegativeflag();

                    emit sendRegister(regist, flag);
                    break;
                }
                cl++;
            }
            cu++;
        }
        cc++;
    }
}

void ISS::quitSim() {
    simPause();
    sim_finished = true;
}

void ISS::visualizeDataUpdate() {
    if (!isCompletelyInitialized) return;

    // update visualization data
    CommandWindow::Data data;

    // current commands in lanes;
    for (auto cluster : clusters) {
        for (auto unit : cluster->getUnits()) {
            for (auto lane : unit->getLanes()) {
                data.commands.push_back(std::make_shared<CommandVPRO>(lane->getCmd().get()));
            }
        }
    }

    // commands in unit command queues;
    for (auto cluster : clusters) {
        data.isDMAbusy.push_back(cluster->isWaitDMABusy());
        data.isbusy.push_back(cluster->isWaitBusy());
        for (auto unit : cluster->getUnits()) {
            auto commands = unit->getCopyOfCommandQueue();
            auto commands_qvec = QVector<std::shared_ptr<CommandVPRO>>();
            for (auto& command : commands) {
                commands_qvec.append(command);
            }
            data.command_queue.push_back(commands_qvec);
        }
    }

    data.clock = long(getTime());

    emit dataUpdate(data);
}

void ISS::printExitStats(bool silent) {
    isCompletelyInitialized = false;
    if (!silent) {
        printf_info("\n\nSim exit\n\n");
        printf_warning(
            "  Dont forget to flush the Cached DCMA! \n"
            "  Main Memory content may not yet be consistent with VPRO written data!\n"
            "    -> dcma_flush();\n\n");
        printf_info("Generating Statistic Report...\n");
        Statistics::get().print();
    }

    // dump to file
    QFileInfo stat_file(QDir::currentPath() + "/" + dumpFileName);
    if (!QDir(stat_file.absoluteDir().path()).exists()) {
        QDir(stat_file.absoluteDir().path()).mkpath(".");
        printf("Created new dir for stats: %s\n",
            stat_file.absoluteDir().path().toStdString().c_str());
    }
    Statistics::get().dumpToFile(dumpFileName);
    Statistics::get().dumpToJSONFile(dumpJSONFileName);

    // output cfg
    QFile file(outputcfg);
    if (!file.open(QIODevice::ReadOnly)) {
        printf_warning("Could not open %s: %s\n",
            outputcfg.toStdString().c_str(),
            file.errorString().toStdString().c_str());
    } else {
    }
    QTextStream in(&file);

    while (!in.atEnd()) {
        // line format: filename address size [skip_pos, skip_len]*
        QString line = in.readLine();
        if (line.startsWith('#') || line.startsWith(';'))  // skip commented lines
            continue;
        QString output;
        uint64_t offset, size;
        QStringList fields = line.split(" ");
        if (fields.size() < 3) {
            printf_warning("Invalid Line for output; %s", line.toStdString().c_str());
            continue;
        }
        output = fields[0].replace("<EXE>", ExeName);
        offset = fields[1].toULong(nullptr, 0);
        size = fields[2].toULong(nullptr, 0);
        // skip MM during dump: skip_len bytes every skip_pos bytes
        QVector<int64_t> skip_pos;
        QVector<int64_t> skip_len;
        for (int si = 3; si + 1 < fields.size(); si += 2) {
            skip_pos.append(fields[si].toLong(nullptr, 0));
            skip_len.append(fields[si + 1].toLong(nullptr, 0));
        }
        if (if_debug(DEBUG_DUMP_FLAGS))
            printf_info("\tSaving Data to file: %s (Offset: %lu Dec | 0x%x, Bytes: %lu)\n",
                output.toStdString().c_str(),
                offset,
                size);

        QFile outputfile(output);
        uint64_t mm_addr = offset;
        if (outputfile.open(QFile::WriteOnly | QFile::Truncate)) {
            for (int i = 0; i < size; i++) {
                uint8_t buffer;
                bus->dbgRead(mm_addr, &buffer);
                outputfile.write((char*)&buffer, sizeof(buffer));
                mm_addr++;
                for (int si = 0; si < skip_pos.size(); si++) {
                    if ((i + 1) % skip_pos[si] == 0) {
                        mm_addr += skip_len[si];
                    }
                }
            }
            outputfile.close();
        } else {
            printf_error("Error opening output file!");
            QTextStream(stdout) << outputfile.errorString();
        }
    }

    file.close();

    printf_info("# Calling exit Script '%s' ... ", exitscript.toStdString().c_str());
    std::ifstream exit_script(exitscript.toStdString().c_str());
    if (!exit_script) {
        printf_warning("[File '%s' not found!]\n", exitscript.toStdString().c_str());
    } else {
        int status = system(QString("bash ").append(exitscript).toStdString().c_str());
        if (status != 0) printf("[executed, returned %i]\n", status);
    }
    exit_script.close();
}

void ISS::dbgMemWrite(intptr_t dst_addr, uint8_t* data_ptr) {
    bus->dbgWrite(dst_addr, data_ptr);
}

void ISS::dbgMemRead(intptr_t dst_addr, uint8_t* data_ptr) const {
    bus->dbgRead(dst_addr, data_ptr);
}
