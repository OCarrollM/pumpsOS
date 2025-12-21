/* x86 crti.s */
.section .init
.global _init
.type _init, @function
_init:
    push %ebp
    movl %esp, %ebp
    /* This is where crtbegin.o .init goes */

.section .fini
.global _fini
.type _fini, @function
_fini:
    push %ebp
    movl %esp, %ebp
    /* Where .fini goes */
    