//==========================================
/// @file       virtual_reflections_exports.hpp
/// @brief      kernel api exports / actual implementations
//==========================================

#pragma once

#ifndef __VIRTUAL_REFLECTIONS_EXPORTS_HPP__
#define __VIRTUAL_REFLECTIONS_EXPORTS_HPP__

#include "common.hpp"

uint64_t ktime_since_boot();
void kprint(const char* str);
void knet_udp_send(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, uint8_t* packet, size_t size);
void ksleep(uint64_t ms);

#endif // __VIRTUAL_REFLECTIONS_EXPORTS_HPP__