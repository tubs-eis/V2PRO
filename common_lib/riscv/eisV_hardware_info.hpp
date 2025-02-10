//
// Created by gesper on 24.10.22.
//

#ifndef TEMPLATE_EISV_HARDWARE_INFO_HPP
#define TEMPLATE_EISV_HARDWARE_INFO_HPP

#include <versioning.h>

#define VERSION_MAJOR 0
#define VERSION_MINOR 1
static char static_versionVersion[] = {
        VERSION_MAJOR_INIT, '.', VERSION_MINOR_INIT, '\0'
};
static char static_completeVersion[] = {
        BUILD_YEAR_CH0, BUILD_YEAR_CH1, BUILD_YEAR_CH2, BUILD_YEAR_CH3,
        '-', BUILD_MONTH_CH0, BUILD_MONTH_CH1, '-', BUILD_DAY_CH0, BUILD_DAY_CH1,
        ' ', BUILD_HOUR_CH0, BUILD_HOUR_CH1,
        ':', BUILD_MIN_CH0, BUILD_MIN_CH1, ':', BUILD_SEC_CH0, BUILD_SEC_CH1, '\0'
};
static char static_defaultAppName[] = "<NO-APP-NAME>";

#include <chrono>
#include <ctime>
#include <string>

#ifdef SIMULATION
#include <QProcess>
#include "core_wrapper.h"
#include "iss_aux.h"
#else
#include "vpro/vpro_aux.h"
#endif

inline void
aux_print_hardware_info(const char appname[] = static_defaultAppName, const char version[] = static_versionVersion,
                        const char versionComplete[] = static_completeVersion) {

    printf("\n###############################\n");
    printf("Application: %s, %s\n", appname, version);
    printf("SW-Version:  %s\n", versionComplete);
    char buff[20];
#ifdef SIMULATION
    time_t now = (unsigned) time(nullptr);
#else   // SIMULATION
    time_t now = get_gpr_hw_build_date(); //*((volatile uint32_t *)(0x80000000));   // IO Access not to VPRO, DebugFifo, ... standard io access returns hw version
#endif  // SIMULATION
    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
    printf("HW-Version:  %s\n", buff);
#ifdef SIMULATION
    QProcess process;
    process.start(
            "GIT_SUPERPROJEKT_DIR=`git rev-parse --show-superproject-working-tree --show-toplevel | head -1` && echo `cd ${GIT_SUPERPROJEKT_DIR} && git rev-parse --short=4 HEAD`");
    process.waitForFinished(-1); // will wait forever until finished
    QString output = process.readAllStandardOutput();
    if (output.isEmpty()) {
        process.start("git rev-parse --short=4 HEAD");
        process.waitForFinished(-1); // will wait forever until finished
        output = process.readAllStandardOutput();
    }

    printf("GIT Commit (ISS): %s\n", output.toStdString().c_str());
    printf("Frequency VPRO:  %d MHz\n", int(1000 / core_->getVPROClockPeriod()));
    printf("Frequency EIS-V: %d MHz\n", int(1000 / core_->getRiscClockPeriod()));
    printf("Frequency DMA:   %d MHz\n", int(1000 / core_->getDMAClockPeriod()));
    printf("Frequency AXI:   %d MHz\n", int(1000 / core_->getAxiClockPeriod()));
    printf("VPRO Config:\n");
    printf("  Clusters:                  %i\n", VPRO_CFG::CLUSTERS);
    printf("  Units per Cluster:         %i\n", VPRO_CFG::UNITS);
    printf("  Proc. Lanes per Unit:      %i\n", VPRO_CFG::LANES);
    printf("DCMA Config:\n");
    if (get_gpr_DCMA_off() == 0){
        printf("  Num RAMs:                  %ld\n", get_gpr_DCMA_nr_rams());
        printf("  Cache Line Size [Bytes]:   %ld\n", get_gpr_DCMA_line_size());
        printf("  Associativity:             %ld\n", 1 << get_gpr_DCMA_associativity());
    }
    else
        printf("  DCMA is off      \n");
#else   // SIMULATION
    printf("GIT Commit: %lx\n", get_gpr_git_sha());
    printf("Frequency VPRO:  %ld MHz\n", get_gpr_vpro_freq()/1000/1000);
    printf("Frequency EIS-V: %ld MHz\n", get_gpr_risc_freq()/1000/1000);
    printf("Frequency DMA:   %ld MHz\n", get_gpr_dma_freq()/1000/1000);
    printf("Frequency AXI:   %ld MHz\n", get_gpr_axi_freq()/1000/1000);
    printf("VPRO Config:\n");
    printf("  Clusters:                  %ld\n", get_gpr_clusters());
    printf("  Units per Cluster:         %ld\n", get_gpr_units());
    printf("  Proc. Lanes per Unit:      %ld\n", get_gpr_laness());
    printf("DCMA Config:\n");
    if (get_gpr_DCMA_off() == 0){
        printf("  Num RAMs:                  %ld\n", get_gpr_DCMA_nr_rams());
        printf("  Cache Line Size [Bytes]:   %ld\n", get_gpr_DCMA_line_size());
        printf("  Associativity:             %ld\n", 1 << get_gpr_DCMA_associativity());
    }
    else
        printf("  DCMA is off      \n");
#endif  // SIMULATION
    printf("###############################\n\n");
}

#endif //TEMPLATE_EISV_HARDWARE_INFO_HPP
