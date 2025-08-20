//==========================================
/// @file       pcie.hpp
/// @brief      pice generic driver
//==========================================

#pragma once

#ifndef __DRIVERS_PCIE_HPP__
#define __DRIVERS_PCIE_HPP__

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

#define PCI_CREATE_CONFIG_ADDRESS(bus, dev, func, off) ((1 << 31) | ((bus) << 16) | ((dev) << 11) | ((func) << 8) | ((off) & 0xFC))

#define PCI_GET_BAR_OFFSET(index)           (0x10 + ((index) * 4))
#define PCI_VENDOR_DEVICE_ID_INVALID        MAX_UINT32
#define PCI_BAR_ADDRESS_MASKED(address)     ((address) & 0xFFFFFFF0)

#define PCI_UNKNOWN -1

#define PCI_CMD_MMIO            (1 << 1)
#define PCI_CMD_BUS_MASTERING   (1 << 2)

#include "common.hpp"
#include "utils/vector.hpp"

union pci_vendor_device_id_t {
    uint32_t raw;
    struct {
        uint16_t vendor_id;
        uint16_t device_id;
    };
};

union pci_class_info_t {
    uint32_t raw;
    struct {
        uint8_t revision_id;
        uint8_t prog_if;
        uint8_t sub_class;
        uint8_t class_code;
    };
};

struct pci_device_t {
    pci_vendor_device_id_t vendor_device_id;
    pci_class_info_t class_info;

    uint32_t bus;
    uint32_t device;
    uint32_t function;
};

uint32_t pci_config_read(const pci_device_t* p_device, uint32_t offset);
void pci_config_write(const pci_device_t* p_device, uint32_t offset, uint32_t value);

const char* pci_get_class_description(const pci_device_t* p_device);

uint32_t pci_read_bar(const pci_device_t* p_device, uint32_t bar);

bool pci_enumerate_devices(linked_list<pci_device_t>* p_list);
pci_device_t* pci_find_device(linked_list<pci_device_t>* p_list, const pci_vendor_device_id_t* p_vendor_device_id_target);
pci_device_t* pci_find_device(linked_list<pci_device_t>* p_list, const pci_class_info_t* p_class_info_target);

#endif // __DRIVERS_PCIE_HPP__