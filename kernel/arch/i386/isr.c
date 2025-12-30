#include "isr.h"
#include <stdio.h>
#include <kernel/tty.h>

static isr_handler_t isr_handlers[256] = {0};

static const char* exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

void isr_register_handler(uint8_t num, isr_handler_t handler) {
    isr_handlers[num] = handler;
}

void isr_handler(struct registers* regs) {
    if(isr_handlers[regs->int_no] != 0) {
        isr_handlers[regs->int_no](regs);
        return;
    }

    if (regs->int_no < 32) {
        terminal_setcolor(0x04);
        printf("\n =-=-= KERNEL PANIC MODE =-=-=\n");
        printf("Exception: %s\n", exception_messages[regs->int_no]);
        printf("Error Code: 0x%x\n", regs->err_code);
        printf("EIP: 0x%x\n", regs->eip);
        printf("CS: 0x%x\n", regs->cs);
        printf("EFLAGS: 0x%x\n", regs->eflags);

        // Halt CPU
        asm volatile("cli; hlt");
    }
}

// TODO