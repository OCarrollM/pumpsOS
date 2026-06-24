#ifndef KERNEL_TASK_H
#define KERNEL_TASK_H

#include <stdint.h>
#include <stdbool.h>
#include "../arch/i386/isr.h"

#define PRIORITY_IDLE   0
#define PRIORITY_LOW    1
#define PRIORITY_NORMAL 5
#define PRIORITY_HIGH   8
#define PRIORITY_RT     10

typedef enum {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_SLEEPING,
    TASK_TERMINATED
} task_state_t;

typedef struct task {
    uint32_t id; // Unique id
    char name[32]; // Name
    task_state_t state; // Current state
    uint32_t priority;
    uint32_t esp; // Saved pointer (for context switching)
    uint32_t stack_base; // Base of stack
    uint32_t stack_top; // Top of kernel stack
    bool is_user; // only if in user mode
    uint32_t user_entry; // user-mode entry point 
    uint32_t user_stack_top; // top of user stack
    uint32_t page_directory; // Physical addr of this tasks page directory
    uint32_t wake_tick; // If sleep, count to wake
    struct task* next; // Linked list of tasks
    struct task* parent; // Forking parent, NULL if none
    int exit_code; // Exit
} task_t;

void scheduler_init(void); // Initialize the scheduler
task_t* task_create(const char* name, void (*entry)(void), uint32_t priority); // Create a new kernel
void task_yield(void); // Give up the CPU
void task_set_priority(task_t* task, uint32_t priority);
task_t* task_current(void); // Get current task
void scheduler_tick(void);
void scheduler_enable_preemption(void);
void scheduler_disable_preemption(void);
void scheduler_print_tasks(void); // Debug
task_t* task_create_user(const char* name, const void* user_payload, uint32_t payload_size, uint32_t priority);
task_t* task_create_user_elf(const char* name, const char* path, uint32_t priority);
task_t* task_fork(struct registers* parent_regs);
void task_exit(int code);
int32_t task_wait(int* status_user);

#endif