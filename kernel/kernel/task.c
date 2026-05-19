#include "task.h"
#include "heap.h"
#include "pmm.h"
#include "vmm.h"
#include "../arch/i386/isr.h"
#include <stdio.h>
#include <string.h>

#define TASK_STACK_SIZE PAGE_SIZE
#define PREEMPT_QUANTUM 10

extern void task_switch(uint32_t* old_esp, uint32_t new_esp);

static task_t* current_task = NULL;
static task_t* task_list = NULL;
static uint32_t next_tast_id = 1;

static volatile bool preemption_enabled = false;
static uint32_t preempt_counter = 0;

/* Find next runnable task in the list */
static task_t* pick_next_task(void) {
    if (task_list == NULL) {
        return NULL;
    }

    task_t* best = NULL;
    uint32_t best_priority = 0;

    task_t* start = current_task ? current_task->next : task_list;
    task_t* candidate = start;

    do {
        if (candidate->state == TASK_READY) {
            if (best == NULL || candidate->priority > best_priority) {
                best = candidate;
                best_priority = candidate->priority;
            }
        }
        candidate = candidate->next;
    } while (candidate != start);

    return best;
}

void task_set_priority(task_t* task, uint32_t priority) {
    if (task) {
        task->priority = priority;
    }
}

void scheduler_enable_preemption(void) {
    preemption_enabled = true;
}

void scheduler_disable_preemption(void) {
    preemption_enabled = false;
}

void scheduler_tick(void) {
    if (!preemption_enabled || current_task == NULL) {
        return;
    }

    if (++preempt_counter < PREEMPT_QUANTUM) {
        return;
    }
    preempt_counter = 0;

    task_t* next = pick_next_task();
    if (next == NULL || next == current_task) {
        return;
    }

    /* Use the cooperative task_switch — it saves callee-saved regs.
       The timer ISR has already saved the caller-saved regs via pusha. */
    task_t* prev = current_task;
    if (prev->state == TASK_RUNNING) {
        prev->state = TASK_READY;
    }
    next->state = TASK_RUNNING;
    current_task = next;

    if (next->page_directory != prev->page_directory) {
        printf("[SCHED] cr3 swap: 0x%x -> 0x%x\n",
            prev->page_directory, next->page_directory);
        vmm_switch_address_space(next->page_directory);
        printf("[SCHED] cr3 swap done\n");
    }

    task_switch(&prev->esp, next->esp);
}

void scheduler_init(void) {
    task_t* main = (task_t*)kmalloc(sizeof(task_t));
    if (!main) {
        printf("FATAL: Could not allocate main task\n");
        return;
    }

    main->id = next_tast_id++;
    strncpy(main->name, "kernel_main", 31);
    main->name[31] = '\0';
    main->state = TASK_RUNNING;
    main->priority = PRIORITY_NORMAL;
    main->page_directory = vmm_get_current_address_space();
    main->esp = 0;
    main->stack_base = 0;
    main->wake_tick = 0;
    main->next = main;

    current_task = main;
    task_list = main;

    //printf("Initialized scheduler, current task: %s (id=%d)\n", main->name, main->id);
}

task_t* task_create(const char* name, void (*entry)(void), uint32_t priority) {
    // Allocate the task struct
    task_t* task = (task_t*)kmalloc(sizeof(task_t));
    if (!task) {
        printf("ERROR: Could not allocate struct\n");
        return NULL;
    }

    void* stack = kmalloc(TASK_STACK_SIZE);
    if (!stack) {
        printf("ERROR: Could not allocate stack\n");
        kfree(task);
        return NULL;
    }

    // Create addr space
    uint32_t pd = vmm_create_address_space();
    if (pd == 0) {
        kfree(stack);
        kfree(task);
        return NULL;
    }

    task->id = next_tast_id++;
    strncpy(task->name, name, 31);
    task->name[31] = '\0';
    task->state = TASK_READY;
    task->priority = priority;
    task->stack_base = (uint32_t)stack;
    task->page_directory = pd;
    task->wake_tick = 0;

    uint32_t* sp = (uint32_t*)((uint8_t*)stack + TASK_STACK_SIZE);

    /* Push in reverse order */
    *(--sp) = (uint32_t)task_exit;
    *(--sp) = (uint32_t)entry;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;

    task->esp = (uint32_t)sp;

    task->next = current_task->next;
    current_task->next = task;

    printf("Created task '%s' (id=%id, stack=0x%x)\n", task->name, task->id, task->priority, task->page_directory);

    return task;
}

void task_yield(void) {
    if (current_task == NULL) {
        return;
    }

    task_t* next = pick_next_task();
    if (next == NULL || next == current_task) {
        return;
    }

    task_t* prev = current_task;
    if (prev->state == TASK_RUNNING) {
        prev->state = TASK_READY;
    }
    next->state = TASK_RUNNING;
    current_task = next;

    // Switch addr space if needed
    if (next->page_directory != prev->page_directory) {
        printf("[SCHED] cr3 swap: 0x%x -> 0x%x\n",
            prev->page_directory, next->page_directory);
        vmm_switch_address_space(next->page_directory);
        printf("[SCHED] cr3 swap done\n");
    }

    task_switch(&prev->esp, next->esp);
}

void task_exit(void) {
    printf("Task '%s' (id=%d) exiting\n", current_task->name, current_task->id);

    current_task->state = TASK_TERMINATED;

    task_yield();

    while(1) {
        asm volatile("hlt");
    }
}

task_t* task_current(void) {
    return current_task;
}

void scheduler_print_tasks(void) {
    printf("\n === TASKS ===\n");
    if (task_list == NULL) {
        printf("    (none)\n");
        return;
    }
    
    task_t* t = task_list;
    do {
        const char* state_str = "?";
        switch (t->state) {
            case TASK_RUNNING:      state_str = "RUNNING";      break;
            case TASK_READY:        state_str = "READY";        break;
            case TASK_BLOCKED:      state_str = "BLOCKED";      break;
            case TASK_SLEEPING:     state_str = "SLEEPING";     break;
            case TASK_TERMINATED:   state_str = "TERMINATED";   break;
        }
        printf("    [%d] %s - %s (priority=%d, esp=0x%x, pd=0x%x)\n", t->id, t->name, state_str, t->priority, t->esp, t->page_directory);
        t = t->next;
    } while (t != task_list);
    printf("\n");
}