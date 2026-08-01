// Intel 82540EM gigabit NIC Driver (From intel / linux project)
#ifndef ARCH_I386_E1000_H
#define ARCH_I386_E1000_H

#include <stdint.h>
#include <stdbool.h>

#define E1000_FRAME_MAX 1518

bool e1000_init(void);
const uint8_t* e1000_mac(void);
// Transmit an ethernet frame
bool e1000_send(const void* data, uint16_t len);
bool e1000_poll_frame(uint8_t* out, uint16_t* len);

uint32_t e1000_rx_irqs(void);
uint32_t e1000_rx_frames(void);
uint32_t e1000_rdh(void);
uint32_t e1000_rdt(void);
uint32_t e1000_rx_cur(void);


#endif