//==========================================
/// @file       pci_driver.hpp
/// @brief      pci related functions
//==========================================

#pragma once

#ifndef __PCI_DRIVER_HPP__
#define __PCI_DRIVER_HPP__

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

#define PCI_CREATE_CONFIG_ADDRESS(bus, dev, func, off) ((1 << 31) | ((bus) << 16) | ((dev) << 11) | ((func) << 8) | ((off) & 0xFC))

#define PCI_GET_BAR_OFFSET(index)           (0x10 + ((index) * 4))
#define PCI_VENDOR_DEVICE_ID_INVALID        0xFFFFFFFF
#define PCI_BAR_ADDRESS_MASKED(address)     ((address) & 0xFFFFFFF0)

#define PCI_UNKNOWN -1

#define PCI_CMD_MMIO            (1 << 1)
#define PCI_CMD_BUS_MASTERING   (1 << 2)

#include "common.hpp"
#include "vector.hpp"

struct pci_device_info_t {
    union {
        uint32_t vendor_device_id;
        struct {
            uint16_t vendor_id;
            uint16_t device_id;
        };
    };

    union {
        uint32_t class_info;
        struct {
            uint8_t revision_id;
            uint8_t prog_if;
            uint8_t sub_class;
            uint8_t class_code;
        };
    };

    uint32_t bar0_address;
    uint32_t bar1_address;
    uint32_t bar2_address;
    uint32_t bar3_address;
    uint32_t bar4_address;
    uint32_t bar5_address;

    uint32_t bus;
    uint32_t device;
    uint32_t function;
};

enum class pci_device_request_mode_t {
    CLASS_INFO,
    VENDOR_DEVICE_ID
};

struct pci_device_request_t {
    union {
        uint32_t class_info = (uint32_t)PCI_UNKNOWN;
        struct {
            uint8_t revision_id;
            uint8_t prog_if;
            uint8_t sub_class;
            uint8_t class_code;
        };
    };

    uint16_t vendor_id = (uint16_t)PCI_UNKNOWN;
    uint16_t device_id = (uint16_t)PCI_UNKNOWN;

    pci_device_request_mode_t mode = pci_device_request_mode_t::CLASS_INFO;
    bool found;
    size_t pci_device_index;
};

uint32_t pci_config_read(uint16_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_config_write(uint16_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);

const char* pci_get_class_description(pci_device_info_t* device);

bool pci_enumerate_devices(vector<pci_device_info_t>* list);
bool pci_find_devices(vector<pci_device_info_t>* list, vector<pci_device_request_t>* request_list);

#endif // __PCI_DRIVER_HPP__