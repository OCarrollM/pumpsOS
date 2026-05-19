#ifndef ARCH_I386_TSS_H
#define ARCH_I386_TSS_H

#include <stdint.h>

// Task State Management

struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

typedef struct tss_entry tss_entry_t;

void tss_install(uint32_t gdt_slot, uint32_t kernel_stack_top);
void tss_set_kernel_stack(uint32_t esp0);

#endif