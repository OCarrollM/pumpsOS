#include "keyboard.h"
#include "ps2.h"
#include "pic.h"
#include "isr.h"
#include "ports.h"
#include <stdio.h>
#include <stdbool.h>

static const keycode_t scancode_to_keycode[128] = {
    /* 0x00 */ KEY_NONE,
    /* 0x01 */ KEY_ESCAPE,
    /* 0x02 */ KEY_1,
    /* 0x03 */ KEY_2,
    /* 0x04 */ KEY_3,
    /* 0x05 */ KEY_4,
    /* 0x06 */ KEY_5,
    /* 0x07 */ KEY_6,
    /* 0x08 */ KEY_7,
    /* 0x09 */ KEY_8,
    /* 0x0A */ KEY_9,
    /* 0x0B */ KEY_0,
    /* 0x0C */ KEY_MINUS,
    /* 0x0D */ KEY_EQUALS,
    /* 0x0E */ KEY_BACKSPACE,
    /* 0x0F */ KEY_TAB,
    /* 0x10 */ KEY_Q,
    /* 0x11 */ KEY_W,
    /* 0x12 */ KEY_E,
    /* 0x13 */ KEY_R,
    /* 0x14 */ KEY_T,
    /* 0x15 */ KEY_Y,
    /* 0x16 */ KEY_U,
    /* 0x17 */ KEY_I,
    /* 0x18 */ KEY_O,
    /* 0x19 */ KEY_P,
    /* 0x1A */ KEY_LBRACKET,
    /* 0x1B */ KEY_RBRACKET,
    /* 0x1C */ KEY_ENTER,
    /* 0x1D */ KEY_LCTRL,
    /* 0x1E */ KEY_A,
    /* 0x1F */ KEY_S,
    /* 0x20 */ KEY_D,
    /* 0x21 */ KEY_F,
    /* 0x22 */ KEY_G,
    /* 0x23 */ KEY_H,
    /* 0x24 */ KEY_J,
    /* 0x25 */ KEY_K,
    /* 0x26 */ KEY_L,
    /* 0x27 */ KEY_SEMICOLON,
    /* 0x28 */ KEY_QUOTE,
    /* 0x29 */ KEY_BACKTICK,
    /* 0x2A */ KEY_LSHIFT,
    /* 0x2B */ KEY_BACKSLASH,
    /* 0x2C */ KEY_Z,
    /* 0x2D */ KEY_X,
    /* 0x2E */ KEY_C,
    /* 0x2F */ KEY_V,
    /* 0x30 */ KEY_B,
    /* 0x31 */ KEY_N,
    /* 0x32 */ KEY_M,
    /* 0x33 */ KEY_COMMA,
    /* 0x34 */ KEY_PERIOD,
    /* 0x35 */ KEY_SLASH,
    /* 0x36 */ KEY_RSHIFT,
    /* 0x37 */ KEY_KP_ASTERISK,
    /* 0x38 */ KEY_LALT,
    /* 0x39 */ KEY_SPACE,
    /* 0x3A */ KEY_CAPSLOCK,
    /* 0x3B */ KEY_F1,
    /* 0x3C */ KEY_F2,
    /* 0x3D */ KEY_F3,
    /* 0x3E */ KEY_F4,
    /* 0x3F */ KEY_F5,
    /* 0x40 */ KEY_F6,
    /* 0x41 */ KEY_F7,
    /* 0x42 */ KEY_F8,
    /* 0x43 */ KEY_F9,
    /* 0x44 */ KEY_F10,
    /* 0x45 */ KEY_NUMLOCK,
    /* 0x46 */ KEY_SCROLLLOCK,
    /* 0x47 */ KEY_KP_7,
    /* 0x48 */ KEY_KP_8,
    /* 0x49 */ KEY_KP_9,
    /* 0x4A */ KEY_KP_MINUS,
    /* 0x4B */ KEY_KP_4,
    /* 0x4C */ KEY_KP_5,
    /* 0x4D */ KEY_KP_6,
    /* 0x4E */ KEY_KP_PLUS,
    /* 0x4F */ KEY_KP_1,
    /* 0x50 */ KEY_KP_2,
    /* 0x51 */ KEY_KP_3,
    /* 0x52 */ KEY_KP_0,
    /* 0x53 */ KEY_KP_PERIOD,
    /* 0x54 */ KEY_NONE,
    /* 0x55 */ KEY_NONE,
    /* 0x56 */ KEY_NONE,
    /* 0x57 */ KEY_F11,
    /* 0x58 */ KEY_F12,
    KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE,
    KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE,
    KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE,
    KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE,
    KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE
};

static const keycode_t extended_scancode_to_keycode[128] = {
    /* 0x00-0x0F */ 
    KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE,
    KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE,
    /* 0x10 */ KEY_NONE,        /* (multimedia prev track) */
    /* 0x11-0x18 */
    KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE,
    /* 0x19 */ KEY_NONE,        /* (multimedia next track) */
    /* 0x1A-0x1B */
    KEY_NONE, KEY_NONE,
    /* 0x1C */ KEY_KP_ENTER,    /* Keypad Enter */
    /* 0x1D */ KEY_RCTRL,       /* Right Control */
    /* 0x1E-0x1F */
    KEY_NONE, KEY_NONE,
    /* 0x20-0x2F */
    KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE,
    KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE,
    /* 0x30-0x34 */
    KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE,
    /* 0x35 */ KEY_KP_SLASH,    /* Keypad / */
    /* 0x36-0x37 */
    KEY_NONE, KEY_NONE,         /* 0x37 = Print Screen (partial) */
    /* 0x38 */ KEY_RALT,        /* Right Alt */
    /* 0x39-0x46 */
    KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE,
    KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE,
    /* 0x47 */ KEY_HOME,        /* Home */
    /* 0x48 */ KEY_UP,          /* Up Arrow */
    /* 0x49 */ KEY_PAGEUP,      /* Page Up */
    /* 0x4A */ KEY_NONE,
    /* 0x4B */ KEY_LEFT,        /* Left Arrow */
    /* 0x4C */ KEY_NONE,
    /* 0x4D */ KEY_RIGHT,       /* Right Arrow */
    /* 0x4E */ KEY_NONE,
    /* 0x4F */ KEY_END,         /* End */
    /* 0x50 */ KEY_DOWN,        /* Down Arrow */
    /* 0x51 */ KEY_PAGEDOWN,    /* Page Down */
    /* 0x52 */ KEY_INSERT,      /* Insert */
    /* 0x53 */ KEY_DELETE,      /* Delete */
    /* 0x54-0x5A */
    KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE,
    /* 0x5B */ KEY_LGUI,        /* Left GUI (Windows key) */
    /* 0x5C */ KEY_RGUI,        /* Right GUI */
    /* 0x5D */ KEY_MENU,        /* Menu key */
    /* 0x5E-0x7F: Rest unused */
    KEY_NONE, KEY_NONE,
    KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE,
    KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE,
    KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE,
    KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE,
};

static const char keymap_normal[KEY_MAX] = {
    [KEY_NONE] = 0,
    [KEY_ESCAPE] = 27,      /* ESC character */
    [KEY_BACKTICK] = '`',
    [KEY_1] = '1', [KEY_2] = '2', [KEY_3] = '3', [KEY_4] = '4', [KEY_5] = '5',
    [KEY_6] = '6', [KEY_7] = '7', [KEY_8] = '8', [KEY_9] = '9', [KEY_0] = '0',
    [KEY_MINUS] = '-',
    [KEY_EQUALS] = '=',
    [KEY_BACKSPACE] = '\b',
    [KEY_TAB] = '\t',
    [KEY_Q] = 'q', [KEY_W] = 'w', [KEY_E] = 'e', [KEY_R] = 'r', [KEY_T] = 't',
    [KEY_Y] = 'y', [KEY_U] = 'u', [KEY_I] = 'i', [KEY_O] = 'o', [KEY_P] = 'p',
    [KEY_LBRACKET] = '[',
    [KEY_RBRACKET] = ']',
    [KEY_BACKSLASH] = '\\',
    [KEY_A] = 'a', [KEY_S] = 's', [KEY_D] = 'd', [KEY_F] = 'f', [KEY_G] = 'g',
    [KEY_H] = 'h', [KEY_J] = 'j', [KEY_K] = 'k', [KEY_L] = 'l',
    [KEY_SEMICOLON] = ';',
    [KEY_QUOTE] = '\'',
    [KEY_ENTER] = '\n',
    [KEY_Z] = 'z', [KEY_X] = 'x', [KEY_C] = 'c', [KEY_V] = 'v', [KEY_B] = 'b',
    [KEY_N] = 'n', [KEY_M] = 'm',
    [KEY_COMMA] = ',',
    [KEY_PERIOD] = '.',
    [KEY_SLASH] = '/',
    [KEY_SPACE] = ' ',
    /* Keypad */
    [KEY_KP_SLASH] = '/',
    [KEY_KP_ASTERISK] = '*',
    [KEY_KP_MINUS] = '-',
    [KEY_KP_PLUS] = '+',
    [KEY_KP_ENTER] = '\n',
    [KEY_KP_PERIOD] = '.',
    [KEY_KP_0] = '0', [KEY_KP_1] = '1', [KEY_KP_2] = '2', [KEY_KP_3] = '3',
    [KEY_KP_4] = '4', [KEY_KP_5] = '5', [KEY_KP_6] = '6', [KEY_KP_7] = '7',
    [KEY_KP_8] = '8', [KEY_KP_9] = '9',
};

static const char keymap_shifted[KEY_MAX] = {
    [KEY_NONE] = 0,
    [KEY_ESCAPE] = 27,
    [KEY_BACKTICK] = '~',
    [KEY_1] = '!', [KEY_2] = '@', [KEY_3] = '#', [KEY_4] = '$', [KEY_5] = '%',
    [KEY_6] = '^', [KEY_7] = '&', [KEY_8] = '*', [KEY_9] = '(', [KEY_0] = ')',
    [KEY_MINUS] = '_',
    [KEY_EQUALS] = '+',
    [KEY_BACKSPACE] = '\b',
    [KEY_TAB] = '\t',
    [KEY_Q] = 'Q', [KEY_W] = 'W', [KEY_E] = 'E', [KEY_R] = 'R', [KEY_T] = 'T',
    [KEY_Y] = 'Y', [KEY_U] = 'U', [KEY_I] = 'I', [KEY_O] = 'O', [KEY_P] = 'P',
    [KEY_LBRACKET] = '{',
    [KEY_RBRACKET] = '}',
    [KEY_BACKSLASH] = '|',
    [KEY_A] = 'A', [KEY_S] = 'S', [KEY_D] = 'D', [KEY_F] = 'F', [KEY_G] = 'G',
    [KEY_H] = 'H', [KEY_J] = 'J', [KEY_K] = 'K', [KEY_L] = 'L',
    [KEY_SEMICOLON] = ':',
    [KEY_QUOTE] = '"',
    [KEY_ENTER] = '\n',
    [KEY_Z] = 'Z', [KEY_X] = 'X', [KEY_C] = 'C', [KEY_V] = 'V', [KEY_B] = 'B',
    [KEY_N] = 'N', [KEY_M] = 'M',
    [KEY_COMMA] = '<',
    [KEY_PERIOD] = '>',
    [KEY_SLASH] = '?',
    [KEY_SPACE] = ' ',
    /* Keypad (same as unshifted) */
    [KEY_KP_SLASH] = '/',
    [KEY_KP_ASTERISK] = '*',
    [KEY_KP_MINUS] = '-',
    [KEY_KP_PLUS] = '+',
    [KEY_KP_ENTER] = '\n',
    [KEY_KP_PERIOD] = '.',
    [KEY_KP_0] = '0', [KEY_KP_1] = '1', [KEY_KP_2] = '2', [KEY_KP_3] = '3',
    [KEY_KP_4] = '4', [KEY_KP_5] = '5', [KEY_KP_6] = '6', [KEY_KP_7] = '7',
    [KEY_KP_8] = '8', [KEY_KP_9] = '9',
};

static bool key_states[KEY_MAX];
static bool capslock_active = false;
static bool numlock_active = true;
static bool scrolllock_active = false;
static bool extended_scancode = false;

#define INPUT_BUFFER_SIZE 256
static char input_buffer[INPUT_BUFFER_SIZE];
static volatile int buffer_head = 0;
static volatile int buffer_tail = 0;

static bool is_letter_key(keycode_t key) {
    switch (key) {
        case KEY_A: case KEY_B: case KEY_C: case KEY_D: case KEY_E:
        case KEY_F: case KEY_G: case KEY_H: case KEY_I: case KEY_J:
        case KEY_K: case KEY_L: case KEY_M: case KEY_N: case KEY_O:
        case KEY_P: case KEY_Q: case KEY_R: case KEY_S: case KEY_T:
        case KEY_U: case KEY_V: case KEY_W: case KEY_X: case KEY_Y:
        case KEY_Z:
            return true;
        default:
            return false;
    }
}

static char keycode_to_ascii(keycode_t key) {
    if(key >= KEY_MAX) {
        return 0;
    }

    bool shift = key_states[KEY_LSHIFT] || key_states[KEY_RSHIFT];
    bool use_shifted = shift;

    if(is_letter_key(key)) {
        if(capslock_active) {
            use_shifted = !use_shifted;
        }
    }
    return use_shifted ? keymap_shifted[key] : keymap_normal[key];
}

static void buffer_put(char c) {
    int next_head = (buffer_head + 1) % INPUT_BUFFER_SIZE;

    if(next_head == buffer_tail) {
        return;
    }

    input_buffer[buffer_head] = c;
    buffer_head = next_head;
}

static char buffer_get(void) {
    if(buffer_head == buffer_tail) {
        return 0;
    }

    char c = input_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % INPUT_BUFFER_SIZE;
    return c;
}

static bool buffer_has_data(void) {
    return buffer_head != buffer_tail;
}

static void keyboard_irq_handler(struct registers* regs) {
    (void) regs;

    uint8_t scancode = inb(PS2_DATA_PORT);

    if(scancode == 0xE0) {
        extended_scancode = true;
        pic_send_eoi(IRQ_KEYBOARD);
        return;
    }
    if(scancode == 0xE1) {
        pic_send_eoi(IRQ_KEYBOARD);
        return;
    }

    bool released = (scancode & 0x80) != 0;
    scancode &= 0x7F;

    keycode_t keycode;
    if(extended_scancode) {
        keycode = extended_scancode_to_keycode[scancode];
        extended_scancode = false;
    } else {
        keycode = scancode_to_keycode[scancode];
    }

    if(keycode != KEY_NONE && keycode < KEY_MAX) {
        key_states[keycode] = !released;
    }

    if(!released) {
        switch(keycode) {
            case KEY_CAPSLOCK:
                capslock_active = !capslock_active;
                break;
            case KEY_NUMLOCK:
                numlock_active = !numlock_active;
                break;
            case KEY_SCROLLLOCK:
                scrolllock_active = !scrolllock_active;
                break;
            default:
                break;
        }
    }

    if(!released) {
        char ascii = keycode_to_ascii(keycode);
        if(ascii != 0) {
            buffer_put(ascii);
        }
    }

    pic_send_eoi(IRQ_KEYBOARD);
}

void keyboard_init(void) {
    for(int i = 0; i < KEY_MAX; i++) {
        key_states[i] = false;
    }
    buffer_head = 0;
    buffer_tail = 0;

    ps2_init();

    isr_register_handler(33, keyboard_irq_handler);

    uint8_t mask_before = inb(0x21);
    pic_clear_mask(IRQ_KEYBOARD);
    uint8_t mask_after = inb(0x21);
    printf("PIC1 mask: %d -> %d\n", mask_before, mask_after);

    printf("Sending keyboard enable cmd\n");
    ps2_write_data(0xF4);

    if(ps2_wait_output()) {
        uint8_t response = inb(PS2_DATA_PORT);
        printf("Keyboard response: %d (250=ACK)\n", response);
    } else {
        printf("No response from keyboard...\n");
    }

    printf("Keyboard enabled\n");
}

char keyboard_getchar(void) {
    while(!buffer_has_data()) {
        asm volatile("hlt");
    }
    return buffer_get();
}

char keyboard_getchar_nonblock(void) {
    return buffer_get();
}

bool keyboard_get_event(key_event_t* event) {
    (void)event;
    return false;
}

bool keyboard_key_pressed(keycode_t key) {
    if(key >= KEY_MAX) {
        return false;
    }
    return key_states[key];
}

bool keyboard_shift_pressed(void) {
    return key_states[KEY_LSHIFT] || key_states[KEY_RSHIFT];
}

bool keyboard_crtl_pressed(void) {
    return key_states[KEY_LCTRL] || key_states[KEY_RCTRL];
}

bool keyboard_alt_pressed(void) {
    return key_states[KEY_LALT] || key_states[KEY_RALT];
}

bool keyboard_capslock_active(void) {
    return capslock_active;
}

void keyboard_readline(char* buffer, int max_length) {
    int pos = 0;

    while(pos < max_length - 1) {
        char c = keyboard_getchar();

        if(c == '\n') {
            buffer[pos] = '\0';
            printf("\n");
            return;
        } else if(c == '\b') {
            if(pos > 0) {
                pos--;
                printf("\b \b");
            }
        } else if(c >= 32 && c < 127) {
            buffer[pos++] = c;
            printf("%c", c);
        }
    }

    buffer[pos] = '\0';
}