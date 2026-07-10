#include "ata.h"
#include "ports.h"

// primary ATA channel for IO ports
#define ATA_DATA 0x1F0
#define ATA_ERROR 0x1F1
#define ATA_SECCOUNT 0x1F2
#define ATA_LBA_LOW 0x1F3
#define ATA_LBA_MID 0x1F4
#define ATA_LBA_HIGH 0x1F5
#define ATA_DRIVE 0x1F6
#define ATA_STATUS 0x1F7 // Read
#define ATA_COMMAND 0x1F7 // Write

// status register bits
#define ATA_SR_BSY 0x80 // busy
#define ATA_SR_DRQ 0x08 // data request
#define ATA_SR_ERR 0x81 // error

#define ATA_CMD_READ_PIO 0x20

// wait for the drive to be ready first
static bool ata_poll(void) {
    for (int i = 0; i < 4; i++) { // 400ms delay
        (void)inb(ATA_STATUS);
    }

    // wait while busy
    uint32_t timeout = 100000;
    while (inb(ATA_STATUS) & ATA_SR_BSY) {
        if (--timeout == 0) return false;
    }

    uint8_t status = inb(ATA_STATUS);
    if (status & ATA_SR_ERR) return false; // error bit
    if (!(status & ATA_SR_DRQ)) return false; // No data ready
    return true;
}

bool ata_read_sector(uint32_t lba, uint8_t* buf) {
    // select the master drive (0) and top 4 bits
    outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));

    outb(ATA_SECCOUNT, 1);
    outb(ATA_LBA_LOW, (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));

    outb(ATA_COMMAND, ATA_CMD_READ_PIO); // read

    if (!ata_poll()) return false; // wait for data relay

    // read 256 words (a word is 4 bits on 32 bit systems so 512 here)
    uint16_t* words = (uint16_t*)buf;
    for (int i = 0; i < 256; i++) {
        words[i] = inw(ATA_DATA);
    }

    return true;
}