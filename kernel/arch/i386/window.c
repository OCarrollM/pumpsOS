#include "window.h"
#include "framebuffer.h"
#include "fbcon.h"
#include "cursor.h"
#include "mouse.h"
#include <string.h>

#define COL_DESKTOP 0x184878
#define COL_BORDER 0x202020
#define COL_TITLEBAR 0x3060A0
#define COL_TITLEBAR_TOP 0x5090D0
#define COL_TITLETEXT 0xFFFFFF

static window_t windows[MAX_WINDOWS];
static int z_order[MAX_WINDOWS];
static int window_count = 0;

static window_t* dragging = NULL;
static int32_t drag_off_x = 0, drag_off_y = 0;

window_t* window_create(int32_t x, int32_t y, int32_t h, int32_t w, const char* title, uint32_t bg) {
    if (window_count >= MAX_WINDOWS) return NULL;

    int slot = window_count;
    window_t* win = &windows[slot];
    win->x = x;
    win->y = y;
    win->w = w;
    win->h = h;
    win->bg = bg;
    win->used = true;
    win->draw_content = 0;
    win->content = 0;
    strncpy(win->title, title, sizeof(win->title) - 1);
    win->title[sizeof(win->title) - 1] = '\0';
    
    z_order[window_count] = slot;
    window_count++;

    return win;
}

int32_t window_client_x(const window_t* win) { return win->x + WIN_BORDER; }
int32_t window_client_y(const window_t* win) { return win->y + WIN_BORDER + WIN_TITLEBAR; }
int32_t window_client_w(const window_t* win) { return win->w - 2 * WIN_BORDER; }
int32_t window_client_h(const window_t* win) { return win->h - 2 * WIN_BORDER - WIN_TITLEBAR; }

void window_draw(window_t* win, bool focused) {
    if (!win || !win->used) return;

    draw_rect(win->x, win->y, win->w, win->h, COL_BORDER);
    draw_rect(win->x + WIN_BORDER, win->y + WIN_BORDER, win->w - 2 * WIN_BORDER, WIN_TITLEBAR, focused ? COL_TITLEBAR_TOP : COL_TITLEBAR); // Title bar

    // title bar text
    fbcon_draw_string_at(win->x + WIN_BORDER + 4, win->y + WIN_BORDER + 2, win->title, COL_TITLETEXT, focused ? COL_TITLEBAR_TOP : COL_TITLEBAR);

    // client part
    draw_rect(window_client_x(win), window_client_y(win), window_client_w(win), window_client_h(win), win->bg);

    if (win->draw_content) {
        win->draw_content(win);
    }
}

void wm_redraw(void) {
    cursor_hide();
    fill_screen(COL_DESKTOP);
    for (int i = 0; i < window_count; i++) {
        window_draw(&windows[z_order[i]], i == window_count - 1);
    }
    cursor_show();
}

static bool point_in_window(const window_t* w, int32_t px, int32_t py) {
    return px >= w->x && px < w->x + w->w && py >= w->y && py < w->y + w->h;
}

static bool point_in_titlebar(const window_t* w, int32_t px, int32_t py) {
    return px >= w->x && px < w->x + w->w &&
        py >= w->y + WIN_BORDER &&
        py <  w->y + WIN_BORDER + WIN_TITLEBAR;
}

static int window_index_at(int32_t px, int32_t py) {
    for (int i = window_count - 1; i >= 0; i--) {
        if (point_in_window(&windows[z_order[i]], px, py)) return i;
    }
    return -1;
}

static void window_raise(int zi) {
    if (zi < 0 || zi == window_count - 1) return; // on top already
    int slot = z_order[zi];
    for (int i = zi; i < window_count - 1; i++) {
        z_order[i] = z_order[i + 1];
    }
    z_order[window_count - 1] = slot;
}

void wm_handle_mouse(void) {
    static bool was_pressed = false;
    bool pressed = (mouse_buttons & 0x01) != 0;

    if (pressed && !was_pressed) {
        int zi = window_index_at(mouse_x, mouse_y);
        if (zi >= 0) {
            window_t* win = &windows[z_order[zi]];
            bool on_title = point_in_titlebar(win, mouse_x, mouse_y);
            window_raise(zi);
            if (on_title) {
                dragging = win;
                drag_off_x = mouse_x - win->x;
                drag_off_y = mouse_y - win->y;
            }
            wm_redraw();
        }
    } else if (!pressed && was_pressed) {
        dragging = NULL;
    } else if (pressed && dragging) {
        int32_t nx = mouse_x - drag_off_x;
        int32_t ny = mouse_y - drag_off_y;
        if (nx != dragging->x || ny != dragging->y) {
            dragging->x = nx;
            dragging->y = ny;
            wm_redraw();
        }
    }

    was_pressed = pressed;
}