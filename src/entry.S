/* src/entry.S */
.section .text
.global _start
.type _start, @function

_start:
    /* Jump to start... and call the main function from kernel.c */
    call kernel_main

1: hlt
    jmp 1b