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
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
} PACKED;

void icmp_receive(network_interface_t* interface, uint8_t* payload, size_t len);

#endif // __DRIVERS_NETWORK_ICMP_HPP__