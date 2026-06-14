#include "syscall.h"
#include "task.h"
#include "elf.h"
#include "vfs.h"
#include "vmm.h"
#include "pmm.h"
#include <kernel/tty.h>
#include <stdio.h>
#include <string.h>

#define KERNEL_BASE 0xC0000000
#define USER_STACK_BASE 0x00500000
#define USER_STACK_TOP (USER_STACK_BASE + 4096)

typedef int32_t (*syscall_fn_t)(struct registers* regs);

static int32_t sys_exit(struct registers* regs) {

    printf("exit(%d) from task '%s'\n", (int)regs->ebx, task_current()->name);

    task_exit();

    return 0;
}

static int32_t sys_write(struct registers* regs) {
    uint32_t fd = regs->ebx;
    uint32_t buf = regs->ecx;
    uint32_t len = regs->edx;

    if (fd != 1 && fd != 2) {
        return -1;
    }

    if (buf >= KERNEL_BASE || len > KERNEL_BASE - buf) {
        return -1;
    }

    terminal_write((const char*)buf, (size_t)len);
    return (int32_t)len;
}

static int32_t sys_fork(struct registers* regs) {
    task_t* child = task_fork(regs);
    return child ? (int32_t)child->id : -1;
}

static int32_t sys_execve(struct registers* regs) {
    uint32_t path_user = regs->ebx;
    if (path_user >= KERNEL_BASE) return -1;

    // copy path
    char path[128];
    const char* p = (const char*)path_user;
    size_t i = 0;
    while (i < sizeof(path) - 1) {
        char c = p[i];
        path[i++] = c;
        if (c == '\0') break;
    }

    path[sizeof(path) - 1] = '\0';

    vfs_node_t* node = vfs_lookup(path);
    if (!node) {
        printf("Not found: %s\n", path);
        return -1;
    }

    task_t* self = task_current();

    // Tear down user half
    uint32_t* pd_virt = (uint32_t*)PAGE_DIR_VIRTUAL;
    for (uint32_t pdi = 0; pdi < 768; pdi++) {
        if (!(pd_virt[pdi] & PTE_PRESENT)) continue;

        uint32_t* pt = (uint32_t*)(PAGE_TABLE_BASE + pdi * PAGE_SIZE);
        for (uint32_t pti = 0; pti < 1024; pti++) {
            if (pt[pti] & PTE_PRESENT) {
                pmm_free_page(pt[pti] & 0xFFFFF000);
                pt[pti] = 0;
            }
        }
        pmm_free_page(pd_virt[pdi] & 0xFFFFF000);
        pd_virt[pdi] = 0;
    }

    // Flush
    vmm_switch_address_space(self->page_directory);

    // Load new ELF into the empty user half
    uint32_t entry = elf_load(node, self->page_directory);
    if (entry == 0) {
        printf("elf_load failed, killing task\n");
        task_exit();
        return -1;
    }

    uint32_t stack_phys = pmm_alloc_page();
    if (stack_phys == 0 || !vmm_map_page(USER_STACK_BASE, stack_phys, PTE_PRESENT | PTE_WRITABLE | PTE_USER)) {
        printf("No user stack, killing task\n");
        task_exit();
        return -1;
    }

    self->user_entry = entry;
    self->user_stack_top = USER_STACK_TOP;

    // Rewrite the iret
    regs->eip = entry;
    regs->useresp = USER_STACK_TOP;
    regs->eflags = 0x202;
    regs->cs = 0x1B;
    regs->ss = 0x23;
    regs->ds = 0x23;
    regs->eax = regs->ebx = regs->ecx = regs->edx = 0;
    regs->esi = regs->edi = regs->ebp = 0;

    printf("Task '%s' -> %s (entry=0x%x)\n", self->name, path, entry);
    return 0;
}

static syscall_fn_t syscall_table[SYSCALL_MAX] = {
    [SYS_EXIT] = sys_exit,
    [SYS_WRITE] = sys_write,
    [SYS_FORK] = sys_fork,
    [SYS_EXECVE] = sys_execve,
};

void syscall_dispatch(struct registers* regs) {
    uint32_t num = regs->eax;

    if (num >= SYSCALL_MAX || syscall_table[num] == 0) {
        printf("unknown syscall %d\n", (int)num);
        regs->eax = (uint32_t)(-1);
        return;
    }
    regs->eax = (uint32_t)syscall_table[num](regs);
}

void syscall_init(void) {
    isr_register_handler(128, syscall_dispatch);
    printf("Syscall layer init (int 0x80)\n");
}