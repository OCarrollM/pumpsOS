#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>
#include <stdint.h>

void terminal_initialize(void);
void terminal_setcolor(uint8_t color);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);
void terminal_print_hex(uint32_t value);

// Header file that will initialize the terminal functions rather than loading
// them in when the c program is run. 

// The ifndef is a guard that checks is a unique value is defined. If not itll 
// define it and move on with the rest of the file.

#endif