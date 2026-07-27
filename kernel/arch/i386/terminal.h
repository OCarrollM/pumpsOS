#ifndef ARCH_I386_TERMINAL_H
#define ARCH_I386_TERMINAL_H

#include <stddef.h>
#include "window.h"

#define TERM_MAX_COLS 128
#define TERM_MAX_ROWS 48

typedef struct {
    window_t* win;
    int cols, rows;
    char cells[TERM_MAX_ROWS][TERM_MAX_COLS];
    int cur_x, cur_y;
} terminal_t;

terminal_t* terminal_create(int32_t x, int32_t y, int32_t w, int32_t h, const char* title);
void term_write(const char* data, size_t len);
void term_putchar(char c);
bool term_active(void);

#endif