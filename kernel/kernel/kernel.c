#include <stdio.h>
#include <string.h>

#include <kernel/tty.h>
#include "../arch/i386/gdt.h"
#include "../arch/i386/idt.h"
#include "../arch/i386/pic.h"
#include "../arch/i386/timer.h"
#include "../arch/i386/keyboard.h"
#include "../arch/i386/tss.h"
#include "../arch/i386/rtc.h"
#include "../arch/i386/ports.h"
#include "../arch/i386/ata.h"
#include "../arch/i386/framebuffer.h"
#include "../arch/i386/fbcon.h"
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
#include "../kernel/pfs.h"

// For ring 3
static const uint8_t user_payload[] = {
    0xB8, 0x01, 0x00, 0x00, 0x00, 0xBB, 0x01, 0x00, 0x00, 0x00, 0xB9, 0x24,
    0x00, 0x40, 0x00, 0xBA, 0x12, 0x00, 0x00, 0x00, 0xCD, 0x80, 0xB8, 0x00,
    0x00, 0x00, 0x00, 0xBB, 0x00, 0x00, 0x00, 0x00, 0xCD, 0x80, 0xEB, 0xFE,
    0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x66, 0x72, 0x6F, 0x6D, 0x20, 0x72,
    0x69, 0x6E, 0x67, 0x20, 0x33, 0x0A,
};

static void thread_body_forever(void* arg) {
    const char* tag = (const char*)arg;
    int i = 0;
    for (;;) {
        printf("[%s] %d\n", tag, i++);
        for (volatile int d = 0; d < 20000000; d++) { }
    }
}

/* Finite: returns after 5 iterations -> trampoline calls task_exit(0) */
static void thread_body_finite(void* arg) {
    const char* tag = (const char*)arg;
    for (int i = 0; i < 5; i++) {
        printf("[%s] %d\n", tag, i);
        for (volatile int d = 0; d < 20000000; d++) { }
    }
    printf("[%s] returning (will exit)\n", tag);
    /* returns -> thread_trampoline -> task_exit(0) */
}

void kernel_main(uint32_t multiboot_info_phys) {
    terminal_initialize();
    // printf("=== Welcome to PumpsOS ===\n\n");

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

    // graphics mode, any text below is now pointless
    if (framebuffer_init(mboot)) {
        // fill_screen(0x0000FF);
        // draw_rect(100, 100, 250, 150, 0xFF0000);
        fbcon_init();
    }
    
    // Kernel subsystems
    syscall_init();
    
    vfs_root = initrd_init_from_multiboot(mboot);
    if (!vfs_root) {
        printf("No initrd loaded\n");
    }
    
    // Devices / Interrupts
    
    printf("=== Welcome to PumpsOS ===\n\n");
    pic_init();
    timer_init(100);
    keyboard_init();
    console_init();
    rtc_time_t now;
    rtc_read(&now);
    printf("Current time: %d-%02d-%02d %02d:%02d:%02d UTC\n", now.year, now.month, now.day, now.hour, now.minute, now.second);

    if (!pfs_mount()) {
        printf("No filesystem found, formatting disk...\n");
        pfs_mkfs();
        pfs_mount();
    }
    vfs_node_t* pfs_root = pfs_vfs_init();
    printf("pfs_vfs_init done\n");
    vfs_mount("disk", pfs_root);
    printf("vfs_mount done\n");

    if (pfs_lookup("hello.txt") < 0) {
        int32_t seed_ino = pfs_create("hello.txt");
        printf("Seed created\n");
        if (seed_ino > 0) {
            const char* text = "Hello from PumpsFS";
            printf("Writing seed file\n");
            pfs_write_file(seed_ino, (const uint8_t*)text, strlen(text));
            printf("Seed written\n");
        }
    }
    printf("Reached scheduler_init\n");

    // Scheduler and tasks

    scheduler_init();
    debugger_init();

    asm volatile("sti");

    // task_create_user_elf("user_elf", "/hello.elf", PRIORITY_NORMAL);
    printf("Creating shell task\n");
    task_t* sh = task_create_user_elf("shell", "/shell.elf", PRIORITY_NORMAL);
    printf("shell task created: %x\n", (uint32_t)sh);
    scheduler_enable_preemption();
    printf("Preemption enabled, entering idle\n");

    while(1) {
        task_reap_terminated();
        asm volatile("hlt");
    }
}