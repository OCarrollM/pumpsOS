/* The bootloader will look for this file and start at the
chosen entry point being _start */
ENTRY(_start)

/* We then now need to tell where the sections of the .o files
will be put in the final image */

SECTIONS {
    /* Before we lived at 2M, for higher half we now live at
    3GB + 1GB which is mapped to 1MB */
    . = 0x00100000;

    _kernel_start = .;
    .multiboot.data : {
        *(.multiboot.data)
    }

    .multiboot.text : {
        *(.multiboot.text)
    }

    . += 0xC0000000;

    /* First, we put in the multiboot header. This needs to
    be in the early image or else the bootloader will not
    recognize us. We will do this in a .text section */
    .text ALIGN(4K) : AT (ADDR (.text) - 0xC0000000) {
        *(.text)
    }

    /* .init and .fini for global con/destructors */
    .init ALIGN(4K) : AT (ADDR (.init) - 0xC0000000) {
        *(.init)
    }

    .fini ALIGN(4K) : AT (ADDR (.fini) - 0xC0000000) {
        *(.fini)
    }

    /* Read-only */
    .rodata ALIGN(4K) : AT (ADDR (.rodata) - 0xC0000000) {
        *(.rodata)
    }

    /* Read-Write */
    .data ALIGN(4K) : AT (ADDR (.data) - 0xC0000000) {
        *(.data)
    }

    /* constructors */
    .ctors ALIGN(4K) : AT (ADDR (.ctors) - 0xC0000000) {
        *(.ctors)
    }

    /* destructors */
    .dtors ALIGN(4K) : AT (ADDR (.dtors) - 0xC0000000) {
        *(.dtors)
        PROVIDE(__DTOR_END__ = .);
    }

    /* Thread storage */
    .tm_clone_table ALIGN(4K) : AT (ADDR (.tm_clone_table) - 0xC0000000) {
        *(.tm_clone_table)
        PROVIDE(__TMC_END__ = .);
    }

    /* Read-Write and stack */
    .bss ALIGN(4K) : AT (ADDR (.bss) - 0xC0000000) {
        *(COMMON)
        *(.bss)
        *(.bootstrap_stack)
    }
    
    _kernel_end = .;
}