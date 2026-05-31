/* Linker script for freestanding programs */

ENTRY(_start)

SECTIONS
{
    . = 0x00400000;

    .text : ALIGN(0x1000) {
        *(.text*)
        *(.rodata*)
    }

    .data : ALIGN(0x1000) {
        *(.data*)
        *(.bss*)
    }
}