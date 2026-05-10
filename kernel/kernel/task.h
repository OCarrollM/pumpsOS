#ifndef KERNEL_TASK_H
#define KERNEL_TASK_H

#include <stdint.h>
#include <stdbool.h>

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
    uint32_t esp; // Saved pointer (for context switching)
    uint32_t stack_base; // Base of stack
    uint32_t wake_tick; // If sleep, count to wake
    struct task* next; // Linked list of tasks
} task_t;

void scheduler_init(void); // Initialize the scheduler
task_t* task_create(const char* name, void (*entry)(void)); // Create a new kernel
void task_yield(void); // Give up the CPU
void task_exit(void); // Terminate task
task_t* task_current(void); // Get current task
void scheduler_tick(void);
void scheduler_enable_preemption(void);
void scheduler_disable_preemption(void);
void scheduler_print_tasks(void); // Debug

#endif