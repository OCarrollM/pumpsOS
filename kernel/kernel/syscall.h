#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include <stdint.h>
#include "../arch/i386/isr.h"

#define SYS_EXIT 0
#define SYS_WRITE 1
#define SYS_FORK 2
#define SYS_EXECVE 3
#define SYS_WAIT 4
#define SYS_READ 5
#define SYS_OPEN 6
#define SYS_CLOSE 7
#define SYS_SLEEP 8
#define SYSCALL_MAX 9

void syscall_dispatch(struct registers* regs);
void syscall_init(void);

#endif