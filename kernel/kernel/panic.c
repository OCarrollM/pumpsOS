#include "panic.h"
#include <stdio.h>
#include <kernel/tty.h>

static inline uint32_t read_cr2(void) {
    uint32_t cr2;
    asm volatile("movl %%cr2, %0" : "=r"(cr2));
    return cr2;
}

static inline uint32_t read_cr3(void) {
    uint32_t cr3;
    asm volatile("movl %%cr3, %0" : "=r"(cr3));
    return cr3;
}

static const char* exception_names[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved"
};

void panic_dump_registers(struct registers* regs) {
    printf("\n--- CPU Registers ---\n");
    printf("  EAX=0x%x  EBX=0x%x  ECX=0x%x  EDX=0x%x\n",
           regs->eax, regs->ebx, regs->ecx, regs->edx);
    printf("  ESI=0x%x  EDI=0x%x  EBP=0x%x  ESP=0x%x\n",
           regs->esi, regs->edi, regs->ebp, regs->esp);
    printf("  EIP=0x%x  CS=0x%x   DS=0x%x   SS=0x%x\n",
           regs->eip, regs->cs, regs->ds, regs->ss);
    printf("  EFLAGS=0x%x\n", regs->eflags);
    printf("  CR2=0x%x (page fault addr)\n", read_cr2());
    printf("  CR3=0x%x (page directory)\n", read_cr3());
}

void panic_stack_trace(struct registers* regs) {
    printf("\n--- Stack trace ---\n");
    
    uint32_t* ebp;
    if (regs) {
        ebp = (uint32_t*)regs->ebp;
    } else {
        asm volatile("movl %%ebp, %0" : "=r"(ebp));
    }

    int frame = 0;
    int max_frames = 16;

    while (ebp && frame < max_frames) {
        if ((uint32_t)ebp < 0xC0000000 || (uint32_t)ebp >= 0xFFC00000) {
            printf("    #%d <invalid frame at 0x%x>\n", frame, (uint32_t)ebp);
            break;
        }

        uint32_t return_addr = ebp[1];
        uint32_t prev_ebp = ebp[0];

        printf("    #%d 0x%x\n", frame, return_addr);

        if (prev_ebp == 0) {
            break;
        }

        ebp = (uint32_t*)prev_ebp;
        frame++;
    }

    if (frame == max_frames) {
        printf("    ... (truncated at %d frames)\n", max_frames);
    }
}

static void decode_page_fault(uint32_t err_code) {
    printf("    Cause:  ");
    printf("%s, ", (err_code & 1) ? "protection violation" : "page not present");
    printf("%s, ",  (err_code & 2) ? "write" : "read");
    printf("%s",   (err_code & 4) ? "user mode" : "kernel mode");
    if (err_code & 8)  printf(", reserved bit set");
    if (err_code & 16) printf(", instruction fetch");
    printf("\n");
}

void panic_from_exception(struct registers* regs) {
    /* Disable interrupts — nothing should preempt the panic */
    asm volatile("cli");

    terminal_setcolor(0x4F);  /* White on red */
    printf("\n");
    printf("=========================================\n");
    printf("            KERNEL PANIC                 \n");
    printf("=========================================\n");
    terminal_setcolor(0x07);  /* Back to grey on black */

    if (regs->int_no < 32) {
        printf("Exception: %s (int %d)\n",
               exception_names[regs->int_no], regs->int_no);
    } else {
        printf("Exception: Unknown (int %d)\n", regs->int_no);
    }
    printf("Error code: 0x%x\n", regs->err_code);

    /* Special decoding for page faults */
    if (regs->int_no == 14) {
        decode_page_fault(regs->err_code);
    }

    panic_dump_registers(regs);
    panic_stack_trace(regs);

    printf("\nSystem halted.\n");

    while (1) {
        asm volatile("cli; hlt");
    }
}

void panic(const char* msg, const char* file, int line) {
    asm volatile("cli");

    terminal_setcolor(0x4F);
    printf("\n");
    printf("=========================================\n");
    printf("            KERNEL PANIC                 \n");
    printf("=========================================\n");
    terminal_setcolor(0x07);

    printf("Message:  %s\n", msg);
    printf("Location: %s:%d\n", file, line);

    /* Build a minimal register snapshot for stack tracing */
    panic_stack_trace(NULL);

    printf("\nSystem halted.\n");

    while (1) {
        asm volatile("cli; hlt");
    }
}