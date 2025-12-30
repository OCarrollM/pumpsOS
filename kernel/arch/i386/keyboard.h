#ifndef ARCH_I386_KEYBOARD_H
#define ARCH_I386_KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

// keycodes, this is a long one
typedef enum {
    KEY_NONE = 0,
    KEY_ESCAPE = 1,

    // FN Keys
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, 
    KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,

    // Num row
    KEY_BACKTICK,
    KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,
    KEY_MINUS,
    KEY_EQUALS,
    KEY_BACKSPACE,
    
    // Letters
    KEY_TAB,
    KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P,
    KEY_LBRACKET, KEY_RBRACKET,       
    KEY_BACKSLASH,      
    
    KEY_CAPSLOCK,
    KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L,
    KEY_SEMICOLON,      
    KEY_QUOTE,          
    KEY_ENTER,
    

    KEY_LSHIFT,
    KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M,
    KEY_COMMA,          
    KEY_PERIOD,         
    KEY_SLASH,          
    KEY_RSHIFT,
    
    KEY_LCTRL,
    KEY_LALT,
    KEY_SPACE,
    KEY_RALT,
    KEY_RCTRL,
    
    KEY_PRINTSCREEN,
    KEY_SCROLLLOCK,
    KEY_PAUSE,
    
    KEY_INSERT,
    KEY_DELETE,
    KEY_HOME,
    KEY_END,
    KEY_PAGEUP,
    KEY_PAGEDOWN,
    
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
    

    KEY_NUMLOCK,
    KEY_KP_SLASH,
    KEY_KP_ASTERISK,
    KEY_KP_MINUS,
    KEY_KP_PLUS,
    KEY_KP_ENTER,
    KEY_KP_PERIOD,
    KEY_KP_0, KEY_KP_1, KEY_KP_2, KEY_KP_3, KEY_KP_4,
    KEY_KP_5, KEY_KP_6, KEY_KP_7, KEY_KP_8, KEY_KP_9,
    
    KEY_LGUI,
    KEY_RGUI,
    KEY_MENU,
    
    KEY_MAX
} keycode_t;

typedef struct {
    keycode_t keycode;
    bool pressed;
    bool shift;
    bool ctrl;
    bool alt;
    bool capslock;
    bool numlock;
    char ascii;
} key_event_t;

void keyboard_init(void);
char keyboard_getchar(void);
char keyboard_getchar_nonblock(void);
bool keyboard_get_event(key_event_t* event);
bool keyboard_key_presse(keycode_t key);
bool keyboard_shift_pressed(void);
bool keyboard_crtl_pressed(void);
bool keyboard_alt_pressed(void);
bool keyboard_capslock_active(void);
void keyboard_readline(char* buffer, int max_length);

#endif