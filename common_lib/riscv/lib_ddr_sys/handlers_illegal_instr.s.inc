#  SPDX-License-Identifier: MIT
#  SPDX-FileCopyrightText: TU Braunschweig, Institut fuer Theoretische Informatik
#  SPDX-FileCopyrightText: 2022, Chair for Chip Design for Embedded Computing, https://www.tu-braunschweig.de/eis
#  SPDX-FileContributor: Sven Gesper <s.gesper@tu-braunschweig.de>

# Handler to generate print of illegal instruction (PC + Instr.data) via UART const addr
#
# #included direct after Label
# handle_illegal_insn:
# ...
#

#include "../eisv_defs.h"

print_time_after_illegal_instr:
    li t2, CNT_RISC_TOTAL_ADDR          /* get time to t1  mepc */
    lw t1, 0(t2)

	addi sp,sp,-4
	sw t1, 0(sp)

    /* print "illegal instr" msg */
    la a0, illegal_insn_msg
    jal ra, puts

    /* print "System Time:" msg */
    la a0, time_msg
    jal ra, puts

	lw t1, 0(sp)
	addi sp,sp,4

    li t4, 0xffffffb0                   /* STDOUT Reg */
    li t0, 32                           /* space */
    sw t0, 0(t4)
    sw t0, 0(t4)
    sw t0, 0(t4)
    sw t0, 0(t4)
    li t0, 48                            /* "0" */
    sw t0, 0(t4)
    li t0, 120                           /* "x" */
    sw t0, 0(t4)

    li t2, 8
    li t3, 0
    li t5, 28
int_print_loop_start0:
    srl a0, t1, t5                      /* get current byte */
    andi a0, a0, 0xf                    /* mask */
    addi t3, t3, 1                      /* loop increment */
    addi t5, t5, -4                     /* shift increment */
    jal ra, puts_byte_to_char           /* convert a0 to char for print */
    sw a0, 0(t4)                        /* print to std out */
    bne t3, t2, int_print_loop_start0    /* continue until all 8 byte are printed */




#
# print header constants
#
    /* message for coming "PC:" */
    la a0, pc_msg
    jal ra, puts

    li t4, 0xffffffb0                   /* STDOUT Reg */
    li t0, 32                           /* space */
    sw t0, 0(t4)
    sw t0, 0(t4)
    sw t0, 0(t4)
    sw t0, 0(t4)
    li t0, 48                            /* "0" */
    sw t0, 0(t4)
    li t0, 120                           /* "x" */
    sw t0, 0(t4)
#
# print data of machine code exception pc
#
print_mepc_after_illegal_instr:
    csrr t1, mepc                       /* get pc to t1  mepc */
    li t2, 8
    li t3, 0
    li t5, 28
int_print_loop_start:
    srl a0, t1, t5                      /* get current byte */
    andi a0, a0, 0xf                    /* mask */
    addi t3, t3, 1                      /* loop increment */
    addi t5, t5, -4                     /* shift increment */
    jal ra, puts_byte_to_char           /* convert a0 to char for print */
    sw a0, 0(t4)                        /* print to std out */
    bne t3, t2, int_print_loop_start    /* continue until all 8 byte are printed */

    la a0, instr_msg                    /* message for coming "PC:" */
    jal ra, puts

    li t4, 0xffffffb0                   /* STDOUT Reg */
    li t0, 32                           /* space */
    sw t0, 0(t4)
    sw t0, 0(t4)
    sw t0, 0(t4)
    sw t0, 0(t4)
    li t0, 48                            /* "0" */
    sw t0, 0(t4)
    li t0, 120                           /* "x" */
    sw t0, 0(t4)

#
# print data of instruction causing the exception
#
print_illegal_instr:
    csrr t0, mepc
    lw t1, 0(t0)
    li t2, 8
    li t3, 0
    li t5, 28
int_print_loop_start2:
    srl a0, t1, t5                      /* get current byte */
    andi a0, a0, 0xf                    /* mask */
    addi t3, t3, 1                      /* loop increment */
    addi t5, t5, -4                     /* shift increment */
    jal ra, puts_byte_to_char           /* convert a0 to char for print */
    sw a0, 0(t4)                        /* print to std out */
    bne t3, t2, int_print_loop_start2   /* continue until all 8 byte are printed */

    li t0, 0x0a                         /* new line */
    sw t0, 0(t4)

	j end_handler_incr_mepc

#
# Helper function (hex for easy conversion)
# convert 4-bit number (in a0) to char (in a0)
#
puts_byte_to_char:                      /* input: a0    */
    li a1, 10
    bge a0, a1, puts_byte_to_char_gt    /* char no number ? */
    addi a0, a0, 0x30                   /* "0" = 0x30 */
    jalr ra
puts_byte_to_char_gt:
    addi a0, a0, 0x37                   /* "A" = 0x41 , "a" = 0x61*/
    jalr ra

#
# after print, restore (increment mepc as required by RISC-V Spec)
# for "C" extension incr. by 2 Byte, else by 4
#
end_handler_incr_mepc:
	csrr t0, mepc   /* get addr of mepc */
	lb t1, 0(t0)    /* the instr. */
	li a0, 0x3      /* mask */
	and t1, t1, a0  /* select lowest 3 bits */
	/* Increment mepc by 2 or 4 depending on whether the instruction at mepc is compressed or not.  */
	/* bne t1, a0, end_handler_incr_mepc2  -> disable of C -> never by 2 -> always by 4 */
	addi t0, t0, 2
end_handler_incr_mepc2:
	addi t0, t0, 2
	csrw mepc, t0   /* write mepc */
    j end_handler_ret



.section .rodata
time_msg:
    .string " AUX Counter (Risc-V Cycles): "
instr_msg:
    .string "\n Illegal Instr is: "
pc_msg:
    .string "\n Illegal Instr PC: "
illegal_insn_msg:
	.string "\n\033[41mIllegal instruction exception handler entered\033[0m"

.section .text.handlers
