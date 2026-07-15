// framebuffer for text
#include "fbcon.h"
#include "framebuffer.h"
#include "font8x16.h"

static uint32_t cur_x = 0; // column
static uint16_t cur_y = 0; // row
static uint32_t cols = 0; // screen width
static uint32_t rows = 0; // screen height
static uint32_t fg_colour = 0xFFFFFF; // white
static uint32_t bg_colour = 0x000000; // black

// draw one char
static void draw_glyph(uint32_t px, uint32_t py, char c, uint32_t fg, uint32_t bg) {
    unsigned char uc = (unsigned char)c;
    if (uc >= 128) uc = '?'; // only 127 chars
    const uint8_t* glyph = font8x16[uc];

    for (uint32_t row = 0; row < FONT_HEIGHT; row++) {
        uint8_t bits = glyph[row];
        for (uint32_t col = 0; col < FONT_WIDTH; col++) {
            uint32_t colour = (bits & (0x80 >> col)) ? fg : bg;
            draw_pixel(px + col, py + row, colour);
        }
    }
}

void fbcon_init(void) {
    cols = fb_get_width() / FONT_WIDTH;
    rows = fb_get_height() / FONT_HEIGHT;
    cur_x = 0;
    cur_y = 0;
    fbcon_clear();
}

void fbcon_set_colour(uint32_t fg, uint32_t bg) {
    fg_colour = fg;
    bg_colour = bg;
}

void fbcon_clear(void) {
    fill_screen(bg_colour);
    cur_x = 0;
    cur_y = 0;
}

// scroll screen up by one at a time. for redrawing as no back buffer atm
static void scroll_if_needed(void) {
    if (cur_y >= rows) {
        fbcon_clear();
    }
}

void fbcon_putchar(char c) {
    if (c == '\n') {
        cur_x = 0;
        cur_y++;
        scroll_if_needed();
        return;
    }
    if (c == '\r') {
        cur_x = 0;
        return;
    }
    if (c == '\b') {
        if (cur_x > 0) {
            cur_x--;
            draw_glyph(cur_x * FONT_WIDTH, cur_y * FONT_HEIGHT, ' ', fg_colour, bg_colour);
        }
        return;
    }
    if (c == '\t') {
        cur_x = (cur_x + 4) & ~3u;
        if (cur_x >= cols) { cur_x = 0; cur_y++; scroll_if_needed(); }
        return;
    }

    draw_glyph(cur_x * FONT_WIDTH, cur_y * FONT_HEIGHT, c, fg_colour, bg_colour);
    cur_x++;
    if (cur_x >= cols) {
        cur_x = 0;
        cur_y++;
        scroll_if_needed();
    }
}

void fbcon_write(const char* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        fbcon_putchar(data[i]);
    }
}