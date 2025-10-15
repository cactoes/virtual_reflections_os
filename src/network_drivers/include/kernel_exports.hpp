//==========================================
/// @file       kernel_exports.hpp
/// @brief      
//==========================================
#pragma once

#ifndef __KERNEL_EXPORTS_HPP__
#define __KERNEL_EXPORTS_HPP__

#include "common.hpp"

/// @brief kernel api functions
extern "C" {
    // NOLINTBEGIN
    void kprint(const char* str);
    #define KsPrint kprint
    
    uint64_t ktime_since_boot();
    #define KsTimeSinceBoot ktime_since_boot

    void knet_udp_send(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, uint8_t* packet, size_t size);
    #define KsNetUdpSend knet_udp_send
    // NOLINTEND
}

#endif // __KERNEL_EXPORTS_HPP__