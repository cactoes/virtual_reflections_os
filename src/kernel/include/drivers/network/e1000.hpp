//==========================================
/// @file       e1000.hpp
/// @brief      e1000 intel network card driver
//==========================================

#pragma once

#ifndef __DRIVER_NETWORK_E1000_HPP__
#define __DRIVER_NETWORK_E1000_HPP__

#define E1000_STATUS                    0x8
#define E1000_STATUS_EEPROM_PRESENT     (1 << 8)

#define E1000_CTRL          0x0
#define E1000_CTRL_RST      (1 << 26)

#define E1000_RAL0      0x5400
#define E1000_RAH0      0x5404
#define E1000_RAH_AV    (1 << 31)

#include "common.hpp"
#include "drivers/pcie.hpp"

struct e1000_rdesc_t {
    uint64_t buffer_addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} PACKED;

struct e1000_tdesc_t {
    uint64_t buffer_addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} PACKED;

struct e1000_t {
    void* mmio_region;
    uint8_t mac[6];

    e1000_rdesc_t* rdesc_array;
    uint8_t* rdesc_buffer_array;
    size_t rx_tail;

    e1000_rdesc_t* tdesc_array;
    uint8_t* tdesc_buffer_array;
    size_t tx_tail;
};

int e1000_init_device(const pci_device_t* p_pcie_device, e1000_t* p_network_device);

#endif // __DRIVER_NETWORK_E1000_HPP__