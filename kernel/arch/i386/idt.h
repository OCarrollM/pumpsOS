#ifndef ARCH_I386_IDT_H
#define ARCH_I386_IDT_H

#include <stdint.h>

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

#define IDT_ENTRIES 256

#define IDT_INTERRUPT_GATE 0x8E
#define IDT_TRAP_GATE 0x8F

void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t offset, uint16_t selector, uint8_t type_attr);

#endif

// TODO: Readup and comment