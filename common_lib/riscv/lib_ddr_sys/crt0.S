/* Copyright (c) 2017  SiFive Inc. All rights reserved.
 * Copyright (c) 2019  ETH Zürich and University of Bologna
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the FreeBSD License.   This program is distributed in the hope that
 * it will be useful, but WITHOUT ANY WARRANTY expressed or implied,
 * including the implied warranties of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  A copy of this license is available at
 * http://www.opensource.org/licenses.
 */
/* Make sure the vector table gets linked into the binary.  */
.global vector_table

/* Entry point for bare metal programs */
.section .text.start
.global _start
.type _start, @function

_start:

/* Enable UART Tx Channel (set control reg) */
.equ    UART_CONTROL_ADDR, 0xffffffb8
li      a1,UART_CONTROL_ADDR
.equ    UART_CONTROL_TX_EN,0x28
li      a0,UART_CONTROL_TX_EN
sw      a0, 0(a1)


/* Set GPR Values for Status RISC-V */
.equ    RUNNING_STATUS_ADDR, 0xfffffc58
.equ    EXITCODE_STATUS_ADDR, 0xfffffc5c

li 	a1, RUNNING_STATUS_ADDR
li  a0, 1
sw  a0, 0(a1)

li 	a1, EXITCODE_STATUS_ADDR
li  a0, 0
sw  a0, 0(a1)

/* lui	x0,0x0    - unneeded (constant) */
lui	x1,0x0
lui	x2,0x0
lui	x3,0x0
lui	x4,0x0
lui	x5,0x0
lui	x6,0x0
lui	x7,0x0
lui	x8,0x0
lui	x9,0x0
lui	x10,0x0
lui	x11,0x0
lui	x12,0x0
lui	x13,0x0
lui	x14,0x0
lui	x15,0x0
lui	x16,0x0
lui	x17,0x0
lui	x18,0x0
lui	x19,0x0
lui	x20,0x0
lui	x21,0x0
lui	x22,0x0
lui	x23,0x0
lui	x24,0x0
lui	x25,0x0
lui	x26,0x0
lui	x27,0x0
lui	x28,0x0
lui	x29,0x0
lui	x30,0x0
lui	x31,0x0

/* initialize global pointer */
.option push
.option norelax
1:	auipc gp, %pcrel_hi(__global_pointer$)
	addi  gp, gp, %pcrel_lo(1b)
.option pop

/* initialize stack pointer */
	la sp, __stack_end

/* set vector table address */
	la a0, __vector_start
	ori a0, a0, 1 /*vector mode = vectored */
	csrw mtvec, a0

/* clear the bss segment */
	la a0, _edata
	la a2, _end
	sub a2, a2, a0
	li a1, 0
	call memset

/* new-style constructors and destructors */
	la a0, __libc_fini_array
	call atexit
	call __libc_init_array

/* call main */
//	lw a0, 0(sp)                    /* a0 = argc */
//	addi a1, sp, __SIZEOF_POINTER__ /* a1 = argv */
//	li a2, 0                        /* a2 = envp = NULL */
// Initialize these variables to 0. Cannot use argc or argv
// since the stack is not initialized
	li a0, 0
	li a1, 0
	li a2, 0

	call main
	tail exit

.size  _start, .-_start

.global _init
.type   _init, @function
.global _fini
.type   _fini, @function
_init:
_fini:
 /* These don't have to do anything since we use init_array/fini_array. Prevent
    missing symbol error */
	ret
.size  _init, .-_init
.size _fini, .-_fini


.global ___dso_handle
___dso_handle:
	.long	0
