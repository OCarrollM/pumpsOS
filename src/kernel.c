#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Check if the compiler thinks we are looking for wrong OS */
#if defined(__linux__)
#error "You are not using a cross-compiler, sort it out"
#endif

#if !defined(__i386__)
#error "This OS will need to be compiled with ix86-elf compiler"
#endif

/* Hardware colors */
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

/* Here we are creating our own version of string length as we
cannot use the standard C library. 

This will need to be done for every addition we wish to use
*/
size_t strlen(const char* str) {
    size_t len = 0;
    while(str[len]) {
        len++;
    }
    return len;
}

#define VGA_WIDTH       80
#define VGA_HEIGHT      25
#define VGA_MEMORY      0xB8000

size_t terminal_row;
size_t terminal_column;
size_t terminal_color;
uint16_t* terminal_buffer = (uint16_t*)VGA_MEMORY;

void terminal_initilize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    for(size_t y = 0; y < VGA_HEIGHT; y++) {
        for(size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

// Terminal scroll function
// If the terminal is filled (100 chars in a 10x10) then
// move rows up by 1 and remove the top row that was there before
// This is looping to HEIGHT - 1, copying the row to one before
// then filles final row with empty spots

void terminal_scroll() {
    for(int i = 0; i < VGA_HEIGHT - 1; i++) { // eg: height = 10 so i will go 0 - 10
        for(int j = 0; j < VGA_WIDTH; j++) { // eg: same for width
            terminal_buffer[i * VGA_WIDTH + j] = terminal_buffer[(i + 1) * VGA_WIDTH + j];
        }
    }

    // Clear the bottom line
    for(int i = 0; i < VGA_WIDTH; i++) {
        terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + i] = vga_entry(' ', terminal_color);
    }
}

// in terminal_putchar check if c == '\n' and 
// increment terminal_row and reset terminal_column.

// Will probbs needs to update this a lot

void terminal_putchar(char c) {

    if(c == '\n'){
        ++terminal_row;
        terminal_column = 0;

        if(terminal_row == VGA_HEIGHT) {
            terminal_scroll();
            terminal_row = VGA_HEIGHT - 1; // This keeps the insert on last row
        }
    } else {
        terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
        if(++terminal_column == VGA_WIDTH) {
            terminal_column = 0;
            if(++terminal_row == VGA_HEIGHT) {
                terminal_scroll();
                terminal_row = VGA_HEIGHT - 1;
            }
        }
    }

}

void terminal_write(const char* data, size_t size) {
    for(size_t i = 0; i < size; i++) {
        terminal_putchar(data[i]);
    }
}

void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}

void kernel_main(void) {
    /* Initialise terminal */
    terminal_initilize();

    terminal_writestring("Line 1\n");
    terminal_writestring("Line 2 I should be line 1\n");
    terminal_writestring("Line 3\n");
    terminal_writestring("Line 4\n");
    terminal_writestring("Line 5\n");
    terminal_writestring("Line 6\n");
    terminal_writestring("Line 7\n");
    terminal_writestring("Line 8\n");
    terminal_writestring("Line 9\n");
    terminal_writestring("Line 10\n");
    terminal_writestring("Line 11\n");
    terminal_writestring("Line 12\n");
    terminal_writestring("Line 13\n");
    terminal_writestring("Line 14\n");
    terminal_writestring("Line 15\n");
    terminal_writestring("Line 16\n");
    terminal_writestring("Line 17\n");
    terminal_writestring("Line 18\n");
    terminal_writestring("Line 19\n");
    terminal_writestring("Line 20\n");
    terminal_writestring("Line 21\n");
    terminal_writestring("Line 22\n");
    terminal_writestring("Line 23\n");
    terminal_writestring("Line 24\n");
    terminal_writestring("Line 25\n");
    terminal_writestring("Line 26\n"); // adding the \n makes it have the black line!
}