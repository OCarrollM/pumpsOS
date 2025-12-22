.set ALIGN,     1<<0                /* Aligns modules on boundaries */
.set MEMINFO,   1<<1                /* Provide memory map */
.set FLAGS,     ALIGN | MEMINFO     /* Multiboot flag field */
.set MAGIC,     0x1BADB002          /* Magic number, helps find header */
.set CHECKSUM,  -(MAGIC + FLAGS)    /* Checksum to prove us */

/* 
Declare us to mark us as a kernel. Bootloader will search
for signiture in first 8 KiB. This can be forced
*/
.section .multiboot 
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* 
Create stack, it must be aligned by 16 bytes to be recognized
failure to do so will cause undefined errors
*/
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

/*
We specify _start as the starting point and ask the Bootloader
to jump to this section when the kernel has loaded
*/
.section .text
.global _start
.type _start, @function
_start:
    /* 
    From here, the bootloader has given the kernel full control
    over our CPU. There is absolutely nothing unless we provide
    it (stdio.h for example)
    */

    /*
    We need to set up a stack, so set the esp reg to the top
    of our created stack from before. This is done here because
    C will not work without stacks
    */
    mov $stack_top, %esp

    /* Global constructors */
    call _init

    /*
    Next we will initialize processor state. This will minimize
    early environments where features are usually offline
    */
    /*
    First, call the kernel
    */
    call kernel_main

    /*
    If we have nothing else to do, it is easy to just
    throw the computer into a loop which to do so we:
    1: Disable interrupts
    2: Wait for next interrupt to hit (this will lock the
    computer)
    3: Jump to the hlt instruction
    */
    cli
1:  hlt
    jmp 1b

/*
Finally we set the size of our _start to the current location
(.) minus the starting
*/
.size _start, . - _start
