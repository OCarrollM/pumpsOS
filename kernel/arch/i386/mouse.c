#include "mouse.h"
#include "ports.h"
#include "framebuffer.h"
#include "isr.h"
#include "pic.h"
#include "ps2.h"
#include <stdio.h>

#define PS2_DATA 0x60
#define PS2_STATUS 0x64 // r
#define PS2_CMD 0x64 // w
#define PS2_STAT_OUTPUT_FULL 0x01
#define PS2_STAT_INPUT_FULL 0x02
#define MOUSE_VECTOR 44

int32_t mouse_x = 0;
int32_t mouse_y = 0;
uint8_t mouse_buttons = 0;

// packet assembly
static uint8_t packet[3];
static int packet_index = 0;

static void mouse_process_packet(void) {
    uint8_t flags = packet[0];
 
    /* Discard packets with X/Y overflow set. */
    if (flags & 0xC0) return;
 
    int dx = packet[1];
    int dy = packet[2];
    if (flags & 0x10) dx |= 0xFFFFFF00;   /* sign-extend X */
    if (flags & 0x20) dy |= 0xFFFFFF00;   /* sign-extend Y */
 
    mouse_buttons = flags & 0x07;
 
    mouse_x += dx;
    mouse_y -= dy;                          /* screen Y is inverted */
 
    int32_t w = (int32_t)fb_get_width();
    int32_t h = (int32_t)fb_get_height();
    if (mouse_x < 0) mouse_x = 0;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_x >= w) mouse_x = w - 1;
    if (mouse_y >= h) mouse_y = h - 1;
}

static void mouse_irq_handler(struct registers* regs) {
    (void)regs;
    static int count = 0;
    count++;
    draw_rect(0, 0, 8, 8, (count & 1) ? 0xFF0000 : 0x00FF00);
    uint8_t data = inb(PS2_DATA);
 
    switch (packet_index) {
        case 0:
            if (!(data & 0x08)) { pic_send_eoi(IRQ_MOUSE); return; } /* resync */
            packet[0] = data;
            packet_index = 1;
            break;
        case 1:
            packet[1] = data;
            packet_index = 2;
            break;
        case 2:
            packet[2] = data;
            packet_index = 0;
            mouse_process_packet();
            break;
    }
 
    pic_send_eoi(IRQ_MOUSE);   /* dual EOI handled inside pic_send_eoi */
}

static void mouse_command(uint8_t cmd) {
    ps2_send_command(0xD4);
    ps2_write_data(cmd);
}

void mouse_init(void) {
    uint8_t config;

    ps2_send_command(PS2_CMD_ENABLE_PORT2);

    ps2_send_command(PS2_CMD_READ_CONFIG);
    config = ps2_read_data();
    config |= PS2_CONFIG_INT_PORT2;        /* enable IRQ12 */
    config &= ~PS2_CONFIG_CLOCK_PORT2;     /* enable the mouse clock */
    ps2_send_command(PS2_CMD_WRITE_CONFIG);
    ps2_write_data(config);


    mouse_command(0xFF);
    uint8_t r_ack  = ps2_read_data();      /* expect 0xFA */
    uint8_t r_test = ps2_read_data();      /* expect 0xAA */
    uint8_t r_id   = ps2_read_data();      /* expect 0x00 */

    mouse_command(0xF4);
    uint8_t r_en = ps2_read_data();        /* expect 0xFA */

    draw_rect(50,  0, 20, 20, (r_ack  == PS2_RESPONSE_ACK)          ? 0x00FF00 : 0xFF0000);
    draw_rect(75,  0, 20, 20, (r_test == PS2_RESPONSE_SELF_TEST_OK) ? 0x00FF00 : 0xFF0000);
    draw_rect(100, 0, 20, 20, (r_en   == PS2_RESPONSE_ACK)          ? 0x00FF00 : 0xFF0000);
    (void)r_id;

    isr_register_handler(MOUSE_VECTOR, mouse_irq_handler);
    pic_clear_mask(2);
    pic_clear_mask(IRQ_MOUSE);

    mouse_x = fb_get_width()  / 2;
    mouse_y = fb_get_height() / 2;
    packet_index = 0;
}