#include <stdio.h>
#include <string.h>

#include <kernel/tty.h>
#include "../arch/i386/gdt.h"
#include "../arch/i386/idt.h"
#include "../arch/i386/pic.h"
#include "../arch/i386/timer.h"
#include "../arch/i386/keyboard.h"
#include "../kernel/multiboot.h"
#include "../kernel/memory_map.h"
#include "../kernel/pmm.h"
#include "../kernel/vmm.h"
#include "../kernel/heap.h"
#include "../kernel/task.h"
#include "../kernel/panic.h"
#include "../kernel/debugger.h"
#include "../kernel/vfs.h"
#include "../kernel/initrd.h"


static void task_a(void) {
    for (int i = 0; i < 5; i++) {
        printf("[A] iteration %d\n", i);
        for (volatile int j = 0; j < 10000000; j++) { }
    }
}

static void task_b(void) {
    for (int i = 0; i < 5; i++) {
        printf("[B] iteration %d\n", i);
        for (volatile int j = 0; j < 10000000; j++) { }
    }
}

void kernel_main(uint32_t multiboot_info_phys) {
    terminal_initialize();
    printf("=== Welcome to PumpsOS ===\n\n");

    gdt_init();
    //printf("[OK] GDT Initialized\n");
    idt_init();
    //printf("[OK] IDT Initialized\n");
    pic_init();
    //printf("[OK] PIC Initialized\n");
    timer_init(100);
    //printf("[OK] Timer Initialized (100 Hz)\n");
    keyboard_init();
    asm volatile("sti");
    //printf("[OK] Interrupts Initialized\n\n");

    /* Convert physical address to virtual */
    multiboot_info_t* mboot = (multiboot_info_t*)(multiboot_info_phys + 0xC0000000);

    if (!memory_map_init(mboot)) {
        PANIC("Failed to initialize memory map");
    }

    pmm_init();
    vmm_init();
    heap_init();
    printf("Memory Management Initialized\n");

    vfs_root = initrd_init_from_multiboot(mboot);
    if (vfs_root) {
        printf("Initrd mounted\n");

        // test
        printf("\n--- Contents of / ---\n");
        struct dirent* de;
        int i = 0;
        while ((de = vfs_readdir(vfs_root, i)) != NULL) {
            printf("    %s\n", de->name);
            i++;
        }

        // Test: read
        vfs_node_t* hello = vfs_lookup("/hello.txt");
        if (hello) {
            char buf[256];
            uint32_t n = vfs_read(hello, 0, sizeof(buf) - 1, (uint8_t*)buf);
            buf[n] = '\0';
            printf("\nContents of hello.txt (%d bytes):\n%s\n", n, buf);
        } else {
            printf("hello.txt not found");
        }
    } else {
        printf("No initrd loaded\n");
    }

    scheduler_init();
    //task_create("task_a", task_a, PRIORITY_NORMAL);
    //task_create("task_b", task_b, PRIORITY_HIGH);
    scheduler_enable_preemption();
    //printf("Scheduler Running\n\n");

    debugger_init();

    //printf("Before breakpoint\n");
    BREAKPOINT();
    //printf("After breakpoint\n");
    printf("> ");
    while(1) {
        char c = keyboard_getchar();
        if(c == '\n') {
            printf("\n> ");
        } else if(c == '\b') {
            printf("\b \b");
        } else {
            printf("%c", c);
        }
    }
    // while(1) {
    //     asm volatile("hlt");
    // }
}