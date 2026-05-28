//==========================================
/// @file       rtl8168.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __DRIVERS_NETWORK_RTL8168_HPP__
#define __DRIVERS_NETWORK_RTL8168_HPP__

#define RTL8168_MAC         0x0 // size 6
#define RTL8168_MAR         0x8 // size 8
#define RTL8168_RBSTART     0x30 // size 4
#define RTL8168_CMD         0x37 // size 1
#define RTL8168_CAPR        0x38 // size 2
#define RTL8168_IMR         0x3C // size 2
#define RTL8168_ISR         0x3E // size 2

#define RTL8168_CONFIG_1    0x52 // 1

#define RTL8168_LWAKE       0x0
#define RTL8168_LWPTN       0x0

#define RTL8168_CMD_RST     0x10

#include "drivers/pcie.hpp"
#include "memory/heap.hpp"
#include "common.hpp"

struct rtl8168_t {
    void* mmio_region;
    u8 mac[6];

    heap_t* dma_heap;
};

bool rtl8168_init_device(const pci_device_t* pcie_device, rtl8168_t* network_device);
bool is_rtl8168_device(const pci_device_t* device);

#endif // __DRIVERS_NETWORK_RTL8168_HPP__