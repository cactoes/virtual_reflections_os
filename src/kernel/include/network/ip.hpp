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

#include "common.hpp"
#include "network/nidm.hpp"

struct ip_header_t {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t header_checksum;
    uint32_t src_addr;
    uint32_t dst_addr;
} PACKED;

void ip_receive(network_interface_device_t* p_device, uint8_t* p_packet, size_t size);
bool ip_send(network_interface_device_t* p_device, uint32_t dst_ip, uint8_t protocol, const uint8_t* p_payload, size_t payload_len);
uint16_t ip_checksum(void* p_data, size_t len);

#endif // __DRIVERS_NETWORK_IP_HPP__