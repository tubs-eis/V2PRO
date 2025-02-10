/* MIPS32 linker script */
/* Diploma thesis, Stephan Nolting, Uni Hannover, 2014..2017 */
/* PhD, Sven Gesper, Uni Hannover, 2018..2021 */

MEMORY
{
  mipsram (rwx) : ORIGIN = 0x00000000, LENGTH = 0x10000000 /* changes require stack pointer to change! (below) */
  vproram (rwx) : ORIGIN = 0x10000000, LENGTH = 0x30000000 /* Total: 1GB on ML605 with 1GB sodimm and assigned to subsystem by uemu */
}

SECTIONS
{

    __sp_hi = 0x0FFF; /* last entry of stack = size of RAM in bytes - 4 */
    __sp_lo = 0xFFFC; /* split into high and low part */

    /DISCARD/ : /* sections we do not need */
    {
/*      crt0.elf(.MIPS.abiflags); # these are specific sections
 *      main.elf(.mdebug.abi32);
*/

      /*  if multi elf are linked, use this wildcard format:  */
#      *(.MIPS.abiflags);
#      *(.reginfo);
#      *(.eh_frame);
#      *(.mdebug.abi32);
#      *(.pdr);
#      *(.rel.pdr);
#      *(.note.GNU-stack);
#      *(.comment);	# this causes objdump not to print comments
#      *(.symtab);	# as the symtable ...
#      *(.strtab);
    }










    . = 0x00000000; /* let's begin at zero */
    .image . : SUBALIGN(4) {    /* all elements in here are aligned to 4-byte */
        __image_begin = .;
        __start = .;
        crt0.elf(.text);

        *(.text .text.*);
        *(.near.text .near.text.*);
        *(.rel.text .rel.text.*);

        __data_begin = .; /*  beginning of general data */
        __gp = .;         /*  beginning of general data */
        *(.plt);
        *(.lower.rodata.* .lower.rodata .rodata .rodata.* .gnu-linkonce.r.* .const .const.*);
        *(.lower.data.* .lower.data .data .data.* .gnu.linkonce.d.* .data.rel.ro.local .data.rel.ro* .dynamic);
        *(.near.data .near.data.*);

        # Constructors
        # https://users.informatik.haw-hamburg.de/~krabat/FH-Labor/gnupro/5_GNUPro_Utilities/c_Using_LD/ldLinker_scripts.html#Output_section_discarding
        __CTOR_LIST__ = .;
        LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
        *(.ctors)
        LONG(0)
        __CTOR_END__ = .;
        __DTOR_LIST__ = .;
        LONG((__DTOR_END__ - __DTOR_LIST__) / 4 - 2)
        *(.dtors)
        LONG(0)
        __DTOR_END__ = .;

        #  usefull?
        *(.rodata1);
        *(.gcc_except_table)
        *(.gcc_except_table.*)
        *(.preinit_array)
        *(SORT(.init_array.*))
        *(.init_array)
        *(SORT(.fini_array.*))
        *(.init*);
        *(.fini*);
        *(.fini_array)
        *(.gnu.linkonce.d.*personality*)
        *(.got.plt .got .sdata .sdata.* .gnu.linkonce.s.*)

        __image_end = .;
    } > mipsram


    .eh_frame : {
      *(.eh_frame);
    } > mipsram
#
    .reginfo : {
      *(.reginfo);
    } > mipsram
#
    .MIPS.abiflags : {
      *(.MIPS.abiflags);
    } > mipsram



    .nobss ALIGN(16) : SUBALIGN(16)
    {
      __nobss_start = .;

      *(.nobss .nobss.*);

      __nobss_end = .;
    } > mipsram

    .nobss_32byte_align ALIGN(32) : SUBALIGN(32)
    {
      __nobss_32byte_align_start = .;

      *(.nobss_32byte_align .nobss_32byte_align.*);

      __nobss_32byte_align_end = .;
    } > mipsram




    .bss ALIGN(4) : SUBALIGN(4)
    {
      __bss_start = .;

      *(.bss .bss.*);

      __bss_end = .;
    } > mipsram


    .vproimage . : SUBALIGN(16)
    {
      __mips_vpro_share_start = .;

      *(.host .host.*);
      *(.vpro .vpro.*);

      __mips_vpro_share_end = .;
    } > vproram
}
