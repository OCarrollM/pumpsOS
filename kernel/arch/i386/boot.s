/* Declare our constants */
.set ALIGN,     1<<0                /* Aligns modules on boundaries */
.set MEMINFO,   1<<1                /* Provide memory map */
.set FLAGS,     ALIGN | MEMINFO     /* Multiboot flag field */
.set MAGIC,     0x1BADB002          /* Magic number, helps find header */
.set CHECKSUM,  -(MAGIC + FLAGS)    /* Checksum to prove us */

/* 
Declare us to mark us as a kernel. Bootloader will search
for signiture in first 8 KiB. This can be forced
*/
.section .multiboot.data, "aw"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* 
Create stack, it must be aligned by 16 bytes to be recognized
failure to do so will cause undefined errors
*/
.section .bootstrap_stack, "aw", @nobits
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

/* Allocate pages, new for our higher half. We don't hardcode
the addresses, instead assuming they are just available for use.
This will let the bootloader know that it must avoid them when it
sees them */

.section .bss, "aw", @nobits
    .align 4096
boot_page_directory:
    .skip 4096
boot_page_table1:
    .skip 4096
# We can add many more page tables from here if the kernel grows beyond
# 3 MiB

/*
We specify _start as the starting point and ask the Bootloader
to jump to this section when the kernel has loaded
*/
.section .multiboot.text, "a"
.global _start
.type _start, @function
_start:
    /* 
    From here, the bootloader has given the kernel full control
    over our CPU. There is absolutely nothing unless we provide
    it (stdio.h for example)
    */

    # Physical address of boot page table 1
    movl $(boot_page_table1 - 0xC0000000), %edi
    movl $0, %esi
    movl $1023, %ecx
    # We first map address 0 and then map 1023 pages, 1024 is VGA

1:
    # Kernel mapping
    cmpl $_kernel_start, %esi
    jl 2f
    cmpl $(_kernel_end - 0xC0000000), %esi
    jge 3f

    # Map addresses as "present, writeable"
    movl %esi, %edx
    orl $0x003, %edx
    movl %edx, (%edi)

2:
    # Page size of 4096 bytes, boot page is 4, loop if not finished
    addl $4096, %esi
    addl $4, %edi
    loop 1b

3:
    # Map the VGA
    movl $(0x000B8000 | 0x003), boot_page_table1 - 0xC0000000 + 1023 * 4

    # The page table is used at both page directory entry 0 (virtually from 0x0
	# to 0x3FFFFF) (thus identity mapping the kernel) and page directory entry
	# 768 (virtually from 0xC0000000 to 0xC03FFFFF) (thus mapping it in the
	# higher half). The kernel is identity mapped because enabling paging does
	# not change the next instruction, which continues to be physical. The CPU
	# would instead page fault if there was no identity mapping.

    movl $(boot_page_table1 - 0xC0000000 + 0x003), boot_page_directory - 0xC0000000 + 0
    movl $(boot_page_table1 - 0xC0000000 + 0x003), boot_page_directory - 0xC0000000 + 768 * 4

    # Set cr3 to boot_page_directory
    movl $(boot_page_directory - 0xC0000000), %ecx
    movl %ecx, %cr3

    # Enable paging (Write protection)
    movl %cr0, %ecx
    orl $0x80010000, %ecx
    movl %ecx, %cr0

    # Jump to higher half
    lea 4f, %ecx
    jmp *%ecx

.section .text

4:
    # From here paging should be set up
    # We now unmap the identity mapping
    movl $0, boot_page_directory + 0

    # And reload crc3 to force a TLB flush
    movl %cr3, %ecx
    movl %ecx, %cr3

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

