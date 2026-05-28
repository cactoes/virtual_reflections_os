//==========================================
/// @file       rtl8168.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __DRIVERS_NETWORK_RTL8168_HPP__
#define __DRIVERS_NETWORK_RTL8168_HPP__

#define RTL8168_MAC     0x0

#include "drivers/pcie.hpp"
#include "memory/heap.hpp"
#include "common.hpp"

struct rtl81xx_t {
    void* mmio_region;
    u8 mac[6];

    heap_t* dma_heap;
};

bool rtl8168_init_device(const pci_device_t* pcie_device, rtl81xx_t* network_device);
bool is_rtl8168_device(const pci_device_t* device);

#endif // __DRIVERS_NETWORK_RTL8168_HPP__