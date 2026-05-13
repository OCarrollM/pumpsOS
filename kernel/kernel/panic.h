#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H

#include <stdint.h>
#include "../arch/i386/isr.h"

// Trigger kenel panic with custom message
void panic(const char* msg, const char* file, int line);
// Print exception details
void panic_from_exception(struct registers* regs);
// Print all registers
void panic_dump_registers(struct registers* regs);
// Walk the call stack and print the frames
void panic_stack_trace(struct registers* regs);

#define PANIC(msg) panic(msg, __FILE__, __LINE__);

#define KASSERT(cond) \
    do { \
        if (!(cond)) { \
            panic("Assertion failed: " #cond, __FILE__, __LINE__); \
        } \
    } while (0)

#endif