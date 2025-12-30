#ifndef ARCH_I386_PS2_H
#define ARCH_I386_PS2_H

#include <stdint.h>
#include <stdbool.h>

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_COMMAND_PORT 0x64

#define PS2_STATUS_OUTPUT_FULL (1 << 0)
#define PS2_STATUS_INPUT_FULL (1 << 1)
#define PS2_STATUS_SYSTEM_FLAG (1 << 2)
#define PS2_STATUS_CMD_DATA (1 << 3)
#define PS2_STATUS_TIMEOUT_ERR (1 << 6)
#define PS2_STATUS_PARITY_ERR (1 << 7)

#define PS2_CMD_READ_CONFIG 0x20
#define PS2_CMD_WRITE_CONFIG 0x60
#define PS2_CMD_DISABLE_PORT2 0xA7
#define PS2_CMD_ENABLE_PORT2 0xA8
#define PS2_CMD_TEST_PORT2 0xA9
#define PS2_CMD_SELF_TEST 0xAA
#define PS2_CMD_TEST_PORT1 0xAB
#define PS2_CMD_DISABLE_PORT1 0xAD
#define PS2_CMD_ENABLE_PORT1 0xAE
#define PS2_CMD_READ_OUTPUT 0xD0
#define PS2_CMD_WRITE_OUTPUT 0xD1

#define PS2_CONFIG_INT_PORT1 (1 << 0)
#define PS2_CONFIG_INT_PORT2 (1 << 1)
#define PS2_CONFIG_SYSTEM_FLAG (1 << 2)
#define PS2_CONFIG_CLOCK_PORT1 (1 << 4)
#define PS2_CONFIG_CLOCK_PORT2 (1 << 5)
#define PS2_CONFIG_TRANSLATION (1 << 6)

#define PS2_RESPONSE_ACK 0xFA
#define PS2_RESPONSE_RESEND 0xFE
#define PS2_RESPONSE_SELF_TEST_OK 0xAA
#define PS2_RESPONSE_PORT_TEST_OK 0x00

void ps2_init(void);
uint8_t ps2_read_data(void);
bool ps2_read_data_nonblock(uint8_t* data);
void ps2_write_data(uint8_t data);
void ps2_send_command(uint8_t command);
bool ps2_wait_output(void);
bool ps2_wait_input(void);
void ps2_flush(void);

#endif