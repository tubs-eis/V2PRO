# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 
#-------------------------------------------------------------------------------
# Make defaults
#-------------------------------------------------------------------------------
.SUFFIXES:
# .phony: always build these targets (no "is up to date" message)
.PHONY: help all allc clean asm install
.DEFAULT_GOAL := help

#-------------------------------------------------------------------------------
# Definitions
#-------------------------------------------------------------------------------
# VPRO
CLUSTERS	?= 8
UNITS		?= 8
LANES		?= 2

# DCMA
NR_RAMS		?= 16
LINE_SIZE	?= 1024
ASSOCIATIVITY ?= 4
RAM_SIZE ?= 4096*64/8

# App
APP_NAME ?= "CNN_Runtime"
RV_VPRO_EXT ?= 1
CNN ?= yololite

INSTALL_DIR	?= aldec:/home/xilinx/EIS-V_bin/cnn_rework/

#-------------------------------------------------------------------------------
# Tools / Paths for RISC-V Compile
#-------------------------------------------------------------------------------
# Paths for HW Compile
RISCV	?= /home/kock/prj/lucidair/riscv-gnu-toolchain/bin
PREFIX	?= ${RISCV}/bin/riscv32-unknown-elf
LIB_DIR	?= ../../../../TOOLS/VPRO/ISS/common_lib/riscv/lib_ddr_sys/
#LIB_DIR=../../../../TOOLS/VPRO/ISS/common_lib/riscv/lib_bram_eisv/
# Flags
HW_FLAGS= -O3 -static -mabi=ilp32 -march=rv32im_zicsr -Wall # -mdiv <- no difference
# -pedantic # maaaany warnings...
HW_FLAGS+= -Wno-unused-variable -Wno-format
#HW_FLAGS+= -misa-spec=2.2 <- no difference (default: 20191213)
HW_FLAGS+= -std=c++2a -std=gnu++20	# std for use of C++20, gnu extensions for e.g. stdint (uint, ...)
HW_FLAGS+= -nostartfiles	#-mbranch-cost=3 # no difference in generated code 2 / 3 / none (0?)
HW_FLAGS+= -DNDEBUG # removes asserts
HW_FLAGS+= -T ${LIB_DIR}/link.ld -L ${LIB_DIR} -lcv-verif
HW_FLAGS+= -I ${RISCV}/riscv32-unknown-elf/include/
HW_FLAGS+= -I ${LIB_DIR}/../../../common_lib/
HW_FLAGS+= -I ${LIB_DIR}/../../../iss_lib/
HW_FLAGS+= -I ../bif/
HW_FLAGS+= -I ./
#HW_FLAGS+= -I ./includes/

# remove unused sections
HW_FLAGS+= -fdata-sections -ffunction-sections
HW_LINK_FLAGS = -Wl,--gc-sections

VPRO_FLAGS = -DNUM_CLUSTERS=${CLUSTERS} -DNUM_VU_PER_CLUSTER=${UNITS} -DNUM_VECTORLANES=${LANES} -DSTAT_COMMENT=${APP_NAME}
VPRO_FLAGS += -DNR_RAMS=${NR_RAMS} -DLINE_SIZE=${LINE_SIZE} -DASSOCIATIVITY=${ASSOCIATIVITY} -DRAM_SIZE=${RAM_SIZE} -DRV_VPRO_EXT=${RV_VPRO_EXT}
VPRO_FLAGS +=-DCONF_LANES=${LANES} -DCONF_UNITS=${UNITS} -DCONF_CLUSTERS=${CLUSTERS} -DCONF_DCMA_NR_RAMS=${NR_RAMS} -DCONF_DCMA_LINE_SIZE=${LINE_SIZE} -DCONF_DCMA_ASSOCIATIVITY=${ASSOCIATIVITY} -DCONF_DCMA_RAM_SIZE=${RAM_SIZE}

HW_FLAGS += ${VPRO_FLAGS}

#C_FILES = main.cpp
C_FILES = $(wildcard *.cpp)
C_FILES += $(wildcard *.h)
C_FILES += $(wildcard ../bif/*.cpp)
C_FILES += $(wildcard ../bif/*.h)
C_FILES += $(wildcard ${LIB_DIR}/../../../common_lib/vpro/*.cpp)
C_FILES += $(wildcard ${LIB_DIR}/../../../common_lib/riscv/*.cpp)

APP=main
OUT_DIR=bin

#-------------------------------------------------------------------------------
# Help
#-------------------------------------------------------------------------------
# .phony: always build these targets (no "is up to date" message)
.PHONY: help
APP_DIR=$(shell basename $(CURDIR))
help:
	@echo "VPRO \e[7m\e[1m ${APP_DIR} | $(APP) \e[0m\e[27m application compilation script"
	@echo "Makefile Targets:"
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
	@echo "  \e[4mclean_hw\e[0m       - Clean up hw directories + files"
	@echo "  \e[4mhelp\e[0m           - Show this text"
	@echo "  \e[4mall\e[0m            - clean + install"
	@echo "--------------------------------------------------------------"

#-------------------------------------------------------------------------------
# Simulator (ISS) Targets
#-------------------------------------------------------------------------------
# Not applyable for runtime -> use sim.cpp

#-------------------------------------------------------------------------------
# Application VPRO Hardware / Machine Code Targets
#-------------------------------------------------------------------------------
elf: ${APP}.elf
${APP}.elf: ${OUT_DIR}/${APP}.elf
hex: ${APP}.hex
${APP}.hex: ${OUT_DIR}/${APP}.hex
32hex: ${APP}.32hex
${APP}.32hex: ${OUT_DIR}/${APP}.32hex
readelf: ${APP}.readelf
${APP}.readelf: ${OUT_DIR}/${APP}.readelf
bin: ${APP}.bin
${APP}.bin: ${OUT_DIR}/${APP}.bin
objdump: ${APP}.objdump
${APP}.objdump: ${OUT_DIR}/${APP}.objdump

# to debug
test:
	@echo "#####################################################################"
	@echo "\e[7mInput Source Files to compile:\e[0m"
	@$(foreach file,$(C_FILES),echo $(file);)
	@echo ""
	@echo "#####################################################################"
	@echo "\e[7mHW Flags:\e[0m"
	@echo $(HW_FLAGS)
	@echo ""
	@echo "#####################################################################"
	@echo "\e[7mCreate Disassembly/asm by: $(PREFIX)-objdump -SlCwrd ${APP}.elf\e[0m"
	@echo "#####################################################################"

hw-lib:
	make -s -C ${LIB_DIR} clean
	make -C ${LIB_DIR} all

${OUT_DIR}/${APP}.elf: hw-lib
	@mkdir -p ${OUT_DIR}
	@echo "\n################################################"
	@echo "Compiling the Runtime Application for Risc-V"
	@echo "  RV_VPRO_EXT: ${RV_VPRO_EXT}, ${CLUSTERS} C x ${UNITS} U"
	@echo "################################################"
	${PREFIX}-g++ ${HW_FLAGS} $(C_FILES) -o $@ ${HW_LINK_FLAGS}

${OUT_DIR}/${APP}.hex: ${OUT_DIR}/${APP}.bin
	@mkdir -p ${OUT_DIR}
	xxd -g 4 $< | cut -d " " -f 2-5 > $@
#   -g 4 byte groups (32-bit) for imem word size
#   -e little endian byte order
#   cut removes address and ascii tranlsation
#
#	${RISCV}/bin/${PREFIX}-objcopy -O verilog $< $@
# [Info] dump of binary file in hex format to be loaded in simulation
#        .tcl Command: mem load -infile /.../fibonacci.hex -format hex -truncate /tb/design_1_i/InstructionRAM/U0/ram_inst/ram
# [Note] objcopy not used, due to address jumps (@...). Adress is byte-wise but interpreted element-wise by questa

${OUT_DIR}/${APP}.32hex: ${OUT_DIR}/${APP}.bin
	xxd -g 4 -c 4 $< | cut -d " " -f 2-2 > $@

${OUT_DIR}/${APP}.readelf: ${OUT_DIR}/${APP}.elf
	mkdir -p ${OUT_DIR}
	${PREFIX}-readelf --sym-base=16 -a $< > $@

${OUT_DIR}/${APP}.objdump: ${OUT_DIR}/${APP}.elf
	mkdir -p ${OUT_DIR}
	${PREFIX}-objdump -d -M no-aliases -M numeric -S $< > $@

${OUT_DIR}/${APP}.bin: ${OUT_DIR}/${APP}.elf
	mkdir -p ${OUT_DIR}
	${PREFIX}-objcopy -O binary -R .vproimage -R .nobss_32byte_align -R .eisvblob_image -R .nobss $< $@
	# swap endianness
	objcopy -I binary -O binary --reverse-bytes=4 $@ $@
	${PREFIX}-size $<

#-------------------------------------------------------------------------------
# all goals
#-------------------------------------------------------------------------------
all: bin hex objdump readelf generate_sim_hex

all_but_sim_export: bin hex objdump readelf

# convert net-specific data to format required by RTL sim
generate_sim_hex: hex 32hex
	@mkdir -p ${OUT_DIR}/data
	cp ../nets/${CNN}/generated/*.bin ${OUT_DIR}/data/
	cp ../nets/${CNN}/input/*.bin ${OUT_DIR}/data/
	cp ../nets/${CNN}/init/input.cfg ${OUT_DIR}/data/
	#mkdir -p ${OUT_DIR}/unaligned
	#for f in ${OUT_DIR}/data/*.bin ; do cp $$f ${OUT_DIR}/unaligned/ ; done
	#python3 align_blob_files.py
	#for f in ${OUT_DIR}/data/*.bin ; do xxd -e -g 4 -c 4 $$f | cut -d " " -f 2-2 > $$f.32hex ; done
	#for f in ${OUT_DIR}/data/*.bin ; do xxd -e -g 4 $$f | cut -d " " -f 2-5 > $$f.hex ; done
	#for f in ${OUT_DIR}/data/*.bin ; do mv $$f $$f.aligned ; done
	#for f in ${OUT_DIR}/unaligned/*.bin ; do mv $$f ${OUT_DIR}/data/ ; done
	python3 sim_hex_init_file_gen.py
	cp ${OUT_DIR}/${APP}.mm_init.bin ${OUT_DIR}/${APP}.mm_init
	sed -i "s/\.bin/\.bin\.hex/g" ${OUT_DIR}/${APP}.mm_init
	cp ${OUT_DIR}/${APP}.mm_init.bin ${OUT_DIR}/${APP}.mm_init.32hex
	sed -i "s/\.bin/\.bin\.32hex/g" ${OUT_DIR}/${APP}.mm_init.32hex
	sed -i "s+`pwd`/${OUT_DIR}/data/+\./+g" ${OUT_DIR}/${APP}.mm_init.bin

allc: clean all

#-------------------------------------------------------------------------------
# Clean-up
#-------------------------------------------------------------------------------
# .phony: always build these targets (no "is up to date" message)
.PHONY: clean clean_hw
clean: clean_hw
clean_hw:
	@echo "\n\tCleaning up Hardware workspace..."
	make -C ${LIB_DIR} clean
	rm -rf bin/

#-------------------------------------------------------------------------------
# eof
