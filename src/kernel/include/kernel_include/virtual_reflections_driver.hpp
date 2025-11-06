//==========================================
/// @file       virtual_reflection_driver.hpp
/// @brief      kernel api driver
//==========================================

#pragma once

#ifndef __VIRTUAL_REFLECTION_DRIVER_HPP__
#define __VIRTUAL_REFLECTION_DRIVER_HPP__
// NOLINTBEGIN

#ifdef DRIVER_NAMING
    #define KsPrint kprint
    #define KsTimeSinceBoot ktime_since_boot
    #define KsNetUdpSend knet_udp_send
    #define KsSleep ksleep

    #define DriverInit driver_init
    #define DriverExit driver_exit
    #define QueryCapability query_capability
#endif

#include "common.hpp"

/// @brief kernel api functions
#ifdef __cplusplus
extern "C" {
#endif

void kprint(const char* str);
uint64_t ktime_since_boot();
void knet_udp_send(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, uint8_t* packet, size_t size);
void ksleep(uint64_t ms);

int driver_init();
int driver_exit();
uint64_t query_capability(const char* feature);

#ifdef __cplusplus
}
#endif

// NOLINTEND
#endif // __VIRTUAL_REFLECTION_DRIVER_HPP__