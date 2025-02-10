# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 
# VPRO HW config via #defines


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


set(VPRO_CONFIG_SWITCHES -DCONF_LANES=${LANES} -DCONF_UNITS=${UNITS} -DCONF_CLUSTERS=${CLUSTERS} -DCONF_DCMA_NR_RAMS=${NR_RAMS} -DCONF_DCMA_LINE_SIZE=${LINE_SIZE} -DCONF_DCMA_ASSOCIATIVITY=${ASSOCIATIVITY} -DCONF_DCMA_RAM_SIZE=${RAM_SIZE})

message(STATUS "[VPRO config] CLUSTERS=${CLUSTERS}")
message(STATUS "[VPRO config] UNITS=${UNITS}")
message(STATUS "[VPRO config] LANES=${LANES}")
message(STATUS "[VPRO config] NR_RAMS=${NR_RAMS}")
message(STATUS "[VPRO config] LINE_SIZE=${LINE_SIZE}")
message(STATUS "[VPRO config] ASSOCIATIVITY=${ASSOCIATIVITY}")
message(STATUS "[VPRO config] RAM_SIZE=${RAM_SIZE}")
