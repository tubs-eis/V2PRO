# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 
# LIBRARY
# 	VPRO Simulator
#
# offers all vpro functions and simulates them
#
# usage:
# 	in Application's CMakeLists File:
# 		set(VPRO_SIMULATOR_LIB_dir "../../isa_intrinsic_lib")
# 		add_subdirectory(${VPRO_SIMULATOR_LIB_dir})
# 		target_link_libraries(${module} VPRO_SIMULATOR_LIB)

cmake_minimum_required(VERSION 3.14)
set(LIB_NAME "VPRO_SIMULATOR_LIB")
set(ISS_LIB_NAME "VPRO_SIMULATOR_LIB_ISS")
project("VPRO_SIMULATOR_LIB")
#############################################################################################
# Paths and Files
#############################################################################################

# check paths (cmake fails if this file is not found)
file(SIZE ${CMAKE_CURRENT_SOURCE_DIR}/../common_lib/vpro.h vpro_common_include_file)

# Source Files for this lib
file(GLOB_RECURSE Sources
	"*.cpp"
)

# remove main from GUI from simulator sources
list(REMOVE_ITEM Sources ${CMAKE_CURRENT_SOURCE_DIR}/simulator/windows/Commands/main.cpp)

# Note that headers are optional, and do not affect add_library, but they will not
# show up in IDEs unless they are listed in add_library.
file(GLOB_RECURSE Headers
 	"*.h"
 	"*.hh"
)

set(LibIncludeDirs
	./
	simulator/windows/Commands/
	${CMAKE_CURRENT_SOURCE_DIR}/../common_lib/
)

# IDEs should put the headers in a nice place
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/" PREFIX "Header Files SIM" FILES ${Headers})


#############################################################################################
# QT Linking (GUI + internals)
#############################################################################################
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/simulator/windows/Commands)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

#############################################################################################
# Library Creation
#############################################################################################
# Make an automatic library - will be static or dynamic based on user setting
#add_library(${LIB_NAME} ${Sources} ${Headers})

if(DEFINED ISS_STANDALONE)
       add_library(${LIB_NAME} ${Sources} ${Headers})
endif()

if(NOT DEFINED ISS_STANDALONE)
       add_library(${LIB_NAME} SHARED ${Sources} ${Headers})
       add_library(${ISS_LIB_NAME} SHARED ${Sources} ${Headers})

       ## This depends on QT
       target_include_directories(${ISS_LIB_NAME} PUBLIC ${Qt5Core_INCLUDE_DIRS})
       target_include_directories(${ISS_LIB_NAME} PUBLIC ${Qt5Widgets_INCLUDE_DIRS})
       target_compile_definitions(${ISS_LIB_NAME} PUBLIC ${Qt5Widgets_DEFINITIONS})
       #set(CMAKE_AUTOGEN_VERBOSE ON)
       target_link_libraries(${ISS_LIB_NAME}  Qt5::Core Qt5::Widgets)

       # We need this directory, and users of our library will need it too
       target_include_directories(${ISS_LIB_NAME} PUBLIC ${LibIncludeDirs})

       # All users of this library will need at least C++17
       target_compile_features(${ISS_LIB_NAME} PUBLIC cxx_std_17)
endif()


## This depends on QT
target_include_directories(${LIB_NAME} PUBLIC ${Qt5Core_INCLUDE_DIRS})
target_include_directories(${LIB_NAME} PUBLIC ${Qt5Widgets_INCLUDE_DIRS})
target_compile_definitions(${LIB_NAME} PUBLIC ${Qt5Widgets_DEFINITIONS})
#set(CMAKE_AUTOGEN_VERBOSE ON)
target_link_libraries(${LIB_NAME}  Qt5::Core Qt5::Widgets)

# We need this directory, and users of our library will need it too
target_include_directories(${LIB_NAME} PUBLIC ${LibIncludeDirs})

# All users of this library will need at least C++17
target_compile_features(${LIB_NAME} PUBLIC cxx_std_17)

#############################################################################################
# Definitions
#############################################################################################
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
	set(LINE_SIZE 1024)
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

# defines for core
if(DEFINED ISS_STANDALONE)
	message(STATUS "[ISS-LIB] Compile as Standalone ISS")
	target_compile_definitions(${LIB_NAME} PUBLIC IS_SIMULATION=1 SIMULATION=1 ISS_STANDALONE=1)
	target_compile_definitions(${LIB_NAME} PUBLIC -DCONF_LANES=${LANES} -DCONF_UNITS=${UNITS} -DCONF_CLUSTERS=${CLUSTERS} -DCONF_DCMA_NR_RAMS=${NR_RAMS} -DCONF_DCMA_LINE_SIZE=${LINE_SIZE} -DCONF_DCMA_ASSOCIATIVITY=${ASSOCIATIVITY} -DCONF_DCMA_RAM_SIZE=${RAM_SIZE})
endif()

if(NOT DEFINED ISS_STANDALONE)
	message("[ISS-LIB] Compile for Virtual Prototype")
	target_compile_definitions(${LIB_NAME} PUBLIC IS_SIMULATION=1 SIMULATION=1)
	target_compile_definitions(${LIB_NAME} PUBLIC -DCONF_LANES=${LANES} -DCONF_UNITS=${UNITS} -DCONF_CLUSTERS=${CLUSTERS} -DCONF_DCMA_NR_RAMS=${NR_RAMS} -DCONF_DCMA_LINE_SIZE=${LINE_SIZE} -DCONF_DCMA_ASSOCIATIVITY=${ASSOCIATIVITY} -DCONF_DCMA_RAM_SIZE=${RAM_SIZE})

	message("[ISS-LIB] Compile for ISS (as well ;) )")
	target_compile_definitions(${ISS_LIB_NAME} PUBLIC IS_SIMULATION=1 SIMULATION=1 ISS_STANDALONE=1)
	target_compile_definitions(${ISS_LIB_NAME} PUBLIC -DCONF_LANES=${LANES} -DCONF_UNITS=${UNITS} -DCONF_CLUSTERS=${CLUSTERS} -DCONF_DCMA_NR_RAMS=${NR_RAMS} -DCONF_DCMA_LINE_SIZE=${LINE_SIZE} -DCONF_DCMA_ASSOCIATIVITY=${ASSOCIATIVITY} -DCONF_DCMA_RAM_SIZE=${RAM_SIZE})
endif()
