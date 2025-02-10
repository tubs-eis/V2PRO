# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 
#
# main CMAKE file
#   includes libs (sim, aux, vpro_cnn, ...)
#   defines executable (sim)
#

cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0074 NEW)

if(NOT DEFINED PROJECT)
    get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    string(REPLACE " " "_" ProjectId ${ProjectId})
    set(PROJECT ${ProjectId})
endif(NOT DEFINED PROJECT)
project(${PROJECT})

#############################################################################################
# Compiler FLAGS
#############################################################################################
macro(use_cxx11)
    if (CMAKE_VERSION VERSION_LESS "3.1")
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++20")
        endif ()
    else ()
        set(CMAKE_CXX_STANDARD 20)
    endif ()
endmacro(use_cxx11)
use_cxx11()
set(GFLAG -std=c++2a)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

#############################################################################################
# Paths and Files
#############################################################################################
# libs
set(VPRO_SIMULATOR_LIB_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../iss_lib")
set(VPRO_AUX_LIB_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../common_lib")

# check paths (cmake fails if this file is not found)
file(SIZE ${CMAKE_CURRENT_SOURCE_DIR}/../../common_lib/vpro.h vpro_common_include_file)

# source files for executable
file(GLOB_RECURSE Sources
        "sources/*.cpp"
        )

# source files for executable
file(GLOB_RECURSE Headers
        "includes/*.h"
        )

set(PlainIncludeDirs
        includes/
        ${CMAKE_CURRENT_SOURCE_DIR}/../../iss_lib/
        ${CMAKE_CURRENT_SOURCE_DIR}/../../common_lib/
        )

#############################################################################################
# Definitions
#############################################################################################
# set HW config via defines
if(NOT DEFINED CLUSTERS)
    set(CLUSTERS 1)
endif(NOT DEFINED CLUSTERS)
if(NOT DEFINED UNITS)
    set(UNITS 1)
endif(NOT DEFINED UNITS)
if(NOT DEFINED LANES)
    set(LANES 2)
endif(NOT DEFINED LANES)
if(NOT DEFINED SCRIPTED)
    set(SCRIPTED 1)
endif(NOT DEFINED SCRIPTED)
if(NOT DEFINED ISS_STANDALONE)
    set(ISS_STANDALONE 1)
endif(NOT DEFINED ISS_STANDALONE)
if(NOT DEFINED SIMULATION)
    set(SIMULATION 1)
endif(NOT DEFINED SIMULATION)

message(STATUS "using CLUSTERS=${CLUSTERS}")
message(STATUS "using UNITS=${UNITS}")
message(STATUS "using LANES=${LANES}")
message(STATUS "using COMMENT=${PROJECT}")
message(STATUS "using SCRIPTED=${SCRIPTED}")
message(STATUS "using ISS_STANDALONE=${ISS_STANDALONE}")

set(module sim)

#############################################################################################
# Executable (Standalone Sim App) or Library (Virtual Prototype App)
#############################################################################################
if (ISS_STANDALONE EQUAL 1)
    add_executable(${module} ${Sources} main.cpp ${Headers})
else()
    add_library(${module} SHARED ${Sources} main.cpp ${Headers})
endif ()

target_compile_definitions(${module} PUBLIC -DNUM_VECTORLANES=${LANES} -DNUM_VU_PER_CLUSTER=${UNITS} -DNUM_CLUSTERS=${CLUSTERS})
add_definitions(-DSCRIPTED=${SCRIPTED} -DSTAT_COMMENT=\"${PROJECT}\" -DSIMULATION=${SIMULATION} -DISS_STANDALONE=${ISS_STANDALONE})
#target_compile_definitions(${module} PUBLIC SCRIPTED=${SCRIPTED} NUM_VU_PER_CLUSTER=${UNITS} NUM_CLUSTERS=${CLUSTERS} STAT_COMMENT=\"${PROJECT}\" SIMULATION=${SIMULATION} ISS_STANDALONE=${ISS_STANDALONE})

# include dirs for libs
target_include_directories(${module} PUBLIC ${PlainIncludeDirs})
target_link_libraries(${module} VPRO_SIMULATOR_LIB)
target_link_libraries(${module} VPRO_AUX_LIB)

# these are all global variables -> assign to address above main_memory max address
if (ISS_STANDALONE EQUAL 1)
	message(INFO "LINKING all rodata to high address ;-) ISS Standalone fix to differ in DMA transfers!")
	target_link_options(${module} PUBLIC -Wl,--no-relax,--section-start=.rodata=0x0000000040000000)
endif ()

# includes VPRO_SIMULATOR_LIB library
# after compile_definitions to include them there!
add_subdirectory(${VPRO_SIMULATOR_LIB_dir} ${CMAKE_CURRENT_BINARY_DIR}/VPRO_SIMULATOR_LIB)
add_subdirectory(${VPRO_AUX_LIB_dir} ${CMAKE_CURRENT_BINARY_DIR}/VPRO_AUX_LIB)

#############################################################################################
# Notes and Deprecated Tries
#############################################################################################
# linker options:
#target_link_options(${module} PUBLIC -Wl,--section-start=glob=0x0000000060000000)   # these are the datas [const global arrays] defined in region glob -> assign to addres above main_memory max address
#target_link_options(${module} PUBLIC -Wl,--no-check-sections)
#target_link_options(${module} PUBLIC -Wl,--print-map)
#target_link_options(${module} PUBLIC -Wl,--verbose)

#######################################
### include VPRO CNN LIB            ###
#######################################
## includes VPRO_CNN_LIB Library
#set(VPRO_CNN_LIB_dir "${CMAKE_CURRENT_SOURCE_DIR}/../cnn_converter/vpro_lib_sim_only")
#add_subdirectory(${VPRO_CNN_LIB_dir} ${CMAKE_CURRENT_BINARY_DIR}/VPRO_CNN_LIB)
#target_include_directories(${module} PUBLIC ../cnn_converter/vpro_lib_sim_only/includes/)
## Depends on Simulator
#target_include_directories(VPRO_CNN_LIB PUBLIC ../../../src/)
#target_link_libraries(VPRO_CNN_LIB VPRO_SIMULATOR_LIB)
#target_compile_definitions(VPRO_SIMULATOR_LIB PUBLIC NUM_VU_PER_CLUSTER=${UNITS} NUM_CLUSTERS=${CLUSTERS} STAT_COMMENT=\"${PROJECT}\")
#target_link_libraries(${module} VPRO_CNN_LIB)

#######################################
### includes Checker                 ##
#######################################
#set(CHECKER_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../host/verifyHW/checker/")
#add_subdirectory(${CHECKER_dir} ${CMAKE_CURRENT_BINARY_DIR}/checker)

#######################################
### include OpenCV                  ###
#######################################
#find_package(OpenCV REQUIRED)
#include_directories(${OpenCV_INCLUDE_DIRS})
#target_link_libraries(${module} ${OpenCV_LIBS})

#######################################
### Link agains PNGLIB              ###
#######################################
#target_link_libraries(${module} ${PNG_LIBRARY})

#######################################
### Link agains Darknet             ###
#######################################
#find_library(DARKNET_LIBRARY NAMES darknet
#             HINTS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR})
#
#target_link_libraries(${module} ${DARKNET_LIBRARY})

#######################################
### Link agains XTensor             ###
#######################################
#find_package(xtl REQUIRED PATHS ${CMAKE_SOURCE_DIR}/lib/xtl/install/)
#find_package(xtensor REQUIRED PATHS ${CMAKE_SOURCE_DIR}/lib/xtensor/install/)
#target_include_directories(${module} PUBLIC ${xtensor_INCLUDE_DIRS})
#target_link_libraries(${module}  xtensor)

#######################################
### Link agains Header with Weights ###
#######################################
#add_library(weightsLIB16 SHARED
#	sources/weights/yolo_lite_manual.cpp
#	includes/weights_yolo_manual.h
#)
#target_link_libraries(${module} weightsLIB16)

#####################################
### Link agains BOOST             ###
#####################################
#set(Boost_USE_DEBUG_LIBS         OFF) # ignore debug libs and
#set(Boost_USE_RELEASE_LIBS       ON)  # only find release libs
#set(Boost_USE_MULTITHREADED      ON)
#find_package( Boost 1.65.1 COMPONENTS thread REQUIRED )
#message(STATUS "Boost version: ${Boost_VERSION}")
#include_directories(${Boost_INCLUDE_DIR})
#include_directories(${Boost_INCLUDE_DIRS})
#target_link_libraries(${module} ${Boost_LIBRARIES})
