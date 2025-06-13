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

#define E1000_RECEIVE_DESC_COUNT 32
#define E1000_TRANSMIT_DESC_COUNT 8

#define E1000_BUFFER_SIZE 2048

#define E1000_TDBAL  0x03800
#define E1000_TDBAH  0x03804
#define E1000_TDLEN  0x03808
#define E1000_TDH    0x03810
#define E1000_TDT    0x03818
#define E1000_TCTL   0x00400
#define E1000_TIPG   0x00410

#define E1000_RDBAL   0x2800
#define E1000_RDBAH   0x2804
#define E1000_RDLEN   0x2808
#define E1000_RDH     0x2810
#define E1000_RDT     0x2818

#define E1000_TCTL_EN     (1 << 1)
#define E1000_TCTL_PSP    (1 << 3)
#define E1000_TCTL_CT_SHIFT 4
#define E1000_TCTL_COLD_SHIFT 12

#define E1000_TCTL_CT_15 0x0F << 4
#define E1000_TCTL_COLD_HALF 0x200 << 12
#define E1000_TCTL_COLD_FULL 0x40 << 12
#define E1000_TCTL_SWXOFF 1 << 22
#define E1000_TCTL_RTLC 1 << 24
#define E1000_TCTL_NRTU 1 << 2

#define E1000_RCTL_EN       (1 << 1)
#define E1000_RCTL_BAM      (1 << 15)
#define E1000_RCTL_SECRC    (1 << 26)
#define E1000_RCTL_SZ_2048  (0 << 16)
#define E1000_RCTL_SZ_4096  (1 << 16)
#define E1000_RCTL_SZ_8192  (2 << 16)
#define E1000_RCTL_SZ_16384 (3 << 16)
#define E1000_RCTL_UPE      (1 << 3)
#define E1000_RCTL_MPE      (1 << 4)

#define E1000_CMD_EOP (1 << 0)
#define E1000_CMD_IFCS (1 << 1)
#define E1000_CMD_RS  (1 << 3)

#include "common.hpp"
#include "drivers/pci_driver.hpp"

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

struct e1000_device_t {
    uint32_t* mmio_region;
    uint8_t mac[6];

    e1000_transmit_desc_t* transmit_descriptions;
    uint8_t* transmit_desc_buffers;
    
    e1000_receive_desc_t* receive_descriptions;
    uint8_t* receive_desc_buffers;
};

int e1000_send_packet(e1000_device_t* device, const void* data, size_t size);
int e1000_init(void* pml4, pci_device_info_t* ahci_pci_device, e1000_device_t* device);

#endif // __E1000_DRIVER_HPP__  