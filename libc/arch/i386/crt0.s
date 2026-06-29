# Program entry
.section .text
.global _start
.type _start, @function
_start:
    movl 4(%esp), %eax # argv
    movl 0(%esp), %ebx # argc
    pushl %eax
    pushl %ebx
    call main
    pushl %eax
    call exit
1: jmp 1b
