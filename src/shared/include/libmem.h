//==========================================
/// @file       libmem.h
/// @brief      standard memory function defines
///             these functions will call implementation specific functions
//==========================================

#pragma once

#ifndef LIBMEM_H
#define LIBMEM_H

#include "libtype.h"

void* memzero(void* dst, u64 size);
void* memset(void* dst, u8 val, u64 size);
void* memcpy(void* dst, const void* src, u64 size);
bool memeq(const void* a, const void* b, u64 size);
void* memmove(void* dst, const void* src, u64 size);

#endif // LIBMEM_H