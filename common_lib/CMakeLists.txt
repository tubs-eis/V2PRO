# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 
###
#
#	This Library uses all cpp files to reduce include mess in application
#	linking agains this library allows e.g. use of
#		- vpro_globals.cpp: HW config struct
#		-
#	most other functions are implemented as header-only libray
##

cmake_minimum_required(VERSION 3.14)
set(LIB_NAME "VPRO_AUX_LIB")

#############################################################################################
# Compiler FLAGS
#############################################################################################
macro(use_cxx11)
    if (CMAKE_VERSION VERSION_LESS "3.1")
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17")
        endif ()
    else ()
        set(CMAKE_CXX_STANDARD 17)
    endif ()
endmacro(use_cxx11)
use_cxx11()
set(GFLAG -std=c++17)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

#############################################################################################
# Lib Paramters
#############################################################################################
if(NOT DEFINED SIMULATION)
    set(SIMULATION 1)
endif(NOT DEFINED SIMULATION)
message(STATUS "[COMMON-LIB] using SIMULATION=${SIMULATION}")
add_definitions(-DSIMULATION=${SIMULATION})

if(NOT DEFINED CLUSTERS)
    set(CLUSTERS 2)
    message(WARNING "[COMMON-LIB] [not defined!] using CLUSTERS=${CLUSTERS}")
endif(NOT DEFINED CLUSTERS)
if(NOT DEFINED UNITS)
    set(UNITS 2)
    message(WARNING "[COMMON-LIB] [not defined!] using UNITS=${UNITS}")
endif(NOT DEFINED UNITS)
if(NOT DEFINED LANES)
    set(LANES 2)
    message(WARNING "[COMMON-LIB] [not defined!] using LANES=${LANES}")
endif(NOT DEFINED LANES)
if(NOT DEFINED NR_RAMS)
    set(NR_RAMS 8)
    message(WARNING "[COMMON-LIB] [not defined!] using NR_RAMS=${NR_RAMS}")
endif(NOT DEFINED NR_RAMS)
if(NOT DEFINED LINE_SIZE)
    set(LINE_SIZE 4096)
    message(WARNING "[COMMON-LIB] [not defined!] using LINE_SIZE=${LINE_SIZE}")
endif(NOT DEFINED LINE_SIZE)
if(NOT DEFINED ASSOCIATIVITY)
    set(ASSOCIATIVITY 4)
    message(WARNING "[COMMON-LIB] [not defined!] using ASSOCIATIVITY=${ASSOCIATIVITY}")
endif(NOT DEFINED ASSOCIATIVITY)
if(NOT DEFINED RAM_SIZE)
    set(RAM_SIZE 524288)
    message(WARNING "[COMMON-LIB] [not defined!] using RAM_SIZE=${RAM_SIZE}")
endif(NOT DEFINED RAM_SIZE)
#############################################################################################
# Files
#############################################################################################
# source files for executable
file(GLOB_RECURSE Sources
    "vpro/*.cpp"
    "riscv/*.cpp"
)

# source files for executable
file(GLOB_RECURSE Headers
    "vpro/*.h"
    "riscv/*.h"
)
list(APPEND Headers "${CMAKE_CURRENT_SOURCE_DIR}/vpro.h")
list(APPEND Headers "${CMAKE_CURRENT_SOURCE_DIR}/eisv.h")
list(APPEND Headers "${CMAKE_CURRENT_SOURCE_DIR}/versioning.h")

set(PlainIncludeDirs
	  .
    ../iss_lib/
)

#############################################################################################
# Library Generation
#############################################################################################
add_library(${LIB_NAME} STATIC ${Sources} ${Headers})
target_include_directories(${LIB_NAME} PUBLIC ${PlainIncludeDirs})
target_compile_definitions(${LIB_NAME} PUBLIC -DCONF_LANES=${LANES} -DCONF_UNITS=${UNITS} -DCONF_CLUSTERS=${CLUSTERS} -DCONF_DCMA_NR_RAMS=${NR_RAMS} -DCONF_DCMA_LINE_SIZE=${LINE_SIZE} -DCONF_DCMA_ASSOCIATIVITY=${ASSOCIATIVITY} -DCONF_DCMA_RAM_SIZE=${RAM_SIZE})

# IDEs should put the headers in a nice place
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/" PREFIX "Header Files AUX" FILES ${Headers})

#target_link_libraries(${LIB_NAME} VPRO_SIMULATOR_LIB)




find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
## This depends on QT
target_include_directories(${LIB_NAME} PUBLIC ${Qt5Core_INCLUDE_DIRS})
target_include_directories(${LIB_NAME} PUBLIC ${Qt5Widgets_INCLUDE_DIRS})
target_compile_definitions(${LIB_NAME} PUBLIC ${Qt5Widgets_DEFINITIONS})
#set(CMAKE_AUTOGEN_VERBOSE ON)
target_link_libraries(${LIB_NAME}  Qt5::Core Qt5::Widgets)


target_compile_features(${LIB_NAME} PUBLIC cxx_std_20)
