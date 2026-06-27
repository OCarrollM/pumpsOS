#ifndef KERNEL_CONSOLE_H
#define KERNEL_CONSOLE_H

#include "vfs.h"

extern vfs_node_t console_node;
extern vfs_node_t keyboard_node;

void console_init(void);

#endif