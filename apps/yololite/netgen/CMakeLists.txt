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

project(netgen)

#######################################
# Compiler FLAGS                    ###
#######################################
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

if(NOT DEFINED RUN_LAYERS_DECOUPLED)
    set(RUN_LAYERS_DECOUPLED 0)
endif(NOT DEFINED RUN_LAYERS_DECOUPLED)
set(NETGEN_CONFIG_SWITCHES -DRUN_LAYERS_DECOUPLED=${RUN_LAYERS_DECOUPLED})

# -fdiagnostics-color: force color for output into pipe (used by main Makefile)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-parameter -fdiagnostics-color=always")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb")
#set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -ggdb") redundant
#set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb -fsanitize=address -fno-omit-frame-pointer")
#set(CMAKE_LINKER_FLAGS_DEBUG "-ggdb -fsanitize=address -fno-omit-frame-pointer") FIXME seems to be unused
set(CMAKE_CXX_FLAGS_RELEASE "-O3")


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

#######################################
### include Qt                      ###
#######################################
find_package(Qt5 COMPONENTS Core REQUIRED)
# FIXME replace Qt by std::

#######################################
### common netgen library           ###
#######################################
set(NetgenIncludeDirs
  ./
  ../bif/
  ${VPRO_AUX_LIB_dir}/
  ${VPRO_AUX_LIB_dir}/vpro/
  ${VPRO_SIMULATOR_LIB_dir}/model/commands/
)

# source files for netgen lib
file(GLOB_RECURSE NetgenSources
  *.h
  *.cpp
  ../bif/*.cpp
)
# remove build folder from source list
# https://stackoverflow.com/questions/15550777/how-do-i-exclude-a-single-file-from-a-cmake-fileglob-pattern
list(FILTER NetgenSources EXCLUDE REGEX ".*build.*$")


include_directories(${NetgenIncludeDirs}) # for all targets
add_definitions(${VPRO_CONFIG_SWITCHES}) # for all targets
add_definitions(${NETGEN_CONFIG_SWITCHES})

add_library(netgen STATIC ${NetgenSources})
target_link_libraries(netgen Qt5::Core)


#######################################
### one target per net              ###
#######################################

# fallback: manual list of known CNNs (subdirs of nets/)
#set(NETS "testlayer;yololite")

# automatically create Makefile targets for all available nets (../nets/*)
file(GLOB NETS RELATIVE ${CMAKE_SOURCE_DIR}/../nets ${CMAKE_SOURCE_DIR}/../nets/*)
foreach(NETNAME ${NETS})         # name of netgen executable and cmake target = subdir of nets/
  set(netdir ${CMAKE_SOURCE_DIR}/../nets/${NETNAME}/src)    # net home dir
  if(NOT IS_DIRECTORY ${netdir})
#    message(STATUS "[cmake netdir] discarding candidate ${netdir}")
    continue()
  endif()
  set(netgen_name ${NETNAME}_gen)
  
  message(STATUS "[cmake netgen] found CNN ${NETNAME} in ${netdir}")

  # net-specific source files
  file(GLOB NET_SOURCES ${netdir}/*.cpp ${netdir}/*.h)
  
  add_executable(${netgen_name} ${NET_SOURCES})
  target_include_directories(${netgen_name} PUBLIC ${netdir}/ )
  target_include_directories(${netgen_name} PUBLIC ${CMAKE_SOURCE_DIR}/Base/ )
  target_link_libraries(${netgen_name} netgen)

endforeach()

