#include "isr.h"
#include "../../kernel/panic.h"
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
    if (isr_handlers[regs->int_no] != 0) {
        isr_handlers[regs->int_no](regs);
        return;
    }
    if (regs->int_no < 32) {
        panic_from_exception(regs);
    } else {
        /* Unhandled hardware IRQ: still EOI, or the PIC blocks it forever. */
        pic_send_eoi(regs->int_no - 32);
    }
}