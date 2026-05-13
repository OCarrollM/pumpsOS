#include "debugger.h"
#include "panic.h"
#include "../kernel/task.h"
#include "../kernel/heap.h"
#include "../kernel/pmm.h"
#include "../kernel/vmm.h"
#include "../arch/i386/keyboard.h"
#include <stdio.h>
#include <string.h>

static uint32_t parse_hex(const char* s) {
    uint32_t result = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }
    while (*s) {
        char c = *s;
        int digit;
        if (c >= '0' && c <= '9')   digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
        else break;
        result = (result << 4) | digit;
        s++;
    }
    return result;
}

static const char* skip_word(const char* s) {
    while (*s && *s != ' ' && *s != '\t') s++;
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

static bool cmd_matches(const char* cmd, const char* prefix) {
    while (*prefix) {
        if (*cmd != *prefix) return false;
        cmd++;
        prefix++;
    }
    return *cmd == '\0' || *cmd == ' ' || *cmd == '\t';
}

// Commands
static void cmd_help(void) {
    printf("Commands:\n");
    printf("  help              - show this message\n");
    printf("  regs              - dump CPU registers (if available)\n");
    printf("  trace             - print stack trace\n");
    printf("  tasks             - list all tasks\n");
    printf("  mem               - show memory stats (heap, PMM)\n");
    printf("  peek <addr>       - read 4 bytes at address (hex)\n");
    printf("  dump <addr> <n>   - dump n bytes starting at address\n");
    printf("  cont              - exit debugger and continue\n");
    printf("  halt              - halt the system\n");
}

static void cmd_peek(const char* args) {
    uint32_t addr = parse_hex(args);
    if (addr == 0) {
        printf("Usage: peek <hex_address>\n");
        return;
    }

    addr &= ~3u;
    uint32_t value = *(volatile uint32_t*)addr;
    printf("0x%x\n", addr, value);
}

static void cmd_dump(const char* args) {
    uint32_t addr = parse_hex(args);
    const char* rest = skip_word(args);
    uint32_t count = parse_hex(rest);

    if (addr == 0 || count == 0) {
        printf("Usage: dump <hex_address> <hex_count>\n");
        return;
    }
    if (count > 256) count = 256;

    uint8_t* ptr = (uint8_t*)(addr & ~0xFu);
    for (uint32_t offset = 0; offset < count; offset += 16) {
        printf("0x%x: ", (uint32_t)(ptr + offset));
        for (int i = 0; i < 16 && (offset + i) < count; i++) {
            printf("%x ", ptr[offset + i]);
        }
        printf("\n");
    }
}

static void cmd_mem(void) {
    heap_print_stats();
    printf("PMM: %d / %d pages free\n", pmm_get_free_page_count(), pmm_get_total_page_count());
}

static void read_command(char* buf, size_t max) {
    keyboard_readline(buf, max);
}

void debugger_enter(struct registers* regs) {
    printf("\n--- Kernel debugger ---\n");
    printf("Type 'help' for commands\n");

    asm volatile("sti");

    char buf[128];

    while (1) {
        printf("(db )");
        read_command(buf, sizeof(buf));

        if (buf[0] == '\0') {
            continue;
        }

        if (cmd_matches(buf, "help")) {
            cmd_help();
        } else if (cmd_matches(buf, "regs")) {
            if (regs) {
                panic_dump_registers(regs);
            } else {
                printf("(no saved register state available)\n");
            }
        } else if (cmd_matches(buf, "trace")) {
            panic_stack_trace(regs);
        } else if (cmd_matches(buf, "tasks")) {
            scheduler_print_tasks();
        } else if (cmd_matches(buf, "mem")) {
            cmd_mem();
        } else if (cmd_matches(buf, "peek")) {
            cmd_peek(skip_word(buf));
        } else if (cmd_matches(buf, "dump")) {
            cmd_dump(skip_word(buf));
        } else if (cmd_matches(buf, "cont")) {
            printf("Continuing.\n");
            asm volatile("sti");
            return;
        } else if (cmd_matches(buf, "halt")) {
            printf("Halting.\n");
            while (1) asm volatile("cli; hlt");
        } else {
            printf("Unknown command. Type 'help'.\n");
        }
    }
}

void debugger_init(void) {
    printf("Kernel debugger initialized\n");
}