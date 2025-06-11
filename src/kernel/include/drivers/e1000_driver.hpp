//==========================================
/// @file       e1000_driver.hpp
/// @brief      e1000 intel driver
//==========================================
#pragma once

#ifndef __E1000_DRIVER_HPP__
#define __E1000_DRIVER_HPP__

#define E1000_MMIO_ADDR             0x3f800000
#define E1000_DMA_HEAP_ADDR         (E1000_MMIO_ADDR - PAGE_SIZE_LARGE)

#define E1000_STATUS                0x00008
#define E1000_STATUS_EEPROM_PRESENT (1 << 8)

#define E1000_EEPROM_READ_REG       0x14
#define E1000_EEPROM_READ_START     1
#define E1000_EEPROM_READ_DONE_BIT  (1 << 4)
#define E1000_EEPROM_READ_TIMEOUT   1000000

#define E1000_REG_RAL 0x5400
#define E1000_REG_RAH 0x5404
#define E1000_RAH_AV  (1U << 31)

#define E1000_NUM_RX_DESC 32
#define E1000_NUM_TX_DESC 8

#include "common.hpp"
#include "drivers/pci_driver.hpp"

struct e1000_device_t {
    uint32_t* mmio_region;
    uint8_t mac[6];
};

struct e1000_receive_desc_t {
    uint64_t buffer_addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} PACKED;

struct e1000_transmit_desc_t {
    uint64_t buffer_addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} PACKED;

int e1000_init(void* pml4, pci_device_info_t* ahci_pci_device, e1000_device_t* device);

#endif // __E1000_DRIVER_HPP__  