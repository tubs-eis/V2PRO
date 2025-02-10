# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 
# Helper for running cmake in the build folder "build"

#-------------------------------------------------------------------------------
# Make defaults
#-------------------------------------------------------------------------------
.SUFFIXES:
.DEFAULT_GOAL := help

.PHONY: all install
# simulation is default
all: clean dir release

#-------------------------------------------------------------------------------
# Hardware definitions
#-------------------------------------------------------------------------------
# VPRO
CLUSTERS	?= 2
UNITS		?= 2
LANES		?= 2
# DCMA
NR_RAMS		?= 8
LINE_SIZE	?= 4096
RAM_SIZE	?= 524288
ASSOCIATIVITY	?= 4

APP_NAME	?= "TemplateApp"
STANDALONE	?= 1

# generated binary file names
APP=main
INSTALL_DIR=aldec:/home/xilinx/EIS-V_bin/

# Paths for HW
LIB_DIR=../../common_lib/riscv/lib_ddr_sys/
PREFIX?=${RISCV}/bin/riscv32-unknown-elf

#-------------------------------------------------------------------------------
# Application definitions
#-------------------------------------------------------------------------------
build ?= build
build_release ?= build_release

current_dir = $(shell pwd)
PROJECT_NAME ?= $(current_dir)

# pass configuration as parameters to cmake script
ISS_FLAGS=-DCLUSTERS=${CLUSTERS} -DUNITS=${UNITS} -DLANES=${LANES} -DPROJECT=${PROJECT_NAME}
ISS_FLAGS += -DNR_RAMS=${NR_RAMS} -DLINE_SIZE=${LINE_SIZE} -DASSOCIATIVITY=${ASSOCIATIVITY} -DRAM_SIZE=${RAM_SIZE}
ISS_FLAGS += -DAPP_NAME=${APP_NAME} -DREPO_DIR=${REPO_DIR}
ifeq (${STANDALONE},0)
	ISS_FLAGS+= -DISS_STANDALONE=0
else
	ISS_FLAGS+= -DISS_STANDALONE=1
endif

#-------------------------------------------------------------------------------
# Tools / Paths
#-------------------------------------------------------------------------------
HW_FLAGS= -O3 -static -mabi=ilp32 -march=rv32im_zicsr -Wall -pedantic
# -mdiv <- no difference
# -mcpu=sifive-e31 <- no difference
# -misa-spec=2.2 <- no difference (default: 20191213)
# -mbranch-cost=3 <- no difference in generated code 2 / 3 / none (0?)
HW_FLAGS+= -nostartfiles #-nostdlib # no std lib -> no printf, exit, ...
# HW_FLAGS+= -fno-use-cxa-atexit -Wabi-tag -D_GLIBCXX_USE_CXX11_ABI=0	# avoid __dso_handle missing error as https://stackoverflow.com/questions/34308720/where-is-dso-handle-defined
HW_FLAGS+= -std=c++2a -std=gnu++20	# std for use of C++20, gnu extensions for e.g. stdint (uint, ...)
#HW_FLAGS+= -DNDEBUG # removes asserts
HW_FLAGS+= -T ${LIB_DIR}/link.ld -L ${LIB_DIR} -lcv-verif
HW_FLAGS+= -I ${RISCV}/riscv32-unknown-elf/include/
HW_FLAGS+= -I ${LIB_DIR}/../../../common_lib/
HW_FLAGS+= -I ${LIB_DIR}/../../../iss_lib/
HW_FLAGS+= -I ./includes/

# remove unused sections
HW_FLAGS+= -fdata-sections -ffunction-sections
HW_LINK_FLAGS = -Wl,--gc-sections
# Remove warning:  main.elf has a LOAD segment with RWX permissions
#HW_LINK_FLAGS += -Wl,--no-warn-rwx-segment

VPRO_FLAGS = -DNUM_CLUSTERS=${CLUSTERS} -DNUM_VU_PER_CLUSTER=${UNITS} -DNUM_VECTORLANES=${LANES} -DSTAT_COMMENT=${APP_NAME}
VPRO_FLAGS += -DNR_RAMS=${NR_RAMS} -DLINE_SIZE=${LINE_SIZE} -DASSOCIATIVITY=${ASSOCIATIVITY}
HW_FLAGS += ${VPRO_FLAGS}

C_FILES = main.cpp
C_FILES += $(wildcard sources/*.cpp)
C_FILES += $(wildcard ${LIB_DIR}/../../../common_lib/vpro/*.cpp)
C_FILES += $(wildcard ${LIB_DIR}/../../../common_lib/riscv/*.cpp)
#C_FILES += $(COMMON_LIB)/mips_aux.cpp $(COMMON_LIB)/vpro_globals.cpp $(COMMON_LIB)/mem_functions.cpp


#-------------------------------------------------------------------------------
# Help
#-------------------------------------------------------------------------------
.PHONY: help test
help:
	@echo "VPRO \e[7m\e[1m ${APP_NAME} \e[0m\e[27m application compilation script"
	@echo "Makefile Targets:"
	@echo "--------------------------------------------------------------"
	@echo "Simulation Targets:"
	@echo "  \e[4mdir\e[0m            - creates (empty) build directories"
	@echo "  \e[4msim\e[0m            - compiles application with simulator (debug mode)"
	@echo "                   (gdb, etc. attachable to debug ISS functionality)"
	@echo "                   runs application with GUI"
	@echo "  \e[4mrelease\e[0m        - compiles application with simulator (release mode)"
	@echo "                   runs application with GUI"
	@echo "  \e[4mscripted\e[0m       - compiles application with maximum of 4 threads (release mode)"
	@echo "                   runs application in console mode (no GUI)"
	@echo "--------------------------------------------------------------"
	@echo "Hardware Build Targets (Machine Code):"
	@echo "  \e[4melf | $(APP).elf\e[0m          - Link all object files (all elf files)"
	@echo "  \e[4mbin | $(APP).bin\e[0m          - Generate binary executable from .elf"
	@echo "  \e[4mhex | $(APP).hex\e[0m          - Generate binary/string executable from .elf"
	@echo "  \e[4mobjdump | $(APP).objdump\e[0m  - Generate disassembler from .elf"
	@echo "  \e[4mreadelf | $(APP).readelf\e[0m  - Generate readelf information from .elf"
	@echo "--------------------------------------------------------------"
	@echo "Debug:"
	@echo "  \e[4mtest\e[0m           - print input files list + compile flags"
	@echo "--------------------------------------------------------------"
	@echo "Other Targets:"
	@echo "  \e[4mclean\e[0m          - Clean up this directory (all)"
	@echo "  \e[4mclean_sim\e[0m      - Clean up simulation directories + files"
	@echo "  \e[4mclean_hw\e[0m       - Clean up hw directories + files"
	@echo "  \e[4mhelp\e[0m           - Show this text"
	@echo "  \e[4mall\e[0m            - clean + sim + install"
	@echo "--------------------------------------------------------------"

# to debug
test:
	@echo "#####################################################################"
	@echo "\e[7mInput Source Files to compile:\e[0m"
	@$(foreach file,$(C_FILES),echo $(file);)
	@echo ""
	@echo "#####################################################################"
	@echo "\e[7mHW Flags:\e[0m"
	@echo $(HW_FLAGS)
	@echo "\e[7mHW Link Flags:\e[0m"
	@echo $(HW_LINK_FLAGS)
	@echo ""
	@echo "#####################################################################"
	@echo "\e[7mCreate Disassembly/asm by:\e[0m $(PREFIX)-objdump -SlCwrd ${APP}.elf"
	@echo "#####################################################################"


#-------------------------------------------------------------------------------
# Simulator (ISS) Targets
#-------------------------------------------------------------------------------
dir:
	mkdir -p ${build}
	mkdir -p ${build_release}

sim: dir
	cmake -B ${build} ${ISS_FLAGS}
	@$(MAKE) -s  -C ${build} sim -j
	cd ${build} && ./sim

release: dir
	cmake -B ${build_release} -Wno-dev -DCMAKE_BUILD_TYPE=Release ${ISS_FLAGS}
	$(MAKE) -s  -C ${build_release} sim -j
	cd ${build_release} && ./sim

console: dir
	cmake -B ${build_release} -Wno-dev -DCMAKE_BUILD_TYPE=Release ${ISS_FLAGS}
	$(MAKE) -s  -C ${build_release} sim -j
	cd ${build_release} && ./sim --windowless

# slow configuration. only 4 thread, set in defines.h: calc vpro, compare
scripted: dir
	cmake -B ${build_release} -Wno-dev -DCMAKE_BUILD_TYPE=Release ${ISS_FLAGS}
	$(MAKE) -s  -C ${build_release} sim -j 4
	cd ${build_release} && ./sim --windowless


#-------------------------------------------------------------------------------
# Application VPRO Hardware / Machine Code Targets
#-------------------------------------------------------------------------------
elf: ${APP}.elf
${APP}.elf: $(C_FILES)
	make -C ${LIB_DIR}
	${PREFIX}-g++ ${HW_FLAGS} $^ -o $@ ${HW_LINK_FLAGS}

hex: ${APP}.hex
${APP}.hex: ${APP}.bin
	xxd -g 4 $< | cut -d " " -f 2-5 > $@
#   -g 4 byte groups (32-bit) for imem word size
#   -e little endian byte order
#   cut removes address and ascii tranlsation
#
#	${RISCV}/bin/${PREFIX}-objcopy -O verilog $< $@
# [Info] dump of binary file in hex format to be loaded in simulation
#        .tcl Command: mem load -infile /.../fibonacci.hex -format hex -truncate /tb/design_1_i/InstructionRAM/U0/ram_inst/ram
# [Note] objcopy not used, due to address jumps (@...). Adress is byte-wise but interpreted element-wise by questa

readelf: ${APP}.readelf
${APP}.readelf: ${APP}.elf
	${PREFIX}-readelf --sym-base=16 -a $< > $@

objdump: ${APP}.objdump
${APP}.objdump: ${APP}.elf
	${PREFIX}-objdump -d -M no-aliases -M numeric -S $< > $@

bin: ${APP}.bin
${APP}.bin: ${APP}.elf
	${PREFIX}-objcopy -O binary -R .vproimage -R .nobss $< $@
	# swap endianness
	objcopy -I binary -O binary --reverse-bytes=4 $@ $@
	${PREFIX}-size $<

32hex: ${APP}.32hex
${APP}.32hex: ${APP}.bin
	xxd -g 4 -c 4 $< | cut -d " " -f 2-2 > $@

install: ${APP}.bin
	scp $< ${INSTALL_DIR}$<

#-------------------------------------------------------------------------------
# Clean-up
#-------------------------------------------------------------------------------
.PHONY: clean clean_sim clean_hw clean_sim_all
clean: clean_sim_all clean_hw
clean_hw:
	@echo "\n\tCleaning up Hardware workspace..."
	rm -f *.hex
	rm -f *.bin
	rm -f *.elf
	rm -f *.readelf
	rm -f *.objdump
	make -C ${LIB_DIR} clean

clean_sim_all:
	@echo "\n\tCleaning up Simulator workspace..."
	rm -rf ${build}
	rm -rf ${build_release}*
	rm -rf init/archive/*.cfg.old
	rm -rf exit/archive/*.cfg.old
	rm -rf data/statistic_out.csv
	rm -rf data/sim_cmd_history.log
	rm -rf data/out*.bin
	rm -rf data/out*.bmp
	rm -rf scripts/log/*.log

clean_sim: clean_sim_all cp

#-------------------------------------------------------------------------------
# eof
