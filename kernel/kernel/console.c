#include "console.h"
#include <kernel/tty.h>
#include "../arch/i386/keyboard.h"
#include <string.h>
#include "task.h"

vfs_node_t console_node;
vfs_node_t keyboard_node;


// Console writing: bytes will go to the vga terminal where the offset is ignored
static uint32_t console_write(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    (void)node;
    (void)offset;
    terminal_write((const char*)buffer, (size_t)size);
    return size;
}

// console read: Since we don't read from stdout we will return 0
static uint32_t console_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    (void)node;
    (void)offset;
    (void)size;
    (void)buffer;
    return 0;
}

// Keyboard read: get all chars without blocking
// Blocking will come later on
static uint32_t keyboard_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {

    (void)node;
    (void)offset;
    if (size == 0) return 0;
    uint32_t n = 0;
    while (n < size) {
        asm volatile("cli"); // auto check buffer
        char c = keyboard_getchar_nonblock();
        if (c != 0) {
            asm volatile("sti"); // we get a char so enable and store
            buffer[n++] = (uint8_t)c;

            char more; // lets get some more if possible
            while (n < size && (more = keyboard_getchar_nonblock()) != 0) {
                buffer[n++] = (uint8_t)more;
            }
            break;
        }
        
        // register and block if buffer empty
        task_t* self = task_current();
        keyboard_set_waiter(self);
  
        self->state = TASK_BLOCKED;

        asm volatile("sti");
        task_yield();
    }
    return n;
}

// Keyboard write: nothing written
static uint32_t keyboard_write(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    (void)node;
    (void)offset;
    (void)size;
    (void)buffer;
    return 0;
}

void console_init(void) {
    memset(&console_node, 0, sizeof(console_node));
    strncpy(console_node.name, "console", VFS_NAME_MAX - 1);
    console_node.flags = VFS_CHARDEVICE;
    console_node.read = console_read;
    console_node.write = console_write;

    memset(&keyboard_node, 0, sizeof(keyboard_node));
    strncpy(keyboard_node.name, "keyboard", VFS_NAME_MAX - 1);
    keyboard_node.flags = VFS_CHARDEVICE;
    keyboard_node.read = keyboard_read;
    keyboard_node.write = keyboard_write;
}