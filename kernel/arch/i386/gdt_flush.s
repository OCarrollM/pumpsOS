# Loads GDT and reloads registers

.section .text
.global gdt_flush
.type gdt_flush, @function

gdt_flush:
    # Pointer to gdt
    movl 4(%esp), %eax

    # Load GDT
    lgdt (%eax)

    # Reload registers
    # Load 0x10 into all data segments
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    # Reload CS
    jmp $0x08, $.flush

.flush:
    ret
    