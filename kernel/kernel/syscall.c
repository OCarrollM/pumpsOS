#include "syscall.h"
#include "task.h"
#include <kernel/tty.h>
#include <stdio.h>

#define KERNEL_BASE 0xC0000000

typedef int32_t (*syscall_fn_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

static uint32_t sys_exit(uint32_t code, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5) {
    (void)a2;
    (void)a3;
    (void)a4;
    (void)a5;

    printf("exit(%d) from task '%s'\n", (int)code, task_current()->name);

    task_exit();

    return 0;
}

static int32_t sys_write(uint32_t fd, uint32_t buf, uint32_t len, uint32_t a4, uint32_t a5) {
    (void)a4;
    (void)a5;

    if (fd != 1 && fd != 2) {
        return -1;
    }

    if (buf >= KERNEL_BASE || len > KERNEL_BASE - buf) {
        return -1;
    }

    terminal_write((const char*)buf, (size_t)len);
    return (int32_t)len;
}

static syscall_fn_t syscall_table[SYSCALL_MAX] = {
    [SYS_EXIT] = sys_exit,
    [SYS_WRITE] = sys_write,
    // NULL needed
};

void syscall_dispatch(struct registers* regs) {
    uint32_t num = regs->eax;

    if (num >= SYSCALL_MAX || syscall_table[num] == 0) {
        printf("unknown syscall %d\n", (int)num);
        regs->eax = (uint32_t)(-1);
        return;
    }

    uint32_t ret = syscall_table[num](regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);
    regs->eax = (uint32_t)ret;
}

void syscall_init(void) {
    isr_register_handler(128, syscall_dispatch);
    printf("Syscall layer init (int 0x80)\n");
}