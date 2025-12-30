#include "ps2.h"
#include "ports.h"
#include <stdio.h>

#define PS2_TIMEOUT  100000

bool ps2_wait_output(void) {
    int timeout = PS2_TIMEOUT;

    while(timeout--) {
        uint8_t status = inb(PS2_STATUS_PORT);
        if(status & PS2_STATUS_OUTPUT_FULL) {
            return true;
        }
    }
    return false;
}

bool ps2_wait_input(void) {
    int timeout = PS2_TIMEOUT;

    while(timeout--) {
        uint8_t status = inb(PS2_STATUS_PORT);
        if(!(status & PS2_STATUS_INPUT_FULL)) {
            return true;
        }
    }
    return false;
}

void ps2_flush(void) {
    while(inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) {
        inb(PS2_DATA_PORT);
        io_wait();
    }
}

uint8_t ps2_read_data(void) {
    ps2_wait_output();
    return inb(PS2_DATA_PORT);
}

bool ps2_read_data_nonblock(uint8_t* data) {
    if(inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) {
        *data = inb(PS2_DATA_PORT);
        return true;
    }
    return false;
}

void ps2_write_data(uint8_t data) {
    ps2_wait_input();
    outb(PS2_DATA_PORT, data);
}

void ps2_send_command(uint8_t command) {
    ps2_wait_input();
    outb(PS2_COMMAND_PORT, command);
}

void ps2_init(void) {
    uint8_t config;

    ps2_send_command(PS2_CMD_DISABLE_PORT1);
    ps2_send_command(PS2_CMD_DISABLE_PORT2);

    ps2_flush();

    ps2_send_command(PS2_CMD_READ_CONFIG);
    config = ps2_read_data();

    config |= PS2_CONFIG_INT_PORT1;
    config &= ~PS2_CONFIG_INT_PORT2;
    config |= PS2_CONFIG_TRANSLATION;

    ps2_send_command(PS2_CMD_WRITE_CONFIG);
    ps2_write_data(config);

    ps2_send_command(PS2_CMD_ENABLE_PORT1);

    printf("PS/2 Controller Enabled\n");
}