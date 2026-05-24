# void user_mode_enter(void);
#
# Kernel function

.section .text
.global user_mode_enter
.type user_mode_enter, @function

user_mode_enter:
    movl 4(%esp), %ebx           # ebx = entry
    movl 8(%esp), %ecx           # ecx = user_stack_top

    movw $0x23, %dx
    movw %dx, %ds
    movw %dx, %es
    movw %dx, %fs
    movw %dx, %gs

    pushl $0x23                  # SS
    pushl %ecx                   # user ESP
    pushl $0x202                 # EFLAGS, IF set
    pushl $0x1B                  # CS
    pushl %ebx                   # EIP

    iret