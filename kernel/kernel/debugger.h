#ifndef KERNEL_DEBUGGER_H
#define KERNEL_DEBUGGER_H

#include <stdint.h>
#include "../arch/i386/isr.h"

#define BREAKPOINT() asm volatile("int $3")

void debugger_enter(struct registers* regs);
void debugger_init(void);

#endif