//==========================================
/// @file       common.hpp
/// @brief      general items that are used
///             in the entire kernel
//==========================================

#pragma once

#ifndef __COMMON_HPP__
#define __COMMON_HPP__

typedef unsigned long long size_t;
typedef unsigned long long uint64_t;
typedef          long long int64_t;

typedef unsigned int uint32_t;
typedef          int int32_t;

typedef unsigned short uint16_t;
typedef          short int16_t;

typedef unsigned char uint8_t;
typedef   signed char int8_t;

void* memset(void* dest, uint8_t val, size_t size);
void* memzero(void* dest, size_t size);
void* memcpy(void* dest, const void* src, size_t size);

bool mem_is_aligned(uint64_t addr, uint64_t align);
uint64_t mem_align_up(uint64_t addr, uint64_t align);
uint64_t mem_align_down(uint64_t addr, uint64_t align);

#endif // __COMMON_HPP__