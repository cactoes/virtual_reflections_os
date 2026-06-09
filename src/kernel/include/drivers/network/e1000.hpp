//==========================================
/// @file       e1000.hpp
/// @brief      e1000 intel network card driver
//==========================================

#pragma once

#ifndef __DRIVER_NETWORK_E1000_HPP__
#define __DRIVER_NETWORK_E1000_HPP__

#define E1000_STATUS                        0x8
#define E1000_STATUS_EEPROM_PRESENT         (1 << 8)

#define E1000_CTRL                          0x0
#define E1000_CTRL_RST                      (1 << 26)

#define E1000_RAL0                          0x5400
#define E1000_RAH0                          0x5404
#define E1000_RAH_AV                        (1 << 31)

#define E1000_BUFFER_SIZE                   2048
#define E1000_RECEIVE_DESC_COUNT            32
#define E1000_TRANSMIT_DESC_COUNT           8

#define E1000_RDBAL                         0x2800
#define E1000_RDBAH                         0x2804
#define E1000_RDLEN                         0x2808
#define E1000_RDH                           0x2810
#define E1000_RDT                           0x2818
#define E1000_RCTL                          0x0100

#define E1000_TDBAL                         0x3800
#define E1000_TDBAH                         0x3804
#define E1000_TDLEN                         0x3808
#define E1000_TDH                           0x3810
#define E1000_TDT                           0x3818
#define E1000_TCTL                          0x0400

#define E1000_TCTL_EN                       (1 << 1)
#define E1000_TCTL_PSP                      (1 << 3)
#define E1000_TCTL_CT_SHIFT                 4
#define E1000_TCTL_COLD_SHIFT               12

#define E1000_RCTL_SZ_2048                  (0 << 16)
#define E1000_RCTL_EN                       (1 << 1)
#define E1000_RCTL_UPE                      (1 << 3)
#define E1000_RCTL_MPE                      (1 << 4)
#define E1000_RCTL_BAM                      (1 << 15)
#define E1000_RCTL_SZ_4096                  (1 << 16)
#define E1000_RCTL_SECRC                    (1 << 26)
#define E1000_RCTL_SZ_8192                  (2 << 16)
#define E1000_RCTL_SZ_16384                 (3 << 16)

#define E1000_ICR                           0xC0
#define E1000_IMS                           0xD0
#define E1000_IMC                           0xD8

#define E1000_IMS_LSC                       (1 << 2)
#define E1000_IMS_RXSEQ                     (1 << 3)
#define E1000_IMS_RXDMT0                    (1 << 4)
#define E1000_IMS_RXT0                      (1 << 7)

#define E1000_CMD_EOP (1 << 0)
#define E1000_CMD_IFCS (1 << 1)
#define E1000_CMD_RS  (1 << 3)

#define E1000_RDESC_STATUS_DONE             (1 << 0)
#define E1000_TDESC_STATUS_DONE             (1 << 0)

#include "common.hpp"
#include "drivers/pcie.hpp"
#include "network/nidm.hpp"

struct e1000_rdesc_t {
    u64 buffer_addr;
    u16 length;
    u16 checksum;
    u8 status;
    u8 errors;
    u16 special;
} PACKED;

struct e1000_tdesc_t {
    u64 buffer_addr;
    u16 length;
    u8 cso;
    u8 cmd;
    u8 status;
    u8 css;
    u16 special;
} PACKED;

struct e1000_t {
    void* mmio_region;
    u8 mac[6];

    heap_t* dma_heap;

    e1000_rdesc_t* rdesc_array;
    u8* rdesc_buffer_array;
    size_t rx_tail;

    e1000_tdesc_t* tdesc_array;
    u8* tdesc_buffer_array;
    size_t tx_tail;
};

bool e1000_init_device(const pci_device_t* p_pcie_device, e1000_t* p_network_device);
bool e1000_send_packet(e1000_t* p_device, const void* data, size_t size);
void e1000_generic_handle_interrupt(e1000_t* device);

bool is_e1000_device(const pci_device_t* device);

#endif // __DRIVER_NETWORK_E1000_HPP__