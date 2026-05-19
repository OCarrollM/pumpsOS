#include "tss.h"
#include "gdt.h"
#include <string.h>

static tss_entry_t tss;
extern struct gdt_entry gdt[];
extern void tss_flush(void);

static void install_tss_descriptor(uint32_t slot, uint32_t base, uint32_t limit) {
    gdt[slot].base_low = (base & 0xFFFF);
    gdt[slot].base_middle = (base >> 16) & 0xFF;
    gdt[slot].base_high = (base >> 24) & 0xFF;

    gdt[slot].limit_low = (limit & 0xFFFF);
    gdt[slot].granularity = (limit >> 16) & 0x0F;
    gdt[slot].access = 0xE9;
}

void tss_install(uint32_t gdt_slot, uint32_t kernel_stack_top) {
    uint32_t base = (uint32_t)&tss;
    uint32_t limit = sizeof(tss) - 1;

    install_tss_descriptor(gdt_slot, base, limit);

    memset(&tss, 0, sizeof(tss));

    tss.ss0 = 0x10;
    tss.esp0 = kernel_stack_top;
    tss.iomap_base = sizeof(tss);

    tss_flush();
}

void tss_set_kernel_stack(uint32_t esp0) {
    tss.esp0 = esp0;
}