#ifndef ARCH_I386_GDT_H
#define ARCH_I386_GDT_H

#include <stdint.h>

// WIP, shoddy work until i understand how this works

/**
 * Every entry is 8 bytes and is kinda weirdly layed out
 *  
 * [0-1] Limit
 * [2-4] Base
 * [5] Access
 * [6] Flags
 * [7] Base
 */

// Struct for the above
struct gdt_entry {
    uint16_t limit_low;     // Limit bits 0-15
    uint16_t base_low;      // Base bits 0-15
    uint8_t base_middle;    // Base bits 16-23
    uint8_t access;         // Access byte
    uint8_t granularity;    // Flags
    uint8_t base_high;      // Base bits 24-31
} __attribute__((packed));

/**
 * Next is a pointer struct where we pass to the LGDT instruction
 */
struct gdt_ptr {
    uint16_t limit;     // Size of GDT - 1
    uint32_t base;      // Address of first entry
} __attribute__((packed));

/**
 * The access byte has 7 bits
 * 
 * 7: Present
 * 5-6: Ring Level (0 = Kernel, 3 = User)
 * 4: Descriptor type (1 = Code, 0 = System)
 * 3: Executable
 * 2: Direction
 * 1: Read/Write
 * 0: Accessed (For the CPU)
 * 
 * https://wiki.osdev.org/Global_Descriptor_Table
 */

#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_CODE 0x18
#define GDT_USER_DATA 0x20
#define GDT_TSS 0x28

void gdt_init(void);

#endif