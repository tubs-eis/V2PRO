

PULP=0

ifeq (${PULP}, 0)
RISCV=/opt/riscv-gcc-pulp-toolchain/corev-openhw-gcc-ubuntu2004-20211104
PREFIX=riscv32-corev-elf
else
RISCV=/opt/riscv-gcc-pulp-toolchain/pulp-gcc-ubuntu2004-20200913
PREFIX=riscv32-unknown-elf
endif

LIB_DIR=./lib_bram_eisv

FLAGS= -O3 -g -static -mabi=ilp32 -march=rv32imc -Wall -pedantic
FLAGS+= -nostartfiles
FLAGS+= -T ${LIB_DIR}/link.ld -L ${LIB_DIR} -lcv-verif
FLAGS+= -L ${RISCV}/${PREFIX}/include/


SOURCES=fibonacci.c

ELFFILES=$(patsubst %.c, %.elf, $(SOURCES))
HEXFILES=$(patsubst %.c, %.hex, $(SOURCES))
OBJDUMPFILES=$(patsubst %.c, %.objdump, $(SOURCES))
READELFFILES=$(patsubst %.c, %.readelf, $(SOURCES))
BINFILES=$(patsubst %.c, %.bin, $(SOURCES))

.PHONY: all lib
all: lib ${HEXFILES} ${OBJDUMPFILES} ${READELFFILES} ${ELFFILES} ${BINFILES}

lib:
	make -C ${LIB_DIR} CV_SW_TOOLCHAIN=${RISCV} RISCV_EXE_PREFIX=${RISCV}/bin/${PREFIX}-

%.elf: %.c
	${RISCV}/bin/${PREFIX}-g++ ${FLAGS} $< -o $@

#ELFFILES=$(patsubst %.c, %.elf, $(SOURCES))
#HEXFILES=$(patsubst %.elf, %.hex, $(ELFFILES))

%.hex: %.bin
	xxd -g 4 $< | cut -d " " -f 2-5 > $@
#   -g 4 byte groups (32-bit) for imem word size
#   -e little endian byte order
#   cut removes address and ascii tranlsation
#
#	${RISCV}/bin/${PREFIX}-objcopy -O verilog $< $@
# [Info] dump of binary file in hex format to be loaded in simulation
#        .tcl Command: mem load -infile /.../fibonacci.hex -format hex -truncate /tb/design_1_i/InstructionRAM/U0/ram_inst/ram
# [Note] objcopy not used, due to address jumps (@...). Adress is byte-wise but interpreted element-wise by questa

%.readelf: %.elf
	${RISCV}/bin/${PREFIX}-readelf --sym-base=16 -a $< > $@

%.objdump: %.elf
	${RISCV}/bin/${PREFIX}-objdump -d -M no-aliases -M numeric -S $< > $@

%.bin: %.elf
	${RISCV}/bin/${PREFIX}-objcopy -O binary -R .vproimage -R .nobss $< $@
	# swap endianness
	objcopy -I binary -O binary --reverse-bytes=4 $@ $@
	${RISCV}/bin/${PREFIX}-size $<

clean:
	rm -f *.hex
	rm -f *.bin
	rm -f *.elf
	rm -f *.readelf
	rm -f *.objdump
	make -C ${LIB_DIR} clean
