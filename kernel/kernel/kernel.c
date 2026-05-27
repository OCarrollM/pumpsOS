#include <stdio.h>
#include <string.h>

#include <kernel/tty.h>
#include "../arch/i386/gdt.h"
#include "../arch/i386/idt.h"
#include "../arch/i386/pic.h"
#include "../arch/i386/timer.h"
#include "../arch/i386/keyboard.h"
#include "../arch/i386/tss.h"
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
#include "../kernel/syscall.h"
#include "../kernel/elf.h"


// static void task_a(void) {
//     for (int i = 0; i < 5; i++) {
//         printf("[A] iteration %d\n", i);
//         for (volatile int j = 0; j < 10000000; j++) { }
//     }
// }

// static void task_b(void) {
//     for (int i = 0; i < 5; i++) {
//         printf("[B] iteration %d\n", i);
//         for (volatile int j = 0; j < 10000000; j++) { }
//     }
// }

void kernel_main(uint32_t multiboot_info_phys) {
    terminal_initialize();
    printf("=== Welcome to PumpsOS ===\n\n");

    gdt_init();
    
    uint32_t esp_now;
    asm volatile("movl %%esp, %0" : "=r"(esp_now));
    tss_install(5, esp_now);
    //printf("[OK] GDT Initialized\n");
    idt_init();

    syscall_init();
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
        vfs_node_t* hello2 = vfs_lookup("/readme.txt");
        if (hello) {
            char buf[256];
            uint32_t n = vfs_read(hello, 0, sizeof(buf) - 1, (uint8_t*)buf);
            buf[n] = '\0';
            printf("\nContents of hello.txt (%d bytes):\n%s\n", n, buf);
        } if (hello2) {
            char buf2[256];
            uint32_t n = vfs_read(hello2, 0, sizeof(buf2) - 1, (uint8_t*)buf2);
            buf2[n] = '\0';
            printf("\nContents of readme.txt (%d bytes):\n%s\n", n, buf2);
        } else {
            printf("hello.txt not found");
        }
    } else {
        printf("No initrd loaded\n");
    }

    scheduler_init();

    static const uint8_t user_payload[] = {
        0xB8, 0x01, 0x00, 0x00, 0x00, 0xBB, 0x01, 0x00, 0x00, 0x00, 0xB9, 0x24,
        0x00, 0x40, 0x00, 0xBA, 0x12, 0x00, 0x00, 0x00, 0xCD, 0x80, 0xB8, 0x00,
        0x00, 0x00, 0x00, 0xBB, 0x00, 0x00, 0x00, 0x00, 0xCD, 0x80, 0xEB, 0xFE,
        0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x66, 0x72, 0x6F, 0x6D, 0x20, 0x72,
        0x69, 0x6E, 0x67, 0x20, 0x33, 0x0A,
    };

    task_create_user("user_test", user_payload, sizeof(user_payload), PRIORITY_NORMAL);

    //task_create("task_a", task_a, PRIORITY_NORMAL);
    //task_create("task_b", task_b, PRIORITY_HIGH);
    scheduler_enable_preemption();
    //printf("Scheduler Running\n\n");

    debugger_init();

    //printf("Before breakpoint\n");
    BREAKPOINT();
    //printf("After breakpoint\n");

    vfs_node_t* n = vfs_lookup("/readme.txt");
    if (n) {
        Elf32_Ehdr hdr;
        vfs_read(n, 0, sizeof(hdr), (uint8_t*)&hdr);
        printf("validate readme.txt: %d (expect 0)\n", elf_validate(&hdr));
    }

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