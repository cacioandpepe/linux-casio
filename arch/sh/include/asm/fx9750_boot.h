#ifndef __ASM_SH_SH7305_BOOT_H
#define __ASM_SH_SH7305_BOOT_H

#define SH7305_BOOT_MAGIC          0x46583937u
#define SH7305_BOOT_VERSION        3

#define FX9750_ROM_VA              0xc0000000UL
#define FX9750_ROM_MAX_SIZE        0x00400000UL

/* Persistent 1 KiB map and read-only RAM repairs for the P3 image. */
#define SH7305_BOOT_1KMAP_PHYS     0x08060000UL
#define SH7305_BOOT_1KMAP_P1       0x88060000UL
#define SH7305_BOOT_1KMAP_SIZE     0x00002800UL
#define SH7305_BOOT_1KMAP_MAX      (SH7305_BOOT_1KMAP_SIZE / 4)

#define SH7305_BOOT_REPAIR_PHYS    0x08068000UL
#define SH7305_BOOT_REPAIR_SIZE    0x00008000UL
#define SH7305_BOOT_FLASH_FIRST    0x00400000UL
#define SH7305_BOOT_FLASH_END      0x00800000UL

#define SH7305_BOOT_PGD_PHYS       0x08065000UL
#define SH7305_BOOT_PTE_PHYS       0x08066000UL
#define FX9750_BOOTINFO_PHYS       0x08067000UL

#define SH7305_BOOT_PGD_P1         0x88065000UL
#define SH7305_BOOT_PTE_P1         0x88066000UL
#define FX9750_BOOTINFO_P1         0x88067000UL

#define SH7305_BOOT_RESERVED_SIZE  0x3000UL

struct fx9750_bootinfo {
	unsigned int magic;
	unsigned int version;

	unsigned int rom_va;
	unsigned int rom_size;
	unsigned int rom_pages;
	unsigned int entry_va;

	unsigned int ram_phys;
	unsigned int ram_size;

	unsigned int rom_1k_blocks;
	unsigned int rom_1k_map_p1;

	unsigned int reserved[6];
};

#ifdef CONFIG_SH_FX9750GIII
void fx9750_update_tlb_1k(unsigned long address, unsigned long phys);
#endif

#endif
