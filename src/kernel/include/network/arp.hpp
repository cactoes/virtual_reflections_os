//==========================================
/// @file       arp.hpp
/// @brief      address resolution protocol
//==========================================

#pragma once

#ifndef __DRIVERS_NETWORK_ARP_HPP__
#define __DRIVERS_NETWORK_ARP_HPP__

#include "common.hpp"
#include "network/nidm.hpp"

// TODO @since 27/08/2025 -- 01:53
// cache clearing idk
struct arp_table_entry_t {
    u64 timestamp;
    u8 mac[6];
};

struct arp_packet_t {
    u16 harware_type;
    u16 protocol_type;

    u8 hardware_length;
    u8 protocol_length;

    u16 operation;

    u8 sender_hw[6];
    u32 sender_ip;
    u8 target_hw[6];
    u32 target_ip;
} __packed;

void arp_table_insert(u32 ipv4, u8 mac[6]);
void arp_discover_request_ipv4(network_interface_t* interface, u32 ipv4);
void arp_receive(network_interface_t* interface, u8* packet, size_t size);
bool arp_lookup(u32 ipv4_addr, u8 mac_out[6]);

#endif // __DRIVERS_NETWORK_ARP_HPP__