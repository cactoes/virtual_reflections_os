//==========================================
/// @file       e1000_driver.hpp
/// @brief      e1000 intel driver
//==========================================
#pragma once

#ifndef __E1000_DRIVER_HPP__
#define __E1000_DRIVER_HPP__

#define E1000_MMIO_ADDR             0x3f800000
#define E1000_DMA_HEAP_ADDR         (E1000_MMIO_ADDR - PAGE_SIZE_LARGE)

#define E1000_STATUS  0x00008
#define E1000_STATUS_EEPROM_PRESENT (1 << 8)

#include "common.hpp"
#include "drivers/pci_driver.hpp"

struct e1000_device_t {
    uint8_t mac[6];
};

int e1000_init(void* pml4, pci_device_info_t* ahci_pci_device, e1000_device_t* device);

#endif // __E1000_DRIVER_HPP__  