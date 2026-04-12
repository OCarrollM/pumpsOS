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

// Function to purposely overflow the buffer
void __attribute__((noinline)) test_stack_smash(void) {
    char buffer[8];
    memset(buffer, 'A', 64);
}
// Y'Know if it weren't for Matthew Bradburys Pen Test Module (442)
// I'd have no idea what any of this meant.

void kernel_main(uint32_t multiboot_info_phys) {
    terminal_initialize();
    printf("=== Welcome to PumpsOS ===\n\n");

    gdt_init();
    printf("[OK] GDT Initialized\n");
    idt_init();
    printf("[OK] IDT Initialized\n");
    pic_init();
    printf("[OK] PIC Initialized\n");
    timer_init(100);
    printf("[OK] Timer Initialized (100 Hz)\n");
    keyboard_init();
    asm volatile("sti");
    printf("[OK] Interrupts Initialized\n\n");

    /* Convert physical address to virtual */
    multiboot_info_t* mboot = (multiboot_info_t*)(multiboot_info_phys + 0xC0000000);

    if(memory_map_init(mboot)) {
        memory_map_print();
        pmm_init();
        pmm_print_stats();
        vmm_init();
        vmm_print_mappings();
        heap_init();

        printf("Testing kmalloc/kfree...\n");
        char* test1 = kmalloc(100);
        char* test2 = kmalloc(200);
        char* test3 = kmalloc(50);

        printf("    test1 = 0x%x\n", (uint32_t)test1);
        printf("    test2 = 0x%x\n", (uint32_t)test2);
        printf("    test3 = 0x%x\n", (uint32_t)test3);

        memset(test1, 'A', 100);
        memset(test2, 'B', 200);
        memset(test3, 'C', 50);
        printf("    Memory writes OK\n");

        kfree(test2);
        printf("    Freed test2\n");

        char* test4 = kmalloc(150);
        printf("    test4 = 0x%x (should reuse test2's space)\n", (uint32_t)test4);

        kfree(test1);
        kfree(test3);
        kfree(test4);

        heap_print_stats();
    } else {
        printf("Failed to initialize memory map!\n");
    }

    /* Interactive shell */
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