//==========================================
/// @file       ip.hpp
/// @brief      ip stuff
//==========================================

#pragma once

#ifndef __DRIVERS_NETWORK_IP_HPP__
#define __DRIVERS_NETWORK_IP_HPP__

#define IP_PROTOCOL_ICMP    1
#define IP_PROTOCOL_TCP     6
#define IP_PROTOCOL_UDP     17

#define BROADCAST_IPV4      TO_IP(255, 255, 255, 255)

#include "common.hpp"
#include "network/nidm.hpp"

struct ip_header_t {
    u8 version_ihl;
    u8 tos;
    u16 total_length;
    u16 identification;
    u16 flags_fragment;
    u8 ttl;
    u8 protocol;
    u16 header_checksum;
    u32 src_addr;
    u32 dst_addr;
} PACKED;

void ip_receive(network_interface_t* interface, u8* packet, size_t size, u8 src_mac[6]);
bool ip_send(network_interface_t* interface, u32 dst_ip, u8 protocol, const u8* payload, size_t payload_len);

#endif // __DRIVERS_NETWORK_IP_HPP__