// Intel 82540EM gigabit NIC Driver (From intel / linux project)
#ifndef ARCH_I386_E1000_H
#define ARCH_I386_E1000_H

#include <stdint.h>
#include <stdbool.h>

bool e1000_init(void);
const uint8_t* e1000_mac(void);
// Transmit an ethernet frame
bool e1000_send(const void* data, uint16_t len);

#endif