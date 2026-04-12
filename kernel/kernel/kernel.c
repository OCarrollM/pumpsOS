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
#include "pmm.h"
#include "vmm.h"

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
    } else {
        printf("Failed to initialize memory meap!\n");
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