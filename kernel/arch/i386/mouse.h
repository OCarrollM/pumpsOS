// PS/2 mouse driver which will track the cursor and buttons
#ifndef ARCH_I386_MOUSE_H
#define ARCH_I386_MOUSE_H

#include <stdint.h>


extern int32_t mouse_x;
extern int32_t mouse_y;
extern uint8_t mouse_buttons;

void mouse_init(void);


#endif