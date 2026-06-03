//==========================================
/// @file       rtl8168.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __DRIVERS_NETWORK_RTL8168_HPP__
#define __DRIVERS_NETWORK_RTL8168_HPP__

#define RTL8168_MAC                 0x0
#define RTL8168_MAR                 0x8
#define RTL8168_TX_DESC_ADDR_LOW    0x20
#define RTL8168_TX_DESC_ADDR_HIGH   0x24
#define RTL8168_RBSTART             0x30
#define RTL8168_CMD                 0x37
#define RTL8168_CAPR                0x38
#define RTL8168_IMR                 0x3C
#define RTL8168_ISR                 0x3E
#define RTL8168_TX_CONFIG           0x40
#define RTL8168_RX_CONFIG           0x44
#define RTL8168_CFG9346             0x50
#define RTL8168_RX_MAX_SIZE         0xDA
#define RTL8168_RX_DESC_ADDR_LOW    0xE4
#define RTL8168_RX_DESC_ADDR_HIGH   0xE8

#define RTL8168_CONFIG_1            0x52

#define RTL8168_LWAKE               0x0
#define RTL8168_LWPTN               0x0
#define RTL8168_CMD_TX_EN           0x04
#define RTL8168_CMD_RX_EN           0x08
#define RTL8168_CFG9346_UNLOCK      0xC0
#define RTL8168_CFG9346_LOCK        0x00

#define RTL8168_CMD_RST             0x10

#define RTL8168_TPPOLL 0x38

#define RTL8168_DESC_OWN            (1U << 31)
#define RTL8168_DESC_EOR            (1U << 30)
#define RTL8168_DESC_FS             (1U << 29)
#define RTL8168_DESC_LS             (1U << 28)

#define NUM_TX_DESC                 64
#define NUM_RX_DESC                 64
#define RX_BUF_SIZE                 1536
#define TX_BUF_SIZE                 1536

#define RTL8168_ISR_ROK     0x0001
#define RTL8168_ISR_RER     0x0002
#define RTL8168_ISR_TOK     0x0004
#define RTL8168_ISR_TER     0x0008
#define RTL8168_ISR_LINKCHG 0x0020

#include "drivers/pcie.hpp"
#include "memory/heap.hpp"
#include "common.hpp"

struct rtl8168_desc_t {
    u32 command;
    u32 vlan;
    u32 address_low;
    u32 address_high;
} PACKED;

struct rtl8168_t {
    void* mmio_region;
    u8 mac[6];

    heap_t* dma_heap;

    rtl8168_desc_t* tdesc_array;
    void* tx_buffer_array;
    u64 tx_current;

    rtl8168_desc_t* rdesc_array;
    void* rx_buffer_array;
    u64 rx_current;
};

bool rtl8168_init_device(const pci_device_t* pcie_device, rtl8168_t* network_device);
bool is_rtl8168_device(const pci_device_t* device);
bool rtl8168_send_packet(rtl8168_t* device, const void* data, u64 size);
void rtl8168_generic_handle_interrupt(rtl8168_t* device);

#endif // __DRIVERS_NETWORK_RTL8168_HPP__