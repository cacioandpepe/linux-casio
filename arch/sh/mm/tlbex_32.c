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
/*
 * This entire path runs from P1 RAM before the first XIP instruction is
 * mapped.  Do not call a normal P3 helper or inspect current->mm here.
 * A damaged boot map for a ROM address is a fault, not a generic PTE walk.
 */
static int __attribute__((section(".sh7305.tlb.text")))
fx9750_refill_rom_1k(unsigned long address, unsigned long error_code)
{
	const struct fx9750_bootinfo *bi =
		(const struct fx9750_bootinfo *)FX9750_BOOTINFO_P1;
	const unsigned int *map;
	unsigned long block;
	unsigned int phys;

	if (address < FX9750_ROM_VA ||
	    address >= FX9750_ROM_VA + FX9750_ROM_MAX_SIZE)
		return -1;

	if (bi->magic != SH7305_BOOT_MAGIC ||
	    bi->version != SH7305_BOOT_VERSION ||
	    bi->rom_va != FX9750_ROM_VA ||
	    !bi->rom_size || bi->rom_size > FX9750_ROM_MAX_SIZE ||
	    (bi->rom_size & (PAGE_SIZE - 1)) ||
	    bi->rom_pages != (bi->rom_size >> PAGE_SHIFT) ||
	    bi->rom_1k_blocks != (bi->rom_size >> 10) ||
	    bi->rom_1k_blocks > SH7305_BOOT_1KMAP_MAX ||
	    bi->rom_1k_map_p1 != SH7305_BOOT_1KMAP_P1 ||
	    address - FX9750_ROM_VA >= bi->rom_size)
		return 1;

	/* The P3 backing is immutable, including blocks repaired into RAM. */
	if (error_code)
		return 1;

	block = (address - FX9750_ROM_VA) >> 10;
	map = (const unsigned int *)SH7305_BOOT_1KMAP_P1;
	phys = map[block];

	if ((phys & 0x3ffu) ||
	    !((phys >= SH7305_BOOT_FLASH_FIRST &&
	       phys < SH7305_BOOT_FLASH_END) ||
	      (phys >= SH7305_BOOT_REPAIR_PHYS &&
	       phys < SH7305_BOOT_REPAIR_PHYS + SH7305_BOOT_REPAIR_SIZE) ||
	      (phys >= SH7305_BOOT_REPAIR_EXTRA_PHYS &&
	       phys < SH7305_BOOT_REPAIR_EXTRA_PHYS +
		      SH7305_BOOT_REPAIR_EXTRA_SIZE))) {
		fx9750_lcd_fault(address, phys);
		return 1;
	}

	fx9750_update_tlb_1k(address, phys);
	return 0;
}
#endif

/*
 * Called with interrupts disabled.
 */
#ifdef CONFIG_SH_FX9750GIII
asmlinkage int __attribute__((section(".sh7305.tlb.text")))
#else
asmlinkage int __kprobes
#endif
handle_tlbmiss(struct pt_regs *regs, unsigned long error_code,
	       unsigned long address)
{

	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	pte_t entry;

#ifdef CONFIG_SH_FX9750GIII
	int ret = fx9750_refill_rom_1k(address, error_code);

	if (ret >= 0)
		return ret;
#endif

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
