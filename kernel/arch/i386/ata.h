#ifndef ARCH_I386_ATA_H
#define ARCH_I386_ATA_H

#include <stdint.h>
#include <stdbool.h>

bool ata_read_sector(uint32_t lba, uint8_t* buf); // 512 byte read

#endif

// Adapted via various file system books