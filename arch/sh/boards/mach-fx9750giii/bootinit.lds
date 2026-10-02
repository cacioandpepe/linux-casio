/* SPDX-License-Identifier: GPL-2.0 */
OUTPUT_ARCH(sh)
ENTRY(_start)
PHDRS { code PT_LOAD FILEHDR PHDRS FLAGS(5); }
SECTIONS {
    . = 0x10000 + SIZEOF_HEADERS;
    .text : { *(.text*) *(.rodata*) } :code
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
