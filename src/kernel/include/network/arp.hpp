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
    uint64_t timestamp;
    uint8_t mac[6];
};

struct arp_packet_t {
    uint16_t harware_type;
    uint16_t protocol_type;

    uint8_t hardware_length;
    uint8_t protocol_length;

    uint16_t operation;

    uint8_t sender_hw[6];
    uint32_t sender_ip;
    uint8_t target_hw[6];
    uint32_t target_ip;
} PACKED;

void arp_table_insert(uint32_t ipv4, uint8_t mac[6]);
void arp_discover_request_ipv4(network_interface_t* interface, uint32_t ipv4);
void arp_receive(network_interface_t* interface, uint8_t* packet, size_t size);
bool arp_lookup(uint32_t ipv4_addr, uint8_t mac_out[6]);

#endif // __DRIVERS_NETWORK_ARP_HPP__