//==========================================
/// @file       kernel_api.hpp
/// @brief      kernel api header for drivers
//==========================================
#pragma once

#ifndef __KERNEL_API_HPP__
#define __KERNEL_API_HPP__

#include "common.hpp"

uint64_t ktime_since_boot();
void kprint(const char* str);
void knet_udp_send(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, uint8_t* packet, size_t size);

#endif // __KERNEL_API_HPP__