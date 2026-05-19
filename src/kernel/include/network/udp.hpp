//==========================================
/// @file       udp.hpp
/// @brief      user datagram protocol
//==========================================

#pragma once

#ifndef __DRIVERS_NETWORK_UDP_HPP__
#define __DRIVERS_NETWORK_UDP_HPP__

#include "common.hpp"
#include "network/nidm.hpp"

struct udp_header_t {
    u16 src_port;
    u16 dst_port;
    u16 length;
    u16 checksum;
} PACKED;

void udp_receive(network_interface_t* interface, u32 src_ip, u8* payload, size_t payload_length);
bool udp_send(u32 dst_ip, u16 src_port, u16 dst_port, const u8* payload, size_t size);

#endif // __DRIVERS_NETWORK_UDP_HPP__