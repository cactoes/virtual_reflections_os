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
    u8 dst_mac[6];
    u8 src_mac[6];
    u16 ethernet_type;
} __packed;

int ethernet_receive(network_interface_t* interface, u8* frame, size_t size);
void ethernet_send(network_interface_t* interface, u8 dst_mac[6], u16 type, const u8* packet, size_t size);

#endif // __DRIVERS_NETWORK_ETHERNET_HPP__