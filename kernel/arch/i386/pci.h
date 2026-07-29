// Discovering the PCI Space, yippee
#ifndef ARCH_I386_PCI_H
#define ARCH_I386_PCI_H

#include <stdint.h>
#include <stdbool.h>

// Common configs, stolen from the www
#define PCI_VENDOR_ID 0x00
#define PCI_DEVICE_ID 0x02
#define PCI_COMMAND 0x04
#define PCI_STATUS 0x06
#define PCI_REVISION_ID 0x08
#define PCI_PROG_IF 0x09
#define PCI_SUBCLASS 0x0A
#define PCI_CLASS 0x0B
#define PCI_HEADER_TYPE 0x0E
#define PCI_BAR0 0x10
#define PCI_BAR1 0x14
#define PCI_BAR2 0x18
#define PCI_BAR3 0x1C
#define PCI_BAR4 0x20
#define PCI_BAR5 0x24
#define PCI_INTERRUPT_LINE 0x3C

// Command Register
#define PCI_CMD_IO_SPACE (1 << 0)
#define PCI_CMD_MEMORY_SPACE (1 << 1)
#define PCI_CMD_BUS_MASTER (1 << 2)

// Class codes
#define PCI_CLASS_STORAGE 0x01
#define PCI_CLASS_NETWORK 0x02
#define PCI_CLASS_DISPLAY 0x03
#define PCI_CLASS_MULTIMEDIA 0x04
#define PCI_CLASS_BRIDGE 0x06
#define PCI_CLASS_SERIAL_BUS 0x0C

typedef struct {
    uint8_t bus, device, function;
    uint16_t vendor_id, device_id;
    uint8_t class_code, subclass, prog_if;
    uint32_t bar[6];
    uint8_t irq_line;
    bool present;
} pci_device_t;

uint32_t pci_read32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off);
uint16_t pci_read16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off);
uint8_t pci_read8 (uint8_t bus, uint8_t dev, uint8_t func, uint8_t off);
void pci_write32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off, uint32_t val);
void pci_write16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off, uint16_t val);

// Scan bus and print
void pci_scan(void);
// Find first device
bool pci_find_by_class(uint8_t class_code, uint8_t subclass, pci_device_t* out);
// Find specific vendor pair
bool pci_find_by_id(uint16_t vendor, uint16_t device, pci_device_t* out);
// Turn on IO space
void pci_enable_device(const pci_device_t* dev);

#endif