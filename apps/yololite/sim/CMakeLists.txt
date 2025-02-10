# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 

cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0074 NEW)

project(sim)

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
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")


set( CMAKE_VERBOSE_MAKEFILE off )
if( VERBOSE_BUILD ) # usage: cmake -DVERBOSE_BUILD=1
  set( CMAKE_VERBOSE_MAKEFILE on )
else()
  set( CMAKE_VERBOSE_MAKEFILE off )
endif()

#######################################
# VPRO and ISS config               ###
#######################################
include(../iss_paths.cmake)
include(../vpro_config.cmake)


if(NOT DEFINED ISS_STANDALONE)
    set(ISS_STANDALONE 1)
endif(NOT DEFINED ISS_STANDALONE)
if(NOT DEFINED SIMULATION)
    set(SIMULATION 1)
endif(NOT DEFINED SIMULATION)

# source files for executable
file(GLOB_RECURSE Sources
	"../bif/*.cpp"
	"../runtime/*.cpp"
        sim.cpp
        )

# source files for executable
file(GLOB_RECURSE Headers
        "../bif/*.h"
        "../runtime/*.h"
        )

set(PlainIncludeDirs
	"../bif/"
	"../runtime/"
        ${VPRO_SIMULATOR_LIB_dir}
        ${VPRO_AUX_LIB_dir}
        )


set(module sim)

#############################################################################################
# Executable (Standalone Sim App) or Library (Virtual Prototype App)
#############################################################################################
if (ISS_STANDALONE EQUAL 1)
    add_executable(${module} ${Sources} ${Headers})
else()
    add_library(${module} SHARED ${Sources} ${Headers})
endif ()

target_compile_definitions(${module} PUBLIC ${VPRO_CONFIG_SWITCHES})
add_definitions(-DSIMULATION=${SIMULATION} -DISS_STANDALONE=${ISS_STANDALONE})

target_include_directories(${module} PUBLIC ${PlainIncludeDirs})
target_link_libraries(${module} VPRO_SIMULATOR_LIB)
target_link_libraries(${module} VPRO_AUX_LIB)


# includes VPRO_SIMULATOR_LIB library
# after compile_definitions to include them there!
add_subdirectory(${VPRO_SIMULATOR_LIB_dir} ${CMAKE_CURRENT_BINARY_DIR}/VPRO_SIMULATOR_LIB)
add_subdirectory(${VPRO_AUX_LIB_dir} ${CMAKE_CURRENT_BINARY_DIR}/VPRO_AUX_LIB)

#############################################################################################
# Executable: VPRO Hardware / Machine Code
#############################################################################################
add_custom_target(make_vpro COMMAND make install
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/<
        )
add_custom_target(make_asm COMMAND make asm
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/<
        )
