#include "pci.h"
#include "ports.h"
#include <stdio.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

// build config space
static uint32_t pci_address(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off) {
    return (uint32_t)((1u << 31) | ((uint32_t)bus << 16) | ((uint32_t)dev << 11) | ((uint32_t)func << 8) | (off & 0xFC));
}

uint32_t pci_read32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off) {
    outl(PCI_CONFIG_ADDRESS, pci_address(bus, dev, func, off));
    return inl(PCI_CONFIG_DATA);
}

uint16_t pci_read16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off) {
    uint32_t v = pci_read32(bus, dev, func, off);
    return (uint16_t)((v >> ((off & 2) * 8)) & 0xFFFF);
}

uint8_t pci_read8(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off) {
    uint32_t v = pci_read32(bus, dev, func, off);
    return (uint8_t)((v >> ((off & 3) * 8)) & 0xFF);
}

void pci_write32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off, uint32_t val) {
    outl(PCI_CONFIG_ADDRESS, pci_address(bus, dev, func, off));
    outl(PCI_CONFIG_DATA, val);
}

void pci_write16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off, uint16_t val) {
    uint32_t old = pci_read32(bus, dev, func, off);
    uint32_t shift = (off & 2) * 8;
    uint32_t mask = 0xFFFFu << shift;
    uint32_t nv = (old & ~mask) | ((uint32_t)val << shift);
    pci_write32(bus, dev, func, off, nv);
}

// Fill pci_device_t from the config space
static bool pci_probe(uint8_t bus, uint8_t dev, uint8_t func, pci_device_t* out) {
    uint16_t vendor = pci_read16(bus, dev, func, PCI_VENDOR_ID);
    if (vendor == 0xFFFF) return false; // nothing

    out->bus = bus;
    out->device = dev;
    out->function = func;
    out->vendor_id = vendor;
    out->device_id = pci_read16(bus, dev, func, PCI_DEVICE_ID);
    out->class_code = pci_read8(bus, dev, func, PCI_CLASS);
    out->subclass = pci_read8(bus, dev, func, PCI_SUBCLASS);
    out->prog_if = pci_read8(bus, dev, func, PCI_PROG_IF);
    out->irq_line = pci_read8(bus, dev, func, PCI_INTERRUPT_LINE);
    for (int i = 0; i < 6; i++) {
        out->bar[i] = pci_read32(bus, dev, func, PCI_BAR0 + i * 4);
    }
    out->present = true;

    return true;
}

static const char* class_name(uint8_t c) {
    switch (c) {
        case PCI_CLASS_STORAGE: return "Mass Storage";
        case PCI_CLASS_NETWORK: return "Network";
        case PCI_CLASS_DISPLAY: return "Display";
        case PCI_CLASS_MULTIMEDIA: return "Multimedia";
        case PCI_CLASS_BRIDGE: return "Bridge";
        case PCI_CLASS_SERIAL_BUS: return "Serial Bus";
        default: return "Other";
    }
}

void pci_scan(void) {
    pci_device_t d;
    printf("Scanning PCI Bus...\n");

    for (int bus = 0; bus < 256; bus++) {
        for (int dev = 0; dev < 32; dev++) {
            if (!pci_probe((uint8_t)bus, (uint8_t)dev, 0, &d)) continue;

            printf("%d:%d.0 %x:%x %s (class %x.%x) irq=%d\n", bus, dev, d.vendor_id, d.device_id, class_name(d.class_code), d.class_code, d.subclass, d.irq_line);

            uint8_t header = pci_read8((uint8_t)bus, (uint8_t)dev, 0, PCI_HEADER_TYPE);
            if (header & 0x80) {
                for (int func = 1; func < 8; func++) {
                    if (pci_probe((uint8_t)bus, (uint8_t)dev, (uint8_t)func, &d)) {
                        printf(" %d:%d.%d %x:%x %s (class %x.%x) irq=%d\n", bus, dev, func, d.vendor_id, d.device_id, class_name(d.class_code), d.class_code, d.subclass, d.irq_line);
                    }
                }
            }
        }
    }
    printf("PCI Scan Complete\n");
}

bool pci_find_by_class(uint8_t class_code, uint8_t subclass, pci_device_t* out) {
    for (int bus = 0; bus < 256; bus++) {
        for (int dev = 0; dev < 32; dev++) {
            for (int func = 0; func < 8; func++) {
                if (!pci_probe((uint8_t)bus, (uint8_t)dev, (uint8_t)func, out)) {
                    continue;
                }
                if(out->class_code == class_code && out->subclass == subclass) {
                    return true;
                }
                if (func == 0) {
                    uint8_t h = pci_read8((uint8_t)bus, (uint8_t)dev, 0, PCI_HEADER_TYPE);
                    if (!(h & 0x80)) break;
                }
            }
        }
    }
    out->present = false;
    return false;
}

bool pci_find_by_id(uint16_t vendor, uint16_t device, pci_device_t* out) {
    for (int bus = 0; bus < 256; bus++) {
        for (int dev = 0; dev < 32; dev++) {
            for (int func = 0; func < 8; func++) {
                if (!pci_probe((uint8_t)bus, (uint8_t)dev, (uint8_t)func, out)) {
                    continue;
                }
                if (out->vendor_id == vendor && out->device_id == device) {
                    return true;
                }
                if (func == 0) {
                    uint8_t h = pci_read8((uint8_t)bus, (uint8_t)dev, 0, PCI_HEADER_TYPE);
                    if (!(h & 0x80)) break;
                }
            }
        }
    }
    out->present = false;
    return false;
}

void pci_enable_device(const pci_device_t* dev) {
    uint16_t cmd = pci_read16(dev->bus, dev->device, dev->function, PCI_COMMAND);
    cmd |= PCI_CMD_IO_SPACE | PCI_CMD_MEMORY_SPACE | PCI_CMD_BUS_MASTER;
    pci_write16(dev->bus, dev->device, dev->function, PCI_COMMAND, cmd);
}