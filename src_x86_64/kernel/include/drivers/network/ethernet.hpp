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
#include "drivers/network/nidm.hpp"

struct ethernet_header_t {
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t ethernet_type;
};

void ethernet_send(network_interface_device_t* p_device, uint8_t p_dst_mac[6], uint16_t type, const uint8_t* p_packet, size_t size);
int ethernet_receive(network_interface_device_t* p_device, uint8_t* p_frame, size_t size);

#endif // __DRIVERS_NETWORK_ETHERNET_HPP__