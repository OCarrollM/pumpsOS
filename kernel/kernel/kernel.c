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
#include "../kernel/console.h"

// For ring 3
static const uint8_t user_payload[] = {
    0xB8, 0x01, 0x00, 0x00, 0x00, 0xBB, 0x01, 0x00, 0x00, 0x00, 0xB9, 0x24,
    0x00, 0x40, 0x00, 0xBA, 0x12, 0x00, 0x00, 0x00, 0xCD, 0x80, 0xB8, 0x00,
    0x00, 0x00, 0x00, 0xBB, 0x00, 0x00, 0x00, 0x00, 0xCD, 0x80, 0xEB, 0xFE,
    0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x66, 0x72, 0x6F, 0x6D, 0x20, 0x72,
    0x69, 0x6E, 0x67, 0x20, 0x33, 0x0A,
};

void kernel_main(uint32_t multiboot_info_phys) {
    terminal_initialize();
    printf("=== Welcome to PumpsOS ===\n\n");

    // CPU Tables (GDT, TSS and IDT)

    gdt_init();
    
    uint32_t esp_now;
    asm volatile("movl %%esp, %0" : "=r"(esp_now));
    tss_install(5, esp_now);

    idt_init();

    // Memory Management

    multiboot_info_t* mboot = (multiboot_info_t*)(multiboot_info_phys + 0xC0000000);

    if (!memory_map_init(mboot)) {
        PANIC("Failed to initialize memory map");
    }
    pmm_init();
    vmm_init();
    heap_init();

    // Kernel subsystems
    syscall_init();

    vfs_root = initrd_init_from_multiboot(mboot);
    if (!vfs_root) {
        printf("No initrd loaded\n");
    }

    // Devices / Interrupts

    pic_init();
    timer_init(100);
    keyboard_init();
    console_init();
    asm volatile("sti");

    // Scheduler and tasks

    scheduler_init();
    debugger_init();

    printf("sizeof(Elf32_Ehdr) = %d (expect 52)\n", sizeof(Elf32_Ehdr));
    printf("sizeof(Elf32_Phdr) = %d (expect 32)\n", sizeof(Elf32_Phdr));
    vfs_node_t* n = vfs_lookup("/readme.txt");
    if (n) {
        Elf32_Ehdr hdr;
        vfs_read(n, 0, sizeof(hdr), (uint8_t*)&hdr);
        printf("validate readme.txt: %d (expect 0)\n", elf_validate(&hdr));
    }

    //task_create_user("user_test", user_payload, sizeof(user_payload), PRIORITY_NORMAL);
    //task_create_user_elf("user_elf", "/hello.elf", PRIORITY_NORMAL);
    //task_create_user_elf("fork_test", "/fork_test.elf", PRIORITY_NORMAL);
    //task_create_user_elf("wait_test", "/wait_test.elf", PRIORITY_NORMAL);
    //task_create_user_elf("read_test", "/read_test.elf", PRIORITY_NORMAL);
    task_create_user_elf("echo_test", "/echo_test.elf", PRIORITY_NORMAL);
    scheduler_enable_preemption();

    // printf("> ");
    // while(1) {
    //     char c = keyboard_getchar();
    //     if(c == '\n') {
    //         printf("\n> ");
    //     } else if(c == '\b') {
    //         printf("\b \b");
    //     } else {
    //         printf("%c", c);
    //     }
    // }
    while(1) {
        asm volatile("hlt");
    }
}