
QT       += core gui widgets

TARGET = Commands
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += "SKIP_SIMSTAT_CORE=1"
DEFINES += "SIMULATION=1"

CONFIG += c++2a

# trick: find . -type f -name "*.cpp"

SOURCES += \
    main.cpp \
    commandwindow.cpp \
    commandstablewidget.cpp \
    savesettings.cpp \
    savemain.cpp \
    siminitfunctions.cpp \
    simupdatefunctions.cpp \
    dataupdatefunctions.cpp \
    rftableview.cpp \
    lmtableview.cpp \
    ../../../core_dma_broadcast.cpp \
    ../../../model/commands/CommandVPRO.cpp \
    ../../../model/commands/CommandBase.cpp \
    ../../../model/commands/CommandDMA.cpp \
    ../../../model/commands/CommandSim.cpp \
    ../../../model/architecture/RegisterFile.cpp \
    ../../../model/architecture/DMA.cpp \
    ../../../model/architecture/Cluster.cpp \
    ../../../model/architecture/VectorLaneDebug.cpp \
    ../../../model/architecture/stats/SimStat.cpp \
    ../../../model/architecture/stats/UnitStat.cpp \
    ../../../model/architecture/stats/DMAStat.cpp \
    ../../../model/architecture/stats/LaneStat.cpp \
    ../../../model/architecture/VectorLaneLS.cpp \
    ../../../model/architecture/VectorUnit.cpp \
    ../../../model/architecture/VectorLane.cpp \
    ../../../model/architecture/Pipeline.cpp \
    ../../../core_wrapper.cpp \
    ../../../simulator/VectorMain.cpp \
    ../../../simulator/ISS.cpp \
    ../../../simulator/helper/typeConversion.cpp \
    ../../../simulator/helper/debugHelper.cpp \
    ../../../simulator/simSim.cpp \
    ../../../iss_aux.cpp

HEADERS += \
    commandwindow.h \
    lmtableview.h \
    commandstablewidget.h \
    savesettings.h \
    savemain.h \
    siminitfunctions.h \
    simupdatefunctions.h \
    dataupdatefunctions.h \
    rftableview.h \
    ../../../model/commands/CommandDMA.h \
    ../../../model/commands/CommandVPRO.h \
    ../../../model/commands/CommandBase.h \
    ../../../model/commands/CommandSim.h \
    ../../../model/architecture/Pipeline.h \
    ../../../model/architecture/RegisterFile.h \
    ../../../model/architecture/stats/UnitStat.h \
    ../../../model/architecture/stats/DMAStat.h \
    ../../../model/architecture/stats/SimStat.h \
    ../../../model/architecture/stats/LaneStat.h \
    ../../../model/architecture/FIFO.h \
    ../../../model/architecture/HFIFO.h \
    ../../../model/architecture/VectorUnit.h \
    ../../../model/architecture/VectorLaneLS.h \
    ../../../model/architecture/VectorLane.h \
    ../../../model/architecture/DMA.h \
    ../../../model/architecture/Cluster.h \
    ../../../core_wrapper.h \
    ../../../iss_aux.h \
    ../../../simulator/ISS.h \
    ../../../simulator/VectorMain.h \
    ../../../simulator/windows/Commands/rftableview.h \
    ../../../simulator/windows/Commands/savesettings.h \
    ../../../simulator/windows/Commands/siminitfunctions.h \
    ../../../simulator/windows/Commands/spinneroverlay.h \
    ../../../simulator/windows/Commands/dataupdatefunctions.h \
    ../../../simulator/windows/Commands/lmtableview.h \
    ../../../simulator/windows/Commands/savemain.h \
    ../../../simulator/windows/Commands/simupdatefunctions.h \
    ../../../simulator/windows/Commands/commandstablewidget.h \
    ../../../simulator/windows/Commands/commandwindow.h \
    ../../../simulator/helper/debugHelper.h \
    ../../../simulator/helper/typeConversion.h \
    ../../../simulator/setting.h \
    ../../../core_dma_broadcast.h


FORMS += \
    commandwindow.ui \
    commandstablewidget.ui

INCLUDEPATH += \
    ../../../../common_lib/ \
    ../../../../iss_lib/

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
