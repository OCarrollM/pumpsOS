.section .text
.global _start
.type _start, @function
_start:
    call kernel_main
1:  hlt
    jmp 1b
