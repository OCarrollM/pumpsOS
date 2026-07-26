#include "window.h"
#include "framebuffer.h"
#include "fbcon.h"
#include "cursor.h"
#include <string.h>

#define COL_BORDER 0x202020
#define COL_TITLEBAR 0x3060A0
#define COL_TITLETEXT 0xFFFFFF

static window_t windows[MAX_WINDOWS];
static int window_count = 0;

window_t* window_create(int32_t x, int32_t y, int32_t h, int32_t w, const char* title, uint32_t bg) {
    if (window_count >= MAX_WINDOWS) return NULL;

    window_t* win = &windows[window_count++];
    win->x = x;
    win->y = y;
    win->w = w;
    win->h = h;
    win->bg = bg;
    win->used = true;
    strncpy(win->title, title, sizeof(win->title) - 1);
    win->title[sizeof(win->title) - 1] = '\0';
    return win;
}

int32_t window_client_x(const window_t* win) { return win->x + WIN_BORDER; }
int32_t window_client_y(const window_t* win) { return win->y + WIN_BORDER + WIN_TITLEBAR; }
int32_t window_client_w(const window_t* win) { return win->w - 2 * WIN_BORDER; }
int32_t window_client_h(const window_t* win) { return win->h - 2 * WIN_BORDER - WIN_TITLEBAR; }

void window_draw(window_t* win) {
    if (!win || !win->used) return;

    draw_rect(win->x, win->y, win->w, win->h, COL_BORDER);
    draw_rect(win->x + WIN_BORDER, win->y + WIN_BORDER, win->w - 2 * WIN_BORDER, WIN_TITLEBAR, COL_TITLEBAR); // Title bar

    // title bar text
    fbcon_draw_string_at(win->x + WIN_BORDER + 4, win->y + WIN_BORDER + 2, win->title, COL_TITLETEXT, COL_TITLEBAR);

    // client part
    draw_rect(window_client_x(win), window_client_y(win), window_client_w(win), window_client_h(win), win->bg);
}

void wm_redraw(void) {
    cursor_hide();
    for (int i = 0; i < window_count; i++) {
        window_draw(&windows[i]);
    }
    cursor_show();
}