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
#define SYS_READ 5
#define SYS_OPEN 6
#define SYS_CLOSE 7
#define SYS_SLEEP 8 
#define SYS_THREAD_CREATE 9
#define SYS_THREAD_EXIT 10
#define ARG_BUF_SIZE 1024

typedef int32_t (*syscall_fn_t)(struct registers* regs);
static uint32_t next_thread_stack_top = USER_STACK_BASE;

static int32_t sys_thread_exit(struct registers* regs) {
    (void)regs;
    task_exit(0);
    return 0;
}

static int32_t sys_sleep(struct registers* regs) {
    uint32_t ms = regs->ebx;
    task_t* self = task_current();

    uint64_t now = timer_get_ticks(); // get current tick
    uint32_t delay_ticks = (ms * 100) / 1000;
    if (delay_ticks == 0 && ms > 0) delay_ticks = 1;

    self->wake_tick = (uint32_t)(now + delay_ticks);
    self->state = TASK_BLOCKED;

    task_yield(); // blocks
    return 0;
}

static int32_t sys_exit(struct registers* regs) {

    printf("exit(%d) from task '%s'\n", (int)regs->ebx, task_current()->name);

    task_exit((int)regs->ebx);

    return 0;
}

static int32_t sys_wait(struct registers* regs) {
    return task_wait((int*)regs->ebx);
}

static int32_t sys_write(struct registers* regs) {
    uint32_t fd = regs->ebx;
    uint32_t buf = regs->ecx;
    uint32_t len = regs->edx;

    if (fd >= MAX_FDS) return -1;
    if (buf >= KERNEL_BASE || len > KERNEL_BASE - buf) return -1;

    task_t* self = task_current();
    struct file* f = &self->fd_table[fd];
    if (!f->used || !f->node) return -1;

    uint32_t n = vfs_write(f->node, f->offset, len, (uint8_t*)buf);
    f->offset += n;
    return (uint32_t)n;
}

static int32_t sys_fork(struct registers* regs) {
    task_t* child = task_fork(regs);
    return child ? (int32_t)child->id : -1;
}

static int32_t sys_execve(struct registers* regs) {
    uint32_t path_user = regs->ebx;
    uint32_t argv_user = regs->ecx;
    if (path_user >= KERNEL_BASE) return -1;

    // copy path
    char path[128];
    {
        const char* p = (const char*)path_user;
        size_t i = 0;
        while (i < sizeof(path) - 1) {
            char c = p[i];
            path[i++] = c;
            if (c == '\0') break;
        }

        path[sizeof(path) - 1] = '\0';
    }

    // Copy argc into kernel memory before teardown
    int argc = 0;
    char argbuf[ARG_BUF_SIZE];
    uint32_t arg_off[MAX_ARGS];
    uint32_t argbuf_used = 0;

    if (argv_user != 0) {
        if (argv_user >= KERNEL_BASE) return -1;
        char** uargv = (char**)argv_user;

        while (argc < MAX_ARGS && uargv[argc] != 0) {
            const char* ustr = uargv[argc];
            if ((uint32_t)ustr >= KERNEL_BASE) return -1;

            arg_off[argc] = argbuf_used;
            // copy string into argbuf
            size_t k = 0;
            while (argbuf_used < ARG_BUF_SIZE - 1) {
                char c = ustr[k++];
                argbuf[argbuf_used++] = c;
                if (c == '\0') break;
            }
            argc++;
        }
    }

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
        task_exit(-1);
        return -1;
    }

    uint32_t stack_phys = pmm_alloc_page();
    if (stack_phys == 0 || !vmm_map_page(USER_STACK_BASE, stack_phys, PTE_PRESENT | PTE_WRITABLE | PTE_USER)) {
        printf("No user stack, killing task\n");
        task_exit(-1);
        return -1;
    }

    uint32_t sp = USER_STACK_TOP;

    uint32_t arg_uaddr[MAX_ARGS];
    for (int a = argc - 1; a >= 0; a--) {
        const char* s = &argbuf[arg_off[a]];
        size_t slen = 0;
        while (s[slen]) slen++;
        slen++;
        sp -= slen;
        memcpy((void*)sp, s, slen);
        arg_uaddr[a] = sp;
    }

    sp &= ~0x3u;

    sp -= (argc + 1) * sizeof(uint32_t);
    uint32_t argv_array = sp;
    uint32_t* uargv_arr = (uint32_t*)sp;
    for (int a = 0; a < argc; a++) {
        uargv_arr[a] = arg_uaddr[a];
    }
    uargv_arr[argc] = 0;

    sp -= sizeof(uint32_t);
    *(uint32_t*)sp = argv_array;
    sp -= sizeof(uint32_t);
    *(uint32_t*)sp = (uint32_t)argc;

    self->user_entry = entry;
    self->user_stack_top = USER_STACK_TOP;

    // Rewrite the iret
    regs->eip = entry;
    regs->useresp = sp;
    regs->eflags = 0x202;
    regs->cs = 0x1B;
    regs->ss = 0x23;
    regs->ds = 0x23;
    regs->eax = regs->ebx = regs->ecx = regs->edx = 0;
    regs->esi = regs->edi = regs->ebp = 0;

    printf("Task '%s' -> %s (argc=%d, entry=0x%x)\n",
           self->name, path, argc, entry);
    return 0;
}

static int32_t sys_open(struct registers* regs) {
    uint32_t path_user = regs->ebx;

    if (path_user >= KERNEL_BASE) return -1;

    // copy path into kernel buffer
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
    if (!node) return -1;

    int fd = fd_alloc(task_current(), node);
    return fd;
}

static int32_t sys_close(struct registers* regs) {
    uint32_t fd = regs->ebx;
    if (fd >= MAX_FDS) return -1;

    task_t* self = task_current();
    struct file* f = &self->fd_table[fd];
    if (!f->used) return -1;

    f->node = NULL;
    f->offset = 0;
    f->used = false;
    return 0;
}

static int32_t sys_read(struct registers* regs) {
    uint32_t fd = regs->ebx;
    uint32_t buf = regs->ecx;
    uint32_t len = regs->edx;



    if (fd >= MAX_FDS) return -1;
    if (buf >= KERNEL_BASE || len > KERNEL_BASE - buf) return -1;

    task_t* self = task_current();
    struct file* f = &self->fd_table[fd];

    if (!f->used || !f->node) return -1;

    uint32_t n = vfs_read(f->node, f->offset, len, (uint8_t*)buf);
    f->offset += n;
    return (int32_t)n;
}

static int32_t sys_thread_create(struct registers* regs) {
    uint32_t entry = regs->ebx;
    uint32_t arg = regs->ecx;

    task_t* self = task_current();
    tas_t* t = (task_t*)kmalloc(sizeof(task_t));
    if (!t) return -1;
    void* kstack = kmalloc(TASK_STACK_SIZE);
    if (!kstack) { kfree(t); return -1; }

    // choose this threads user stack
    next_thread_stack_top -= 0x10000;
    uint32_t tstack_top = next_thread_stack_top;
    uint32_t tstack_base = tstack_top - PAGE_SIZE;

    // map new stack into the shared addr space
    uint32_t stack_phys = pmm_alloc_page();
    if (stack_phys == 0 || !vmm_map_page(tstack_base, stack_phys, PTE_PRESENT | PTE_WRITABLE | PTE_USER)) {
        if (stack_phys) pmm_free_page(stack_phys);
        kfree(kstack);
        kfree(t);
        return -1;
    }

    // put arg on the new user stack so trampoline can read
    uint32_t user_esp = tstack_top;
    user_esp -= sizeof(uint32_t);
    *(uint32_t*)user_esp = arg;

    user_esp -= sizeof(uint32_t);
    *(uint32_t*)user_esp = 0;

    scheduler_disable_preemption();

    t->id = next_tast_id++;
    strncpy(t->name, "thread", 31);
    t->name[31] = '\0';
    t->state = TASK_READY;
    t->priority = self->priority;
    t->stack_base = (uint32_t)kstack;
    t->stack_top  = (uint32_t)kstack + TASK_STACK_SIZE;
    t->is_user   = true;
    t->is_thread = true;                         
    t->user_entry = entry;
    t->user_stack_top = tstack_top;
    t->page_directory = self->page_directory;     
    t->wake_tick = 0;
    t->parent = NULL;
    t->exit_code = 0;

    fd_table_init(t);

    extern void user_mode_enter(void);
    uint32_t* sp = (uint32_t*)((uint8_t*)kstack + TASK_STACK_SIZE);
    *(--sp) = user_esp;                  
    *(--sp) = entry;                   
    *(--sp) = 0xDEADBEEF;              
    *(--sp) = (uint32_t)user_mode_enter;
    *(--sp) = 0; *(--sp) = 0;            
    *(--sp) = 0; *(--sp) = 0;         
    t->esp = (uint32_t)sp;

    t->next = current_task->next;
    current_task->next = t;

    scheduler_enable_preemption();

    return (int32_t)t->id;
}

static syscall_fn_t syscall_table[SYSCALL_MAX] = {
    [SYS_EXIT] = sys_exit,
    [SYS_WRITE] = sys_write,
    [SYS_FORK] = sys_fork,
    [SYS_EXECVE] = sys_execve,
    [SYS_WAIT] = sys_wait,
    [SYS_READ] = sys_read,
    [SYS_OPEN] = sys_open,
    [SYS_CLOSE] = sys_close,
    [SYS_SLEEP] = sys_sleep,
    [SYS_THREAD_CREATE] = sys_thread_create,
    [SYS_THREAD_EXIT] = sys_thread_exit,
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

