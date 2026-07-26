#include "terminal.h"
#include "framebuffer.h"
#include "fbcon.h"
#include "cursor.h"
#include "font8x16.h"
#include <string.h>

#define TERM_FG 0xD0D0D0
#define TERM_BG 0x101010
#define TERM_CUR 0x30C030

static terminal_t term;
static bool have_term = false;

// pixel position of cell in window area
static int32_t cell_px(const terminal_t* t, int col) {
    return window_client_x(t->win) + col * FONT_WIDTH;
}
static int32_t cell_py(const terminal_t* t, int row) {
    return window_client_y(t->win) + row * FONT_HEIGHT;
}

// repaint whole grid
static void term_draw_content(window_t* win) {
    terminal_t* t = (terminal_t*)win->content;
    if (!t) return;

    for (int row = 0; row < t->rows; row++) {
        for (int col = 0; col < t->cols; col++) {
            char c = t->cells[row][col];
            if (c == '\0') c = ' ';
            fbcon_draw_char_at(cell_px(t, col), cell_py(t, row), c, TERM_FG, TERM_BG);
        }
    }
    draw_rect(cell_px(t, t->cur_x), cell_py(t, t->cur_y) + FONT_HEIGHT - 2, FONT_WIDTH, 2, TERM_CUR);
}

static void term_clear(terminal_t* t) {
    for (int r = 0; r < TERM_MAX_ROWS; r++) {
        for (int c = 0; c < TERM_MAX_COLS; c++) {
            t->cells[r][c] = ' ';
        }
    }
    t->cur_x = 0;
    t->cur_y = 0;
}

// shift the rows up by one
static void term_scroll(terminal_t* t) {
    for (int r = 0; r < t->rows - 1; r++) {
        for (int c = 0; c < t->cols; c++) {
            t->cells[r][c] = t->cells[r + 1][c];
        }
    }
    for (int c = 0; c < t->cols; c++) {
        t->cells[t->rows - 1][c] = ' ';
    }
    t->cur_y = t->rows - 1;
}

terminal_t* terminal_create(int32_t x, int32_t y, int32_t w, int32_t h, const char* title) {
    window_t* win = window_create(x, y, w, h, title, TERM_BG);
    if (!win) return 0;

    term.win = win;
    term.cols = window_client_w(win) / FONT_WIDTH;
    term.rows = window_client_h(win) / FONT_HEIGHT;
    if (term.cols > TERM_MAX_COLS) term.cols = TERM_MAX_COLS;
    if (term.rows > TERM_MAX_ROWS) term.rows = TERM_MAX_ROWS;
    term_clear(&term);

    win->content = &term;
    win->draw_content = term_draw_content;

    have_term = true;
    return &term;
}

// update the grid for one character
static bool term_putchar_buffered(char c) {
    terminal_t* t = &term;
    bool needs_full_redraw = false;

    if (c == '\n') {
        t->cur_x = 0;
        t->cur_y++;
    } else if (c == '\r') {
        t->cur_x = 0;
    } else if (c == '\b') {
        if (t->cur_x > 0) {
            t->cur_x--;
            t->cells[t->cur_y][t->cur_x] = ' ';
        }
    } else if (c == '\t') {
        t->cur_x = (t->cur_x + 4) & ~3;
        if (t->cur_x >= t->cols) { t->cur_x = 0; t->cur_y++; }
    } else {
        t->cells[t->cur_y][t->cur_x] = c;
        t->cur_x++;
        if (t->cur_x >= t->cols) { t->cur_x = 0; t->cur_y++; }
    }

    if (t->cur_y >= t->rows) {
        term_scroll(t);
        needs_full_redraw = true;
    }
    return needs_full_redraw;
}

void term_putchar(char c) {
    term_write(&c, 1);
}

void term_write(const char* data, size_t len) {
    if (!have_term) return;
    terminal_t* t = &term;

    cursor_hide();

    bool full = false;
    for (size_t i = 0; i < len; i++) {
        draw_rect(cell_px(t, t->cur_x), cell_py(t, t->cur_y) + FONT_HEIGHT - 2, FONT_WIDTH, 2, TERM_BG);

        int prev_x = t->cur_x, prev_y = t->cur_y;
        if (term_putchar_buffered(data[i])) {
            full = true;
        } else if (!full) {
            char ch = t->cells[prev_y][prev_x];
            fbcon_draw_char_at(cell_px(t, prev_x), cell_py(t, prev_y), ch == '\0' ? ' ' : ch, TERM_FG, TERM_BG);
        }
    }

    if (full) {
        term_draw_content(t->win);
    } else {
        draw_rect(cell_px(t, t->cur_x), cell_py(t, t->cur_y) + FONT_HEIGHT - 2, FONT_WIDTH, 2, TERM_CUR);
    }

    cursor_show();
}

bool term_active(void) { return have_term; }