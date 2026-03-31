//==========================================
/// @file       icmp.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __DRIVERS_NETWORK_ICMP_HPP__
#define __DRIVERS_NETWORK_ICMP_HPP__

#include "common.hpp"
#include "drivers/network/nidm.hpp"

struct icmp_header_t {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
} PACKED;

void icmp_receive(network_interface_device_t* p_device, uint8_t* p_payload, size_t len);

#endif // __DRIVERS_NETWORK_ICMP_HPP__