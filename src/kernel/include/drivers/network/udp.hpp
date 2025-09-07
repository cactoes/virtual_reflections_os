//==========================================
/// @file       udp.hpp
/// @brief      user datagram protocol
//==========================================

#pragma once

#ifndef __DRIVERS_NETWORK_UDP_HPP__
#define __DRIVERS_NETWORK_UDP_HPP__

#include "common.hpp"
#include "drivers/network/nidm.hpp"

struct udp_header_t {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} PACKED;

void udp_receive(network_interface_device_t* p_device, uint8_t* p_payload, size_t payload_length);
int udp_send(network_interface_device_t* p_device, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, const uint8_t* p_payload, size_t size);

#endif // __DRIVERS_NETWORK_UDP_HPP__