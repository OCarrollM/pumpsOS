// src/kernal.c
#include <stdio.h>
#include <stddef.h>
#define COM1 0x3F8
// Awesome sauce from Paul Dempster 2021

// VGA Text Buffer
volatile __uint16_t* vga_buffer = (__uint16_t*)0xB8000;
const size_t VGA_WIDTH = 80;

static inline __uint16_t vga_entry(unsigned char ch, __uint8_t color) {
    return (__uint16_t)ch | (__uint16_t)color << 8;
}

static inline void serial_write_char(char c) { 
    while(!(*(volatile unsigned char *)(COM1 + 5) & 0x20 ));
    *(volatile unsigned char *)(COM1) = c;
}

static void serial_write(const char* str) {
    for(size_t i=0; str[i]; i++) {
        serial_write_char(str[i]);
    }
}

void kernel_main(void) {
    const char *msg = "I love Pumps, I also love baby flo but this is pumps tiny kernal";
    serial_write(msg);
    // simple loop
    for(;;) { __asm__ ("hlt"); }
}

// This is gonna write directly to a VGA text buffer
// no bootloader code just kernal logic, pumps says we have to start
// somewhere