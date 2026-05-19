//==========================================
/// @file       icmp.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __DRIVERS_NETWORK_ICMP_HPP__
#define __DRIVERS_NETWORK_ICMP_HPP__

#include "common.hpp"
#include "network/nidm.hpp"

struct icmp_header_t {
    u8 type;
    u8 code;
    u16 checksum;
    u16 identifier;
    u16 sequence;
} PACKED;

void icmp_receive(network_interface_t* interface, u8* payload, size_t len);

#endif // __DRIVERS_NETWORK_ICMP_HPP__