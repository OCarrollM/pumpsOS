#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include <stdint.h>
#include "../arch/i386/isr.h"

#define SYS_EXIT 0
#define SYS_WRITE 1
#define SYS_FORK 2
#define SYS_EXECVE 3
#define SYSCALL_MAX 8

void syscall_dispatch(struct registers* regs);
void syscall_init(void);

#endif