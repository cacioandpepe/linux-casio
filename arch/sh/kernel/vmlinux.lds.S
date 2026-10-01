/* SPDX-License-Identifier: GPL-2.0 */
/*
 * SuperH Linux kernel linker script
 *
 * fx-9750GIII:
 *
 * P1 RAM:
 *   zero page
 *   head_32
 *   VBR / exception entry
 *   TLB miss walker
 *   init text/data
 *   writable data
 *   BSS
 *
 * P3 translated ROM:
 *   normal kernel text
 *   rodata
 *   exception tables
 */

OUTPUT_ARCH(sh)

#define RUNTIME_DISCARD_EXIT

#include <asm/thread_info.h>
#include <asm/cache.h>
#include <asm/vmlinux.lds.h>

#ifdef CONFIG_PMB
#define MEMORY_OFFSET 0
#else
#define MEMORY_OFFSET __MEMORY_START
#endif

#define FX9750_RAM_BASE \
	(PAGE_OFFSET + MEMORY_OFFSET + PHYSICAL_OFFSET)

#define FX9750_ROM_BASE 0xc0000000

ENTRY(_start)

SECTIONS
{
#ifdef CONFIG_SH_FX9750GIII

	/* ---------------------------------------------------------
	 * P1 RAM: bootstrap
	 * --------------------------------------------------------- */

	. = FX9750_RAM_BASE + CONFIG_ZERO_PAGE_OFFSET;

	__sh7305_ram_start = .;

	.empty_zero_page : AT(ADDR(.empty_zero_page)) {
		*(.empty_zero_page)
	} = 0

	. = ALIGN(PAGE_SIZE);

	.head.text : AT(ADDR(.head.text)) {
		HEAD_TEXT
	} = 0x0009

	. = ALIGN(PAGE_SIZE);

	.sh7305.entry.text : AT(ADDR(.sh7305.entry.text)) {
		__sh7305_entry_start = .;

		KEEP(*(.sh7305.entry.text))
		KEEP(*(.sh7305.tlb.text))
		KEEP(*(.sh7305.tlbrefill.text))
		KEEP(*(.sh7305.irqflags.text))
		KEEP(*(.sh7305.uncached.text))

		__sh7305_entry_end = .;
	} = 0x0009

	. = ALIGN(PAGE_SIZE);

	__sh7305_ram_after_entry = .;


	/* ---------------------------------------------------------
	 * P3: flash-backed mapped kernel
	 * --------------------------------------------------------- */

	. = FX9750_ROM_BASE;

	__sh7305_rom_start = .;
	_text = .;

	.text : AT(ADDR(.text)) {
		TEXT_TEXT
		SCHED_TEXT
		LOCK_TEXT
		KPROBES_TEXT
		IRQENTRY_TEXT
		SOFTIRQENTRY_TEXT

		*(.fixup)
		*(.gnu.warning)

		_etext = .;
	} = 0x0009

	EXCEPTION_TABLE(16)

	RO_DATA(PAGE_SIZE)

	DWARF_EH_FRAME

	. = ALIGN(PAGE_SIZE);

	__sh7305_rom_end = .;
	__sh7305_rom_size =
		__sh7305_rom_end - __sh7305_rom_start;

	ASSERT(__sh7305_rom_size <= 0x00280000,
	       "fx9750: P3 ROM exceeds the persistent 1 KiB map")


	/* ---------------------------------------------------------
	 * Return to P1 RAM.
	 *
	 * Explicit-address output section allows us to resume the
	 * low P1 VMA after linking the high P3 ROM region.
	 * --------------------------------------------------------- */

	.sh7305.ram.anchor __sh7305_ram_after_entry (NOLOAD) : {
		BYTE(0)
	}

	. = ALIGN(PAGE_SIZE);

	__init_begin = .;

	INIT_TEXT_SECTION(PAGE_SIZE)
	INIT_DATA_SECTION(16)

	. = ALIGN(4);

	.machvec.init : AT(ADDR(.machvec.init)) {
		__machvec_start = .;
		*(.machvec.init)
		__machvec_end = .;
	}

	PERCPU_SECTION(L1_CACHE_BYTES)

	.exit.text : AT(ADDR(.exit.text)) {
		EXIT_TEXT
	}

	.exit.data : AT(ADDR(.exit.data)) {
		EXIT_DATA
	}

	. = ALIGN(PAGE_SIZE);
	__init_end = .;


	/* ---------------------------------------------------------
	 * Permanent writable P1 data
	 * --------------------------------------------------------- */

	_sdata = .;

	RW_DATA(L1_CACHE_BYTES, PAGE_SIZE, THREAD_SIZE)

	_edata = .;

	. = ALIGN(PAGE_SIZE);

	BSS_SECTION(0, PAGE_SIZE, 4)

	_end = .;

	__sh7305_ram_end = .;

	/* The loader needs a page-padded RAM image below persistent ROM repairs. */
	ASSERT(__sh7305_ram_end <= 0x88056000,
	       "fx9750: P1 RAM image overlaps loader RAM sources")

#else

	/* Standard SuperH layout */

	. = PAGE_OFFSET + MEMORY_OFFSET +
	    PHYSICAL_OFFSET + CONFIG_ZERO_PAGE_OFFSET;

	_text = .;

	.empty_zero_page : AT(ADDR(.empty_zero_page)) {
		*(.empty_zero_page)
	} = 0

	.text : AT(ADDR(.text)) {
		HEAD_TEXT
		TEXT_TEXT
		SCHED_TEXT
		LOCK_TEXT
		KPROBES_TEXT
		IRQENTRY_TEXT
		SOFTIRQENTRY_TEXT
		*(.fixup)
		*(.gnu.warning)
		_etext = .;
	} = 0x0009

	EXCEPTION_TABLE(16)

	_sdata = .;

	RO_DATA(PAGE_SIZE)
	RW_DATA(L1_CACHE_BYTES, PAGE_SIZE, THREAD_SIZE)

	_edata = .;

	DWARF_EH_FRAME

	. = ALIGN(PAGE_SIZE);

	__init_begin = .;

	INIT_TEXT_SECTION(PAGE_SIZE)
	INIT_DATA_SECTION(16)

	. = ALIGN(4);

	.machvec.init : AT(ADDR(.machvec.init)) {
		__machvec_start = .;
		*(.machvec.init)
		__machvec_end = .;
	}

	PERCPU_SECTION(L1_CACHE_BYTES)

	.exit.text : AT(ADDR(.exit.text)) {
		EXIT_TEXT
	}

	.exit.data : AT(ADDR(.exit.data)) {
		EXIT_DATA
	}

	. = ALIGN(PAGE_SIZE);
	__init_end = .;
	__bss_start = .;
	SBSS(0)
	. = ALIGN(PAGE_SIZE);
	.bss : AT(ADDR(.bss) - LOAD_OFFSET) {
		BSS_FIRST_SECTIONS
		. = ALIGN(PAGE_SIZE);
		*(.bss..page_aligned)
		. = ALIGN(PAGE_SIZE);
		*(.dynbss)
		*(BSS_MAIN)
		*(COMMON)
		. = ALIGN(8);
	}
	__bss_stop = .;
	_end = . ;

#endif

	STABS_DEBUG
	DWARF_DEBUG
	MODINFO
	ELF_DETAILS

	DISCARDS
}
