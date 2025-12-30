#include "idt.h"
#include "gdt.h"
#include "isr.h"

struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;

extern void idt_flush(uint32_t);

void idt_set_gate(uint8_t num, uint32_t offset, uint16_t selector, uint8_t type_attr) {
    idt[num].offset_low = offset & 0xFFFF;
    idt[num].offset_high = (offset >> 16) & 0xFFFF;
    idt[num].selector = selector;
    idt[num].zero = 0;
    idt[num].type_attr = type_attr;
}

void idt_init(void) {
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base = (uint32_t)&idt;

    for(int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i,0,0,0);
    }

    // 31 exception handlers
    // TODO undertstand every single one
    idt_set_gate(0, (uint32_t)isr0, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(1, (uint32_t)isr1, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(2, (uint32_t)isr2, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(3, (uint32_t)isr3, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(4, (uint32_t)isr4, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(5, (uint32_t)isr5, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(6, (uint32_t)isr6, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(7, (uint32_t)isr7, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(8, (uint32_t)isr8, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(9, (uint32_t)isr9, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(10, (uint32_t)isr10, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(11, (uint32_t)isr11, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(12, (uint32_t)isr12, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(13, (uint32_t)isr13, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(14, (uint32_t)isr14, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(15, (uint32_t)isr15, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(16, (uint32_t)isr16, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(17, (uint32_t)isr17, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(18, (uint32_t)isr18, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(19, (uint32_t)isr19, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(20, (uint32_t)isr20, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(21, (uint32_t)isr21, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(22, (uint32_t)isr22, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(23, (uint32_t)isr23, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(24, (uint32_t)isr24, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(25, (uint32_t)isr25, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(26, (uint32_t)isr26, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(27, (uint32_t)isr27, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(28, (uint32_t)isr28, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(29, (uint32_t)isr29, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(30, (uint32_t)isr30, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    idt_set_gate(31, (uint32_t)isr31, GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);

    idt_flush((uint32_t)&idtp);
}