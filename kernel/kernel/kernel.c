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
    rtc_time_t now;
    rtc_read(&now);
    printf("Current time: %d-%02d-%02d %02d:%02d:%02d UTC\n", now.year, now.month, now.day, now.hour, now.minute, now.second);
    asm volatile("sti");

    // Scheduler and tasks

    scheduler_init();
    //thread_create("thread-A", thread_body_finite, "AAA", PRIORITY_NORMAL);
    //thread_create("thread-B", thread_body_forever, "BBB", PRIORITY_NORMAL);
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
    task_create_user_elf("user_elf", "/hello.elf", PRIORITY_NORMAL);
    //task_create_user_elf("fork_test", "/fork_test.elf", PRIORITY_NORMAL);
    //task_create_user_elf("wait_test", "/wait_test.elf", PRIORITY_NORMAL);
    //task_create_user_elf("read_test", "/read_test.elf", PRIORITY_NORMAL);
    //task_create_user_elf("echo_test", "/echo_test.elf", PRIORITY_NORMAL);
    task_create_user_elf("shell", "/shell.elf", PRIORITY_NORMAL);
    scheduler_enable_preemption();

    uint8_t sector[512];
    if (ata_read_sector(0, sector)) {
        printf("Sector 0 read OK. First bytes: ");
        for (int i = 0; i < 20; i++) {
            printf("%c", sector[i] >= 32 && sector[i] < 127 ? sector[i] : '.');
        }
        printf("\n");
    } else {
        printf("ATA read FAILED\n");
    }

    // write test
    uint8_t wbuf[512];
    memset(wbuf, 0, 512);
    const char* msg = "WRITTEN BY PUMPSOS KERNEL";
    memcpy(wbuf, msg, strlen(msg));

    if (ata_write_sector(1, wbuf)) {
        printf("Wrote sector 1.\n");
        uint8_t rbuf[512];
        if (ata_read_sector(1, rbuf)) {
            printf("Read back: ");
            for (int i = 0; i < 25; i++) {
                printf("%c", rbuf[i] >= 32 && rbuf[i] < 127 ? rbuf[i] : '.');
            }
            printf("\n");
        }
    } else {
        printf("ATA write FAILED\n");
    }

    pfs_mkfs();
    pfs_mount();
     // inode and block test
    int32_t i1 = pfs_alloc_inode();
    int32_t i2 = pfs_alloc_inode();
    int32_t b1 = pfs_alloc_block();
    int32_t b2 = pfs_alloc_block();
    printf("Alloc inodes: %d %d, blocks: %d %d\n", i1, i2, b1, b2);

    // third test on files
    int32_t ino = pfs_create("hello.txt");
    printf("Created hello.txt as inode %d\n", ino);

    const char* text = "Hello from PumpsFS";
    pfs_write_file(ino, (const uint8_t*)text, strlen(text));
    printf("Wrote %d bytes\n", (int)strlen(text));

    int32_t found = pfs_lookup("hello.txt");
    printf("Lookup hello.txt -> inode %d\n", found);

    uint8_t buf[512];
    int32_t bytes_read = pfs_read_file(found, buf, sizeof(buf));
    buf[bytes_read] = '\0';
    printf("read %d bytes: %s\n", bytes_read, buf);

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
        task_reap_terminated();
        asm volatile("hlt");
    }
}