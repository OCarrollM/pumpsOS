#include "task.h"
#include "heap.h"
#include "pmm.h"
#include "vmm.h"
#include <stdio.h>
#include <string.h>

#define TASK_STACK_SIZE PAGE_SIZE

extern void task_switch(uint32_t* old_esp, uint32_t new_esp);

static task_t* current_task = NULL;
static task_t* task_list = NULL;
static uint32_t next_tast_id = 1;

/* Find next runnable task in the list */
static task_t* pick_next_task(void) {
    if (task_list == NULL) {
        return NULL;
    }

    task_t* start = current_task ? current_task->next : task_list;
    task_t* candidate = start;

    do {
        if (candidate->state == TASK_READY) {
            return candidate;
        }
        candidate = candidate->next;
    } while (candidate != start);

    return NULL;
}

// static void task_entry_wrapper(void) {

// }

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
    main->esp = 0;
    main->stack_base = 0;
    main->wake_tick = 0;
    main->next = main;

    current_task = main;
    task_list = main;

    printf("Initialized scheduler, current task: %s (id=%d)\n", main->name, main->id);
}

task_t* task_create(const char* name, void (*entry)(void)) {
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

    task->id = next_tast_id++;
    strncpy(task->name, name, 31);
    task->name[31] = '\0';
    task->state = TASK_READY;
    task->stack_base = (uint32_t)stack;
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

    printf("Created task '%s' (id=%id, stack=0x%x)\n", task->name, task->id, task->stack_base);

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
        printf("    [%d] %s - %s (esp=0x%x)\n", t->id, t->name, state_str, t->esp);
        t = t->next;
    } while (t != task_list);
    printf("\n");
}