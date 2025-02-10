# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 
# set paths to ISS

# priorities:
# - lib.cnf (DO NOT check in!)
# - env vpro_sim_lib_dir, vpro_aux_lib_dir
# - default

# default
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# !!                                                  !!
# !! NEVER, REALLY NEVER EVER MODIFY THE DEFAULT PATH !!
# !! TO ADAPT TO YOUR NON-STANDARD ISS LOCATION       !!
# !!                                                  !!
# !! Modifying the default path here WILL break the   !!
# !! environment for everyone else.                   !!
# !! The well established way to point to your non-   !!
# !! standard ISS location is to create a local       !!
# !! file 'lib.cnf', see below. USE IT!               !!
# !!                                                  !!
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
if(NOT DEFINED VPRO_SIMULATOR_LIB_dir)
  # DO NOT MODIFY THE DEFAULT PATH, use lib.cnf instead
  set(VPRO_SIMULATOR_LIB_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../../../TOOLS/VPRO/ISS/iss_lib") # DO NOT MODIFY THE DEFAULT PATH, use lib.cnf instead
  # DO NOT MODIFY THE DEFAULT PATH, use lib.cnf instead
endif()
if (NOT DEFINED VPRO_AUX_LIB_dir)
  # DO NOT MODIFY THE DEFAULT PATH, use lib.cnf instead
  set(VPRO_AUX_LIB_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../../../TOOLS/VPRO/ISS/common_lib") # DO NOT MODIFY THE DEFAULT PATH, use lib.cnf instead
  # DO NOT MODIFY THE DEFAULT PATH, use lib.cnf instead
endif()
# example lib.cnf (NEVER check in to git!):
# set(VPRO_SIMULATOR_LIB_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../../../TOOLS/VPRO/ISS/iss_lib")
# set(VPRO_AUX_LIB_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../../../TOOLS/VPRO/ISS/common_lib")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../lib.cnf")
  include(${CMAKE_CURRENT_SOURCE_DIR}/../lib.cnf)
  message(STATUS "[lib.cnf] loaded")
else()
  if(DEFINED ENV{vpro_sim_lib_dir})
    set(VPRO_SIMULATOR_LIB_dir "$ENV{vpro_sim_lib_dir}")
    message(STATUS "using ENV VPRO_SIM_LIB_DIR ${VPRO_SIMULATOR_LIB_dir}")
  endif()

  if(DEFINED ENV{vpro_aux_lib_dir})
    set(VPRO_AUX_LIB_dir "$ENV{vpro_aux_lib_dir}")
    message(STATUS "using ENV VPRO_AUX_LIB_DIR ${VPRO_AUX_LIB_dir}")
  endif()
endif()


set(FAILED_VAR_LIB 0)
if(NOT EXISTS "${VPRO_SIMULATOR_LIB_dir}")
  message(SEND_ERROR "VPRO_SIMULATOR_LIB_dir ${VPRO_SIMULATOR_LIB_dir} not found. Set ENV vpro_sim_lib_dir or set paths in lib.cnf")
  set(FAILED_VAR_LIB 1)
endif()

if(NOT EXISTS "${VPRO_AUX_LIB_dir}")
  message(SEND_ERROR "VPRO_AUX_LIB_dir ${VPRO_AUX_LIB_dir} not found. Set ENV vpro_aux_lib_dir or set paths in lib.cnf")
  set(FAILED_VAR_LIB 1)
endif()

if(${FAILED_VAR_LIB})
  return()
endif()

message(STATUS "[ISS config] using VPRO_SIMULATOR_LIB_dir=${VPRO_SIMULATOR_LIB_dir}")
message(STATUS "[ISS config] using VPRO_AUX_LIB_dir=${VPRO_AUX_LIB_dir}")
