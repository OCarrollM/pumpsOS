/* The bootloader will look for this file and start at the
chosen entry point being _start */
ENTRY(_start)

/* We then now need to tell where the sections of the .o files
will be put in the final image */

SECTIONS {
    /* We use a start offset of 2M as research claimed and provide
    that it was a safer starting point in memory. As 1M became
    a little iffy under the UEFI changes */
    . = 2M;

    /* First, we put in the multiboot header. This needs to
    be in the early image or else the bootloader will not
    recognize us. We will do this in a .text section */
    .text BLOCK(4K) : ALIGN(4K) {
        *(.multiboot)
        *(.text)
    }

    /* .init and .fini for global con/destructors */
    .init BLOCK(4K) : ALIGN(4K) {
        *(.init)
    }

    .fini BLOCK(4K) : ALIGN(4K) {
        *(.fini)
    }

    /* Read-only */
    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.rodata)
    }

    /* Read-Write */
    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)
    }

    /* constructors */
    .ctors BLOCK(4K) : ALIGN(4K) {
        *(.ctors)
    }

    /* destructors */
    .dtors BLOCK(4K) : ALIGN(4K) {
        *(.dtors)
        PROVIDE(__DTOR_END__ = .);
    }

    /* Thread storage */
    .tm_clone_table BLOCK(4K) : ALIGN(4K) {
        *(.tm_clone_table)
        PROVIDE(__TMC_END__ = .);
    }

    /* Read-Write and stack */
    .bss BLOCK(4K) : ALIGN(4K) {
        *(COMMON)
        *(.bss)
    }
}