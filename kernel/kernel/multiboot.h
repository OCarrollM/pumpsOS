#ifndef KERNEL_MULTIBOOT_H
#define KERNEL_MULTIBOOT_H

#include <stdint.h>

#define MULTIBOOT_HEADER_MAGIC 0x1BADB002
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

#define MULTIBOOT_FLAG_ALIGN (1 << 0)
#define MULTIBOOT_FLAG_MEMINFO (1 << 1)
#define MULTIBOOT_FLAG_VIDEOMODE (1 << 2)

#define MULTIBBOOT_INFO_MEMORY (1 << 0)     /* mem_lower, mem_upper */
#define MULTIBBOOT_INFO_BOOTDEV (1 << 1)    /* boot_device */
#define MULTIBBOOT_INFO_CMDLINE (1 << 2)    /* cmdline */
#define MULTIBBOOT_INFO_MODS (1 << 3)       /* mods_count, mods_addr */
#define MULTIBBOOT_INFO_AOUT_SYMS (1 << 4)  /* a.out symbol table */
#define MULTIBBOOT_INFO_ELF_SHDR (1 << 5)   /* ELF section header*/
#define MULTIBBOOT_INFO_MEM_MAP (1 << 6)    /* mmap_length, mmap_addr */
#define MULTIBBOOT_INFO_DRIVE_INFO (1 << 7) /* drives_length, drives_addr */
#define MULTIBBOOT_INFO_CONFIG_TABLE (1 << 8) /* config_table */
#define MULTIBBOOT_INFO_BOOT_LOADER (1 << 9)    /* boot_loader_name */
#define MULTIBBOOT_INFO_APM_TABLE (1 << 10) /* apm_table */
#define MULTIBBOOT_INFO_VBE_INFO (1 << 11)  /* VBE Info */
#define MULTIBBOOT_INFO_FRAMEBUFFER (1 << 12)   /* Framebuffer */

#define MULTIBOOT_MEMORY_AVAILABLE 1        /* Usable RAM */
#define MULTIBOOT_MEMORY_RESERVED 2         /* Reserved, cannot use this */
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE 3 /* ACPI Tables */
#define MULTIBOOT_MEMORY_NVS 4              /* ACPI non-volatile storage */
#define MULTIBOOT_MEMORY_BADRAM 5           /* Defective RAM */

/* Info structs */

typedef struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;

    union {
        struct {
            uint32_t tabsize;
            uint32_t strsize;
            uint32_t addr;
            uint32_t reserved;
        } aout_sym;
        struct {
            uint32_t num;
            uint32_t size;
            uint32_t addr;
            uint32_t shndx;
        } elf_sec;
    } u;

    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;

    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;

    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t color_info[6];
} __attribute__((packed)) multiboot_info_t;

typedef struct multiboot_mmap_entry {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed)) multiboot_mmap_entry_t;

/* Alt struct using 32 bit halves for older code */
typedef struct multiboot_mmap_entry_split {
    uint32_t size;
    uint32_t addr_low;
    uint32_t addr_high;
    uint32_t len_low;
    uint32_t len_high;
    uint32_t type;
} __attribute__((packed)) multiboot_mmap_entry_split_t;

typedef struct multiboot_module {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t cmdline;
    uint32_t reserved;
} __attribute__((packed)) multiboot_module_t;

#endif