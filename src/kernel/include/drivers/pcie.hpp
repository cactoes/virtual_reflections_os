//==========================================
/// @file       pcie.hpp
/// @brief      pice generic driver
//==========================================

#pragma once

#ifndef __DRIVERS_PCIE_HPP__
#define __DRIVERS_PCIE_HPP__

#define PCI_CONFIG_STATUS       0x06
#define PCI_CONFIG_ADDRESS      0xCF8
#define PCI_CONFIG_DATA         0xCFC
#define PCI_CONFIG_CAP_BASE     0x34
#define PCI_CONFIG_IRQ_LINE     0x3C

#define PCI_CREATE_CONFIG_ADDRESS(bus, dev, func, off) ((1 << 31) | ((bus) << 16) | ((dev) << 11) | ((func) << 8) | ((off) & 0xFC))

#define PCI_GET_BAR_OFFSET(index)           (0x10 + ((index) * 4))
#define PCI_VENDOR_DEVICE_ID_INVALID        MAX_UINT32
#define PCI_BAR_ADDRESS_MASKED(address)     ((address) & 0xFFFFFFF0)

#define PCI_UNKNOWN -1

#define PCI_COMMAND             0x4
#define PCI_CMD_MMIO            (1 << 1)
#define PCI_CMD_BUS_MASTERING   (1 << 2)

#define PCI_BAR_IO_REGION    (1 << 0)

#include "common.hpp"
#include "utils/vector.hpp"

union pci_vendor_device_id_t {
    u32 raw;
    struct {
        u16 vendor_id;
        u16 device_id;
    };
};

union pci_class_info_t {
    u32 raw;
    struct {
        u8 revision_id;
        u8 prog_if;
        u8 sub_class;
        u8 class_code;
    };
};

struct pci_device_t {
    pci_vendor_device_id_t vendor_device_id;
    pci_class_info_t class_info;

    u32 bus;
    u32 device;
    u32 function;
};

struct pcie_device_manager_t {
    linked_list<pci_device_t> devices;
};

void set_global_pcie_device_manager(pcie_device_manager_t* pcie_device_manager);
pcie_device_manager_t* get_global_pcie_device_manager();

u32 pci_config_read(const pci_device_t* p_device, u32 offset);
void pci_config_write(const pci_device_t* p_device, u32 offset, u32 value);

const char* pci_get_class_description(const pci_device_t* p_device);

u32 pci_read_bar(const pci_device_t* p_device, u32 bar);
bool pci_write_bar(const pci_device_t* device, u32 bar, u32 value);

bool pci_enumerate_devices(pcie_device_manager_t* device_manager);
pci_device_t* pci_find_device(pcie_device_manager_t* device_manager, const pci_vendor_device_id_t* p_vendor_device_id_target);
pci_device_t* pci_find_device(pcie_device_manager_t* device_manager, const pci_class_info_t* p_class_info_target);

void pci_loop_devices(pcie_device_manager_t* device_manager, void(*callback)(const pci_device_t*));

bool pci_cmd_enable(const pci_device_t* pcie_device, u32 flags);

#endif // __DRIVERS_PCIE_HPP__