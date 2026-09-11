/*
 * TLB miss handler for SH with an MMU.
 *
 *  Copyright (C) 1999  Niibe Yutaka
 *  Copyright (C) 2003 - 2012  Paul Mundt
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/kprobes.h>
#include <linux/kdebug.h>
#include <asm/mmu_context.h>
#include <asm/thread_info.h>
#include <asm/tlb.h>
#include <asm/fx9750_boot.h>

#ifdef CONFIG_SH_FX9750GIII
static int fx9750_refill_rom_1k(unsigned long address);
#endif



#ifdef CONFIG_SH_FX9750GIII
/*
 * Fast refill for the flash-backed P3 kernel image.
 *
 * This path intentionally does not walk the normal Linux page-table
 * hierarchy.  During the earliest P3 miss, normal kernel .text is not
 * mapped yet, so calling into generic helpers there could recursively
 * fault.
 *
 * The loader has already populated the 4 KiB PTE page at 88046000.
 */
static __always_inline int

fx9750_rom_tlbmiss(unsigned long address)
{
	struct fx9750_bootinfo *bi =
		(struct fx9750_bootinfo *)FX9750_BOOTINFO_P1;
	pte_t *ptes = (pte_t *)FX9750_BOOT_PTE_P1;
	unsigned long index;
	pte_t entry;

	/*
	 * The calculator's P3 kernel is backed by four independently-located
	 * 1 KiB flash blocks for every Linux 4 KiB software page.
	 */
	{
		int ret = fx9750_refill_rom_1k(address);

		if (ret >= 0)
			return ret;
	}

	/* -1 means "not our special ROM range". */
	if (bi->magic != FX9750_BOOT_MAGIC ||
	    bi->version != FX9750_BOOT_VERSION)
		return -1;

	if (bi->rom_va != FX9750_ROM_VA ||
	    !bi->rom_size ||
	    bi->rom_size > FX9750_ROM_MAX_SIZE)
		return -1;

	if (address < bi->rom_va ||
	    address >= bi->rom_va + bi->rom_size)
		return -1;

	index = (address - bi->rom_va) >> PAGE_SHIFT;

	if (index >= 1024 || index >= bi->rom_pages)
		return 1;

	entry = ptes[index];

	if (pte_none(entry) || pte_not_present(entry))
		return 1;

	/*
	 * __update_tlb() is forced into P1 RAM on this machine,
	 * so this operation cannot recursively fault into P3.
	 */
	__update_tlb(NULL, address, entry);

	return 0;
}
#endif


#ifdef CONFIG_SH_FX9750GIII

extern void fx9750_update_tlb_1k(unsigned long address,
				 unsigned long phys);

/*
 * Return:
 *   -1 = address is not in our special P3 ROM window
 *    0 = mapping installed
 *    1 = address belongs to ROM but boot mapping is invalid
 */
static int __attribute__((section(".fx9750.tlb.text")))
fx9750_refill_rom_1k(unsigned long address)
{
	struct fx9750_bootinfo *bi =
		(struct fx9750_bootinfo *)FX9750_BOOTINFO_P1;

	volatile unsigned int *map;
	unsigned long block;
	unsigned int phys;

	if (bi->magic != FX9750_BOOT_MAGIC ||
	    bi->version != FX9750_BOOT_VERSION ||
	    bi->rom_va != FX9750_ROM_VA ||
	    !bi->rom_size ||
	    bi->rom_size > FX9750_ROM_MAX_SIZE)
		return -1;

	if (address < bi->rom_va ||
	    address >= bi->rom_va + bi->rom_size)
		return -1;

	if (bi->rom_1k_map_p1 != FX9750_BOOT_1KMAP_P1 ||
	    !bi->rom_1k_blocks ||
	    bi->rom_1k_blocks > FX9750_BOOT_1KMAP_MAX)
		return 1;

	/*
	 * Hardware mapping granularity is 1 KiB even though Linux
	 * continues to use 4 KiB software pages.
	 *
	 * Map only the 1 KiB block which actually faulted.
	 * One exception -> one LDTLB.
	 */
	block = (address - bi->rom_va) >> 10;

	if (block >= bi->rom_1k_blocks)
		return 1;

	map = (volatile unsigned int *)bi->rom_1k_map_p1;
	phys = map[block];

	if (phys == 0xffffffffu ||
	    (phys & 0x3ffu) ||
	    phys >= 0x00800000u)
		return 1;

	fx9750_update_tlb_1k(address, phys);

	return 0;
}
#endif

/*
 * Called with interrupts disabled.
 */
#ifdef CONFIG_SH_FX9750GIII
asmlinkage int __attribute__((section(".fx9750.tlb.text")))
#else
asmlinkage int __kprobes
#endif
handle_tlbmiss(struct pt_regs *regs, unsigned long error_code,
	       unsigned long address)
{

#ifdef CONFIG_SH_FX9750GIII
	/*
	 * FX9750 HANDLE_TLBMISS ENTRY TEST
	 *
	 * Freeze immediately on entering the C TLB handler.
	 * Compiler prologue may execute before this loop.
	 */
	__asm__ __volatile__(
		"1:\n\t"
		"bra 1b\n\t"
		" nop\n\t"
	);
#endif

	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	pte_t entry;

	/*
	 * We don't take page faults for P1, P2, and parts of P4, these
	 * are always mapped, whether it be due to legacy behaviour in
	 * 29-bit mode, or due to PMB configuration in 32-bit mode.
	 */
	if (address >= P3SEG && address < P3_ADDR_MAX) {
		pgd = pgd_offset_k(address);
	} else {
		if (unlikely(address >= TASK_SIZE || !current->mm))
			return 1;

		pgd = pgd_offset(current->mm, address);
	}

	p4d = p4d_offset(pgd, address);
	if (p4d_none_or_clear_bad(p4d))
		return 1;
	pud = pud_offset(p4d, address);
	if (pud_none_or_clear_bad(pud))
		return 1;
	pmd = pmd_offset(pud, address);
	if (pmd_none_or_clear_bad(pmd))
		return 1;
	pte = pte_offset_kernel(pmd, address);
	entry = *pte;
	if (unlikely(pte_none(entry) || pte_not_present(entry)))
		return 1;
	if (unlikely(error_code && !pte_write(entry)))
		return 1;

	if (error_code)
		entry = pte_mkdirty(entry);
	entry = pte_mkyoung(entry);

	set_pte(pte, entry);

#if defined(CONFIG_CPU_SH4) && !defined(CONFIG_SMP)
	/*
	 * SH-4 does not set MMUCR.RC to the corresponding TLB entry in
	 * the case of an initial page write exception, so we need to
	 * flush it in order to avoid potential TLB entry duplication.
	 */
	if (error_code == FAULT_CODE_INITIAL)
		local_flush_tlb_one(get_asid(), address & PAGE_MASK);
#endif

	set_thread_fault_code(error_code);
	update_mmu_cache(NULL, address, pte);

	return 0;
}
