#include "gdt.h"
// Need to include our header

// Add TSS later
struct gdt_entry gdt[6];
struct gdt_ptr gp;

// This will load GDT and reload segments
extern void gdt_flush(uint32_t);

/*
 * Set up a single GDT entry.
 * 
 * num:    Which entry in the GDT to set
 * base:   Starting address of the segment
 * limit:  Size of the segment (in units determined by granularity)
 * access: Access byte (defines permissions, ring level, type)
 * gran:   Granularity byte (defines flags and upper limit bits)
 */
static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;
    
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}

/*
 * Next I will initialize the GDT by creating a flat memory model
 * covering the whole 4GB space we assigned 
 */
void gdt_init(void) {
    gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gp.base = (uint32_t)&gdt;

    // Null desc (CPU needs this even if not used)
    gdt_set_gate(0,0,0,0,0);

    // Kernel code
    gdt_set_gate(1,0,0xFFFFFFF,0x9A,0xCF);

    // Kernel data
    gdt_set_gate(2,0,0xFFFFFFF,0x92,0xCF);

    // User Code
    gdt_set_gate(3,0,0xFFFFFFF,0xFA,0xCF);

    // User Data
    gdt_set_gate(4,0,0xFFFFFFF,0xF2,0xCF);

    // TSS, Null for now
    gdt_set_gate(5,0,0,0,0);

    // Load and flush
    gdt_flush((uint32_t)&gp);
}