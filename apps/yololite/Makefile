# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 
# Examples:
# make build_yololite_gen   # build netgen for nets/yololite
# make run_yololite_gen     # execute yololite netgen
# make sim_yololite         # build & execute yololite_gen, build & execute ISS
# make gdb_sim_yololite     # build & execute yololite_gen, build & execute ISS with debug symbols, execute in gdb
# make -j sim_yololite emu_yololite # execute ISS and emulation in parallel

# make <x> VERBOSE_BUILD=1  # debug build process
# make <x> DEBUG=1          # build debug-enabled executable
# make %sim_% INTERACTIVE=1 # start ISS in interactive mode (with window)

# Logfiles:
# - netgen/build/[c]make.log  build libnetgen
# - sim/build/[c]make.log     build ISS
# - runtime/make.log          build EISV executable
# - nets/%/*.log              build & run net generators, execute sim/emu

# Dependencies:
#                                 nets/%/weights/*
# netgen/*.[cpp|h] --> libnetgen  |
#                          v      v       (%=[testlayer|yololite|...])
# nets/%/*.[cpp|h] ----> %_gen ---+-> nets/%/generated/[*blob.bin|*.txt]
#                          ^          nets/%/[init/input.cfg|exit/output.cfg]
#                  bif/* --+                           v
#                          v                           |
# runtime/*.[cpp|h], sim/sim.cpp ----> sim ------------+--> nets/%/sim_results/*, logs
#                                  (executable)

# policy: use clean dependencies to enable parallel build/execution/simulation of multiple nets (make -j)

# bash redirection syntax used by some targets
SHELL := env bash

default: run_yololite_gen

#-------------------------------------------------------------------------------
# Hardware configuration
#-------------------------------------------------------------------------------
# VPRO
CLUSTERS	?= 8
UNITS		?= 8
LANES		?= 2
# DCMA
NR_RAMS		?= 8
LINE_SIZE	?= 4096
ASSOCIATIVITY	?= 8
RAM_SIZE	?= 524288 # =4096*64/8

BUILD_SIM ?= sim/build
BUILD_NETGEN ?= netgen/build

# pass vpro config swtiches to cmake, not make: let cmake figure out what has to be rebuilt
VPRO_CONFIG_SWITCHES:= -DCLUSTERS=${CLUSTERS} -DUNITS=${UNITS} -DLANES=${LANES} -DNR_RAMS=${NR_RAMS} -DLINE_SIZE=${LINE_SIZE} -DASSOCIATIVITY=${ASSOCIATIVITY} -DRAM_SIZE=${RAM_SIZE}

#-------------------------------------------------------------------------------
# Compile options
#-------------------------------------------------------------------------------
NETGEN_CMAKE_OPTS:=${VPRO_CONFIG_SWITCHES}
NETGEN_MAKE_OPTS:=-C ${BUILD_NETGEN}

SIM_CMAKE_OPTS:=${VPRO_CONFIG_SWITCHES}
SIM_MAKE_OPTS:=-C ${BUILD_SIM}


# debug build process
VERBOSE_BUILD?=0
ifeq ($(VERBOSE_BUILD),0)
NETGEN_MAKE_OPTS+=-s -j
SIM_MAKE_OPTS+=-s -j
else
NETGEN_CMAKE_OPTS+=-DVERBOSE_BUILD=1
SIM_CMAKE_OPTS+=-DVERBOSE_BUILD=1
endif

# debug netgen and sim executables
DEBUG?=0

# failed attempts to set DEBUG=1 globally BEFORE the following if/else are executed, depeding on target gdb_%:
#gdb_%: DEBUG:=1 # no effect; DEBUG is only set for prerequisites but not for the if/else below
#gdb_%: $(eval DEBUG:=1) # DEBUG is always 1, even if gdb_% is not a target
#gdb_%: # no other target is executed
#	$(eval DEBUG:=1)
# --> overwrite (NETGEN|SIM)_CMAKE_OPTS in rules gdb_% -> inherited by prerequisites

ifeq ($(DEBUG),0)
NETGEN_CMAKE_OPTS+=-DCMAKE_BUILD_TYPE=Release
SIM_CMAKE_OPTS+=-DCMAKE_BUILD_TYPE=Release
else
NETGEN_CMAKE_OPTS+=-DCMAKE_BUILD_TYPE=Debug
SIM_CMAKE_OPTS+=-DCMAKE_BUILD_TYPE=Debug
endif

# base_net::run_layers_decoupled
RLD?=0
NETGEN_CMAKE_OPTS+=-DRUN_LAYERS_DECOUPLED=$(RLD)

INTERACTIVE?=0
SIM_CLPARAMS:=
ifeq ($(INTERACTIVE),0)
SIM_CLPARAMS+=--windowless
endif


# \n forces the message to start on it's own line in case of missing newline
SUCCESS_MSG = "\n[make] $@ SUCCESS\n"


#-------------------------------------------------------------------------------
# targets for all nets/
#-------------------------------------------------------------------------------
# automatically create targets for all available nets (nets/*)
NETS:=$(patsubst nets/%,%,$(shell find nets/ -mindepth 1 -maxdepth 1 -type d))
# fallback: manual list of known CNNs (subdirs of netgen/nets/)
#NETS:=testlayer yololite

NETGENS:=$(NETS:%=build_%_gen)

# build netgens for all nets
all: $(NETGENS)

# run all netgens
genall: $(NETS:%=run_%_gen)

simall: $(NETS:%=sim_%)

dumpall: $(NETS:%=dump_%)

# simulate all nets that are not generated (and never called) by nn_quant
# you may want to use "make -j -k verify_sim" for parallel execution
verify_%: %_yololite %_dconvtest1_offsetconv %_dconvtest2_deform %_dconvtest3_conv
	@true # for some unknown reason, this rule must not be empty

# compile everything without building a net-specific netgen; useful to check for compilation errors
compile: libnetgen ${BUILD_SIM}/sim build_runtime

#-------------------------------------------------------------------------------
# netgens
#-------------------------------------------------------------------------------

# common stuff for all nets (build once)

# let cmake update build/Makefile
# always executed exactly once if one or multiple netgen(s) are to be built
.PHONY: cmake_common_netgen
cmake_common_netgen:
	mkdir -p ${BUILD_NETGEN}
	set -o pipefail ; cmake -S netgen -B ${BUILD_NETGEN} $(NETGEN_CMAKE_OPTS) |& tee ${BUILD_NETGEN}/cmake.log

# build libnetgen
# always executed exactly once if one or multiple netgen(s) are to be built
.PHONY: libnetgen
libnetgen: cmake_common_netgen
	set -o pipefail ; $(MAKE) $(NETGEN_MAKE_OPTS) netgen |& tee ${BUILD_NETGEN}/make.log

# build netgen executable(s) if any input changed (as figured out by cmake)
# invoke Makefile generated by targets libnetgen->cmake_common_netgen
# parallel build (-j) of multiple netgens: no common target files in parallel make calls (each target has it's individual subdir in nets/)


.PHONY: build_no_libnetgen_%_gen
build_no_libnetgen_%_gen:
	set -o pipefail ; $(MAKE) $(NETGEN_MAKE_OPTS) $*_gen |& tee nets/$*/$@.log
	@printf $(SUCCESS_MSG) | tee -a nets/$*/$@.log


.PHONY: build_%_gen
build_%_gen: libnetgen
	set -o pipefail ; $(MAKE) $(NETGEN_MAKE_OPTS) $*_gen |& tee nets/$*/$@.log
	@printf $(SUCCESS_MSG) | tee -a nets/$*/$@.log


# cmake dynamically determines if %_gen has to be rebuilt while this makefile is executing it's dependency graph
# -> this instance of make can not statically figure out if %_gen has to be run
.PHONY: gen_% run_%_gen
run_%_gen gen_%: build_%_gen
	$(MAKE) nets/$*/generated/eisvblob.bin

# only run %_gen if any of it's inputs changed
# cd required for netgen to find it's input files
nets/%/generated/eisvblob.bin: ${BUILD_NETGEN}/%_gen nets/%/weights/*
	@echo "== Running $*_gen"
	set -o pipefail ; cd nets/$* && ../../${BUILD_NETGEN}/$*_gen |& tee run_$*_gen.log
	@printf $(SUCCESS_MSG) | tee -a nets/$*/run_$*_gen.log

# run netgen executable in gdb
.PHONY: gdb_gen_% gdb_run_%_gen
# pattern-specific variable values are inherited by all prerequisites, important here: cmake_common_netgen
gdb_run_%_gen gdb_gen_%: NETGEN_CMAKE_OPTS:=$(subst Release,Debug,${NETGEN_CMAKE_OPTS})
gdb_run_%_gen gdb_gen_%: build_%_gen
	@echo "== Running $*_gen in gdb"
	cd nets/$* && gdb ../../${BUILD_NETGEN}/$*_gen

#-------------------------------------------------------------------------------
# simulation
#-------------------------------------------------------------------------------

.PHONY: cmake_common_sim
cmake_common_sim:
	mkdir -p ${BUILD_SIM}
	set -o pipefail ; cmake -S sim -B ${BUILD_SIM} $(SIM_CMAKE_OPTS) |& tee ${BUILD_SIM}/cmake.log

.PHONY: ${BUILD_SIM}/sim
${BUILD_SIM}/sim: cmake_common_sim
	set -o pipefail ; $(MAKE) $(SIM_MAKE_OPTS) sim |& tee ${BUILD_SIM}/make.log

.PHONY: sim_%
sim_%: run_%_gen ${BUILD_SIM}/sim
	mkdir -p nets/$*/sim_results
	set -o pipefail ; cd nets/$*/sim_results && ../../../${BUILD_SIM}/sim ${SIM_CLPARAMS} |& tee ../$@.log
	@printf $(SUCCESS_MSG) | tee -a nets/$*/$@.log

# run simulator executable in gdb
.PHONY: gdb_sim_%
# pattern-specific variable values are inherited by all prerequisites, important here: cmake_common_sim
gdb_sim_%: SIM_CMAKE_OPTS:=$(subst Release,Debug,${SIM_CMAKE_OPTS})
gdb_sim_%: run_%_gen ${BUILD_SIM}/sim
	mkdir -p nets/$*/sim_results
	cd nets/$*/sim_results && gdb --args ../../../sim/build/sim ${SIM_CLPARAMS}

#-------------------------------------------------------------------------------
# emulation
#-------------------------------------------------------------------------------
EMU_BOARD ?= aldec
INSTALL_DIR ?= /home/xilinx/EIS-V_bin/cnn_generic
BITSTREAM_DIR ?= /home/xilinx/overlays
BITSTREAM ?= ${CLUSTERS}c${UNITS}u_reference_fast.bit
EMU_TIME ?= 10
# legacy/default ./cnn_generic.py, ./iolibs/iobase/cnn_generic_new.py
CNN_SCRIPT ?= ./cnn_generic_new.py


# runtime binaries are net-agnostic
.PHONY: build_runtime
build_runtime:
	set -o pipefail ; make -C runtime CLUSTERS="${CLUSTERS}" UNITS="${UNITS}" all_but_sim_export |& tee runtime/make.log

# convert net-specific data to format required by RTL sim (conceptually broken, runtime/Makefile should contain only net-agnostic rules)
# prerequisite: build_runtime_% uses output generated by %_gen
.PHONY: build_runtime_%
build_runtime_%: run_%_gen
	set -o pipefail ; make -C runtime CNN="$*" CLUSTERS="${CLUSTERS}" UNITS="${UNITS}" all |& tee runtime/make.log

# enable parallel build of %_gen and runtime, force emu to be executed after that
.PHONY: emu_%
emu_%: run_%_gen build_runtime_% #.WAIT plain_emu_%* \n @true
	$(MAKE) plain_emu_$*

# enable parallel build of %_gen and runtime, force emu to be executed after that
.PHONY: emu_no_runtime_%
emu_no_runtime_%: run_%_gen
	$(MAKE) plain_emu_$*
	set -o pipefail ; cp nets/$*/emu_$*.log nets/$*/emu_no_runtime_$*.log

# gnu make < 4.4 does not know .WAIT
#.WAIT:

# execute emulation without rebuilding prerequisites
.PHONY: plain_emu_% eis_plain_emu_% other_plain_emu_%
plain_emu_%:
	@if [[ "`hostname -I`" =~ "134.169." ]]; then \
	echo -e "Executing EIS specific emu! [Host IP in range 134.169.x.x]\n"; \
	make -s eis_plain_emu_$*; \
	else \
	make -s other_plain_emu_$*; \
	fi

# Whether to delete or to keep the tmp folder with execution results on FPGA board (default: delete to avoid space overflow on SD card)
# The folder is located in /tmp and gets a random name upon execution
KEEP_TMP ?= 0
eis_plain_emu_%:
	mkdir -p nets/$*/emu_results
	cd runtime && python3 cnn_client.py --cnn ../nets/$* -c ${CLUSTERS} -u ${UNITS} --keep ${KEEP_TMP} --bitstream ${BITSTREAM_DIR}/${BITSTREAM} |& tee ../nets/$*/emu_$*.log
	@printf $(SUCCESS_MSG) | tee -a nets/$*/emu_$*.log

other_plain_emu_%:
	mkdir -p nets/$*/emu_results
	set -o pipefail ; cd nets/$*/ && rsync -avI generated/*blob.bin ${EMU_BOARD}:${INSTALL_DIR}/generated/ |& tee emu_$*.log
	set -o pipefail ; cd nets/$*/ && rsync -avIrL input ${EMU_BOARD}:${INSTALL_DIR}/ |& tee -a emu_$*.log
	set -o pipefail ; cd nets/$*/ && rsync -avI init/input.cfg ${EMU_BOARD}:${INSTALL_DIR}/input/ |& tee -a emu_$*.log
	set -o pipefail ; cd nets/$*/ && rsync -avI exit/output.cfg ${EMU_BOARD}:${INSTALL_DIR}/output/ |& tee -a emu_$*.log
	set -o pipefail ; cd runtime/ && rsync -avI bin/main.bin ${EMU_BOARD}:${INSTALL_DIR}/main.bin |& tee -a ../nets/$*/emu_$*.log
	set -o pipefail ; rsync -avI runtime/cnn_generic.py ${EMU_BOARD}:/home/xilinx/python_scripts/cnn/ |& tee -a nets/$*/emu_$*.log
	set -o pipefail ; rsync -avI runtime/cnn_generic_new.py ${EMU_BOARD}:/home/xilinx/python_scripts/cnn/ |& tee -a nets/$*/emu_$*.log
	set -o pipefail ; rsync -avIr --exclude *.pyc --exclude __pycache__ runtime/iolibs ${EMU_BOARD}:/home/xilinx/python_scripts/cnn/ |& tee -a nets/$*/emu_$*.log
	set -o pipefail ; ssh ${EMU_BOARD} "source /etc/profile; \
	source .profile; \
	source .bashrc; \
	sed -i 's/sim_results/emu_results/g' ${INSTALL_DIR}/output/output.cfg; \
	cd python_scripts/cnn/; \
	sudo -E ${CNN_SCRIPT} \
	--bitstream ${BITSTREAM_DIR}/${BITSTREAM} \
	-bin ${INSTALL_DIR}/main.bin \
	-pwd ${INSTALL_DIR}/output/ \
	-in ${INSTALL_DIR}/input/input.cfg \
	-out ${INSTALL_DIR}/output/output.cfg \
	-c ${CLUSTERS} -u ${UNITS} -t ${EMU_TIME} \
	-no_oskip -no_iskip;" |& tee -a nets/$*/emu_$*.log
	set -o pipefail ; rsync -avIrL ${EMU_BOARD}:${INSTALL_DIR}/emu_results nets/$*/ |& tee -a nets/$*/emu_$*.log
	set -o pipefail ; rsync -avIL ${EMU_BOARD}:${INSTALL_DIR}/*.log nets/$*/emu_results/ |& tee -a nets/$*/emu_$*.log
	@printf $(SUCCESS_MSG) | tee -a nets/$*/emu_$*.log


#-------------------------------------------------------------------------------
# run layers decoupled
#-------------------------------------------------------------------------------
# ref_fixed for all intermediate layers required; obtain by e.g. running nn_quant with evaluate_all_outputs=True
cp_ref_to_input_%:
	rm -f nets/$*/input/*.dec nets/$*/input/*.hex
	cp -fp nets/$*/ref_fixed/*.bin nets/$*/input

rld_build_%_gen: cp_ref_to_input_%
	$(MAKE) build_$*_gen RLD=1

rld_run_%_gen: cp_ref_to_input_%
	$(MAKE) run_$*_gen RLD=1

rld_sim_%: cp_ref_to_input_%
	$(MAKE) sim_$* RLD=1

rld_emu_%: cp_ref_to_input_%
	$(MAKE) emu_$* RLD=1

#-------------------------------------------------------------------------------
# misc
#-------------------------------------------------------------------------------
.PHONY: dump_%
dump_%:
	./dumpdata.sh nets/$*

FILTER?=nnqtest_*
filter_results:
	-@shopt -s nullglob ; grep -i -E "error|warn" nets/${FILTER}/*.log /dev/null | grep -v "main.elf" | grep -v "Abs. error" | grep -v "Quantization error"
	-@shopt -s nullglob ; ls -al -d nets/${FILTER}/vis*/
	-@shopt -s nullglob ; tail -n 1 -q nets/${FILTER}/build_*.log nets/${FILTER}/run_*.log nets/${FILTER}/sim_*.log nets/${FILTER}/emu_*.log | grep -v SUCCESS

.PHONY: clean
clean:
	$(MAKE) $(NETGEN_MAKE_OPTS) clean
	$(MAKE) $(SIM_MAKE_OPTS) clean
	$(MAKE) -C runtime/ -s clean

.PHONY: cleanall
cleanall: cleannnqtests
	rm -rf netgen/build
	rm -rf sim/build
	rm -rf runtime/bin
	rm -rf cache
	rm -rf build*

# naming convention: tests auto-generated by nn_quantization start with nnqtest_
.PHONY: cleannnqtests
cleannnqtests:
	rm -rf nets/nnqtest_* netgen/build/nnqtest_* netgen/build/CMakeFiles/nnqtest_*

.PHONY: info
info:
	@echo Available nets: $(NETS)
	@echo Netgens: $(NETGENS)
	@echo NETGEN_CMAKE_OPTS: ${NETGEN_CMAKE_OPTS}
	@echo NETGEN_MAKE_OPTS: ${NETGEN_MAKE_OPTS}
	@echo SIM_CMAKE_OPTS: ${SIM_CMAKE_OPTS}
	@echo SIM_MAKE_OPTS: ${SIM_MAKE_OPTS}
	@echo VERBOSE_BUILD: ${VERBOSE_BUILD}
	@echo DEBUG: ${DEBUG}
	@echo INTERACTIVE: ${INTERACTIVE}
	@echo SIM_CLPARAMS: ${SIM_CLPARAMS}


.PHONY: TAGS
TAGS:
	find -name \*.cpp > srcfiles.txt
	find -name \*.h >> srcfiles.txt
	find ../../../TOOLS/VPRO/ISS/ -name \*.cpp >> srcfiles.txt
	find ../../../TOOLS/VPRO/ISS/ -name \*.h >> srcfiles.txt
	find runtime/ -name \*.py >> srcfiles.txt
	grep -v -E '/build/|/legacy/|/nnqtest_|/\.git/|/\.#' srcfiles.txt | sort >> srcfiles.tmp
	mv -f srcfiles.tmp srcfiles.txt
	xargs -a srcfiles.txt etags
