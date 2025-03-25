//==========================================
/// @file       pci.hpp
/// @brief      pci related functions
//==========================================

#pragma once

#ifndef __PCI_HPP___
#define __PCI_HPP__

#include "../common.hpp"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

#define PCI_CREATE_CONFIG_ADDRESS(bus, dev, func, off) ((1 << 31) | ((bus) << 16) | ((dev) << 11) | ((func) << 8) | ((off) & 0xFC))

// #define PCI_GET_BASE_CLASS(class_info)      (((class_info) >> 24) & 0xFF)
// #define PCI_GET_SUB_CLASS(class_info)       (((class_info) >> 16) & 0xFF)
// #define PCI_GET_PROG_IF(class_info)         (((class_info) >> 8) & 0xFF)
// #define PCI_GET_REVISION_ID(class_info)     ((class_info) & 0xFF)

// #define PCI_GET_VENDOR_ID(id_info) ((id_info) & 0xFFFF)
// #define PCI_GET_DEVICE_ID(id_info) (((id_info) >> 16) & 0xFFFF)

#define PCI_GET_BAR_OFFSET(index)           (0x10 + ((index) * 4))
#define PCI_VENDOR_DEVICE_ID_INVALID        0xFFFFFFFF
#define PCI_BAR_ADDRESS_MASKED(address)     ((address) & 0xFFFFFFF0)

#define PCI_UNKNOWN -1

#define PCI_CMD_MMIO            (1 << 1)
#define PCI_CMD_BUS_MASTERING   (1 << 2)

namespace kernel::driver::pci {

typedef struct __KD_PCI_DEVICE_INFO {
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
} pci_device_info_t;

typedef struct __KD_PCI_DEVICE_REQUEST {
    union {
        uint32_t class_info = (uint32_t)PCI_UNKNOWN;
        struct {
            uint8_t revision_id;
            uint8_t prog_if;
            uint8_t sub_class;
            uint8_t class_code;
        };
    };

    size_t pci_device_index;
} pci_device_request_t;

/// @brief              read a pci config at specific offset
/// @param bus          number used to identify the pci(e) switch
/// @param device       number is used to identify the specific endpoint
/// @param function     number is used to identify a specific function or capability
/// @param offset       offset within the config
/// @param[out] value   config data at offset
/// @return             KRESULT(0): success
[[nodiscard]] kresult_t
config_read(
    uint16_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset,
    uint32_t* value);

/// @brief              writes data at offset
/// @param bus          number used to identify the pci(e) switch
/// @param device       number is used to identify the specific endpoint
/// @param function     number is used to identify a specific function or capability
/// @param offset       offset within the config
/// @param value        data to write at offset
/// @return             KRESULT(0): success
[[nodiscard]] kresult_t
config_write(
    uint16_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset,
    uint32_t value);

/// @brief                      loops over all pci(e) devices & checks validity
/// @param[inout] pci_list      the list where the valid pci(e) device info will be placed
/// @param max_size             max size of the list
/// @param[out] actual_size     actual size of the pci list
/// @return                     KRESULT(0): success
///                             KRESULT(1): max_size was too small
[[nodiscard]] kresult_t
enumerate_pci_devices(
    pci_device_info_t* pci_list,
    size_t max_size,
    size_t* actual_size);

/// @brief                      enumerates pci(e) devices but doesn't initialize
///                             or read the device only counts it if its valid
/// @param[out] device_count    actual device count
/// @return                     KRESULT(0): success
[[nodiscard]] kresult_t
get_pci_device_count(
    size_t* device_count);

/// @brief                 turns class code into description
/// @param[in] pci_device  pointer to pci(e) device info
/// @return                string for pci(e) class
const char*
get_class_description(
    const pci_device_info_t* pci_device);

/// @brief                              looks for all requested pci devices in the given
///                                     pci device list.
///                                     if there are multiple that match only the
///                                     first one will be stored.
/// @paramp[in] pci_device_list         list of all pci devices
/// @param size                         size of pci_device_list
/// @param[inout] requested_devices     list of devices that are required to be in the pci device list
/// @param requested_devices_size       size of requested_devices list
/// @return                             KRESULT(0): success
///                                     KERSULT(1): invalid pointer(s) given
///                                     KERSULT(2): not all devices were found
[[nodiscard]] kresult_t
find_pci_devices(
    const pci_device_info_t* pci_device_list,
    size_t size,
    pci_device_request_t* requested_devices,
    size_t requested_devices_size);

} // namespace kernel::driver::pci

// // print debug information
// for (size_t i = 0; i < device_count; i++) {
//     auto& dev = pci_entries.get()[i];
//     const char* device_description = kernel::driver::pci::get_class_description(&dev);

//     kernel::print::print("%s: PCI device %uh:%uh (id: %uh:%uh.%u)\n", device_description, dev.vendor_id, dev.device_id, dev.bus, dev.device, dev.function);

//     if (dev.bar0_address != 0) kernel::print::print("    BAR0: 0x%p [0x%p]\n", dev.bar0_address, PCI_BAR_ADDRESS_MASKED(dev.bar0_address));
//     if (dev.bar1_address != 0) kernel::print::print("    BAR1: 0x%p [0x%p]\n", dev.bar1_address, PCI_BAR_ADDRESS_MASKED(dev.bar1_address));
//     if (dev.bar2_address != 0) kernel::print::print("    BAR2: 0x%p [0x%p]\n", dev.bar2_address, PCI_BAR_ADDRESS_MASKED(dev.bar2_address));
//     if (dev.bar3_address != 0) kernel::print::print("    BAR3: 0x%p [0x%p]\n", dev.bar3_address, PCI_BAR_ADDRESS_MASKED(dev.bar3_address));
//     if (dev.bar4_address != 0) kernel::print::print("    BAR4: 0x%p [0x%p]\n", dev.bar4_address, PCI_BAR_ADDRESS_MASKED(dev.bar4_address));
//     if (dev.bar5_address != 0) kernel::print::print("    BAR5: 0x%p [0x%p]\n", dev.bar5_address, PCI_BAR_ADDRESS_MASKED(dev.bar5_address));
// }

#endif // __PCI_HPP__