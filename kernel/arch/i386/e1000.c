// Intels driver
#include "e1000.h"
#include "pci.h"
#include "../kernel/vmm.h"
#include "../kernel/vmem.h"
#include <stdio.h>

#define E1000_VENDOR 0x8086
#define E1000_DEVICE 0x100E

// Register the offsets
#define E1000_CTRL 0x0000
#define E1000_STATUS 0x0008
#define E1000_EERD 0x0014
#define E1000_RAL 0x5400
#define E1000_RAH 0x5404

// EERD Bits
#define EERD_START (1 << 0)
#define EERD_DONE (1 << 4)
#define EERD_ADDR_SHIFT 8
#define EERD_DATA_SHIFT 16

// cards register space
#define E1000_MMIO_SIZE 0x20000

static uint32_t mmio = 0;
static uint8_t mac[6];
static bool eeprom_present = false;

// register access
static inline uint32_t e1000_read(uint32_t reg) {
    return *(volatile uint32_t*)(mmio + reg);
}
static inline void e1000_write(uint32_t reg, uint32_t value) {
    *(volatile uint32_t*)(mmio + reg) = value;
}

static void e1000_detect_eeprom(void) {
    e1000_write(E1000_EERD, EERD_START);
    for (int i = 0; i < 1000; i++) {
        if (e1000_read(E1000_EERD) & EERD_DONE) {
            eeprom_present = true;
            return;
        }
    }
    eeprom_present = false;
}

static uint16_t eeprom_read(uint8_t addr) {
    e1000_write(E1000_EERD, ((uint32_t)addr << EERD_ADDR_SHIFT) | EERD_START);
    uint32_t val;
    int timeout = 100000;
    do {
        val = e1000_read(E1000_EERD);
    } while (!(val & EERD_DONE) && --timeout);
    return (uint16_t)(val >> EERD_DATA_SHIFT);
}
 // Read mac
static void e1000_read_mac(void) {
    if (eeprom_present) {
        uint16_t w;
        w = eeprom_read(0); mac[0] = w & 0xFF; mac[1] = w >> 8;
        w = eeprom_read(1); mac[2] = w & 0xFF; mac[3] = w >> 8;
        w = eeprom_read(2); mac[4] = w & 0xFF; mac[5] = w >> 8;
    } else {
        uint32_t ral = e1000_read(E1000_RAL);
        uint32_t rah = e1000_read(E1000_RAH);
        mac[0] = (ral) & 0xFF;
        mac[1] = (ral >> 8) & 0xFF;
        mac[2] = (ral >> 16) & 0xFF;
        mac[3] = (ral >> 24) & 0xFF;
        mac[4] = (rah) & 0xFF;
        mac[5] = (rah >> 8) & 0xFF;
    }
}

const uint8_t* e1000_mac(void) { return mac; }

bool e1000_init(void) {
    pci_device_t dev;
    if (!pci_find_by_id(E1000_VENDOR, E1000_DEVICE, &dev)) {
        printf("No E1000 card found\n");
        return false;
    }

    printf("E1000 card found at %d:%d.%d irq=%d\n", dev.bus, dev.device, dev.function, dev.irq_line);

    // let card do I/O and respond to mem reads
    pci_enable_device(&dev);

    uint32_t bar0 = dev.bar[0] & 0xFFFFFFF0u;
    printf("E1000 BAR0 phys = 0x%x\n", bar0);

    // map register space into kernel half
    uint32_t pages = E1000_MMIO_SIZE / 0x1000;
    for (uint32_t i = 0; i < pages; i++) {
        if (!vmm_map_page(VMEM_E1000_MMIO + i * 0x1000, bar0 + i * 0x1000, PTE_PRESENT | PTE_WRITABLE)) {
            printf("E1000 failed to map registers\n");
            return false;
        }
    }
    mmio = VMEM_E1000_MMIO;

    // for sanity AHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
    uint32_t status = e1000_read(E1000_STATUS);
    printf("E1000 Status = 0x%x\n", status);

    e1000_detect_eeprom();
    printf("E1000 eeprom %s\n", eeprom_present ? "present" : "absent");

    e1000_read_mac();
    printf("E1000 mac = %02x:%02x:%02x:%02x:%02x:%02x\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return true;
}