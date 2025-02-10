# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 

PREFIX=riscv32-unknown-elf

SRC = crt0.S handlers.S syscalls.c vectors.S
OBJ = crt0.o handlers.o syscalls.o vectors.o
LIBCV-VERIF = libcv-verif.a
CFLAGS ?= -Os -g -static -mabi=ilp32 -march=rv32im -Wall -pedantic

all: vars $(LIBCV-VERIF)

$(LIBCV-VERIF): $(OBJ)
	${RISCV}/bin/${PREFIX}-ar rcs $@ $(OBJ)

%.o : %.c
	${RISCV}/bin/${PREFIX}-gcc $(CFLAGS) -c $< -o $@

%.o : %.S
	${RISCV}/bin/${PREFIX}-gcc $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(LIBCV-VERIF)


vars:
	@echo "make bsp variables:"
	@echo "   LIBCV-VERIF      = $(LIBCV-VERIF)"
	@echo "   RISCV            = $(RISCV)"
	@echo "   PREFIX           = $(PREFIX)"
