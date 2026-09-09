#ifndef __ASM_SH_FX9750_BOOT_H
#define __ASM_SH_FX9750_BOOT_H

#define FX9750_BOOT_MAGIC        0x46583937u  /* "FX97" */
#define FX9750_BOOT_VERSION      1

#define FX9750_ROM_VA            0xc0000000UL
#define FX9750_ROM_MAX_SIZE      0x00400000UL

#define FX9750_BOOT_PGD_PHYS     0x08045000UL
#define FX9750_BOOT_PTE_PHYS     0x08046000UL
#define FX9750_BOOTINFO_PHYS     0x08047000UL

#define FX9750_BOOT_PGD_P1       0x88045000UL
#define FX9750_BOOT_PTE_P1       0x88046000UL
#define FX9750_BOOTINFO_P1       0x88047000UL

#define FX9750_BOOT_RESERVED_SIZE 0x3000UL

struct fx9750_bootinfo {
    unsigned int magic;
    unsigned int version;

    unsigned int rom_va;
    unsigned int rom_size;
    unsigned int rom_pages;

    unsigned int entry_va;

    unsigned int ram_phys;
    unsigned int ram_size;

    unsigned int reserved[8];
};

#endif
