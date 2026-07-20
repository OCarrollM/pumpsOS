#include "cursor.h"
#include "cursor_bitmap.h"
#include "framebuffer.h"
#include "mouse.h"
#include <stdbool.h>

static uint32_t backing[CURSOR_H][CURSOR_W];
static int32_t last_x = 0, last_y = 0;
static bool drawn = false;

#define CURSOR_OUTLINE 0x000000
#define CURSOR_FILL 0xFFFFFF

// save screen state
static void save_backing(int32_t x, int32_t y) {
    for (int dy = 0; dy < CURSOR_H; dy++) {
        for (int dx = 0; dx < CURSOR_W; dx++) {
            backing[dy][dx] = read_pixel(x + dx, y + dy);
        }
    }
}

// put any saved pixels back where the cursor was previously
static void restore_backing(void) {
    for (int dy = 0; dy < CURSOR_H; dy++) {
        for (int dx = 0; dx < CURSOR_W; dx++) {
            draw_pixel(last_x + dx, last_y + dy, backing[dy][dx]);
        }
    }
}

// paint the cursor bitmap
static void paint_cursor(int32_t x, int32_t y) {
    for (int dy = 0; dy < CURSOR_H; dy++) {
        for (int dx = 0; dx < CURSOR_W; dx++) {
            uint8_t v = cursor_bitmap[dy][dx];
            if (v == 0) continue;
            draw_pixel(x + dx, y + dy, (v == 1) ? CURSOR_OUTLINE : CURSOR_FILL);
        }
    }
}

void cursor_hide(void) {
    if (!drawn) return;
    restore_backing();
    drawn = false;
}

void cursor_show(void) {
    if (drawn) return;
    last_x = mouse_x;
    last_y = mouse_y;
    save_backing(last_x, last_y);
    paint_cursor(last_x, last_y);
    drawn = true;
}

void cursor_update(void) {
    if (drawn && mouse_x == last_x && mouse_y == last_y) return;
    cursor_hide();
    cursor_show();
}