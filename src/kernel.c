// src/kernal.c
#include <stdio.h>
#include <stddef.h>
// Awesome sauce from Paul Dempster 2021

// VGA Text Buffer
volatile __uint16_t* vga_buffer = (__uint16_t*)0xB8000;
const size_t VGA_WIDTH = 80;

static inline __uint16_t vga_entry(unsigned char ch, __uint8_t color) {
    return (__uint16_t)ch | (__uint16_t)color << 8;
}

void kernel_main(void) {
    const char *msg = "I love Pumps, I also love baby flo but this is pumps tiny kernal";
    size_t i = 0;
    // eliminate the first line
    for (i=0; i<VGA_WIDTH; i++) {
        vga_buffer[i] = vga_entry(' ', 0x07);
    }
    // Write some message lol
    i = 0;
    while(msg[i]) {
        vga_buffer[i] = vga_entry(msg[i], 0x0F); // white on black should be orange though
        i++;
    }
    // simple loop
    for(;;) { __asm__ ("hlt"); }
}

// This is gonna write directly to a VGA text buffer
// no bootloader code just kernal logic, pumps says we have to start
// somewhere