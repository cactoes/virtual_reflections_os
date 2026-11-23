//==========================================
/// @file       libatomic.h
/// @brief      
//==========================================

#pragma once

#ifndef LIBATOMIC_H
#define LIBATOMIC_H

#include "libtype.h"

u32 atomic_fetch32(volatile u32* ptr);
void atomic_store32(volatile u32* ptr, u32 value);
u32 atomic_add32(volatile u32* ptr, u32 value);

#endif // LIBATOMIC_H