//==========================================
/// @file       virtual_reflections_exports.hpp
/// @brief      kernel api exports / actual implementations
//==========================================

#pragma once

#ifndef __VIRTUAL_REFLECTIONS_EXPORTS_HPP__
#define __VIRTUAL_REFLECTIONS_EXPORTS_HPP__

#include "common.hpp"

u64 ktime_since_boot();
void kprint(const char* str);
void knet_udp_send(u32 dst_ip, u16 src_port, u16 dst_port, u8* packet, size_t size);
void ksleep(u64 ms);

#endif // __VIRTUAL_REFLECTIONS_EXPORTS_HPP__