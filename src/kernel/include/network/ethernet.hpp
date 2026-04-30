//==========================================
/// @file       ethernet.hpp
/// @brief      ethernet processing layer
//==========================================

#pragma once

#ifndef __DRIVERS_NETWORK_ETHERNET_HPP__
#define __DRIVERS_NETWORK_ETHERNET_HPP__

#define ETHERNET_TYPE_IPV4      0x0800
#define ETHERNET_TYPE_ARP       0x0806

#include "common.hpp"
#include "network/nidm.hpp"

struct ethernet_header_t {
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t ethernet_type;
} PACKED;

int ethernet_receive(network_interface_t* interface, uint8_t* frame, size_t size);
void ethernet_send(network_interface_t* interface, uint8_t dst_mac[6], uint16_t type, const uint8_t* packet, size_t size);

#endif // __DRIVERS_NETWORK_ETHERNET_HPP__