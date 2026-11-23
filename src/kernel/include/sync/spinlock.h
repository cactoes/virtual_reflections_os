//==========================================
/// @file       spinlock.h
/// @brief      do not use these inside an irq
//==========================================

#pragma once

#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "libtype.h"

typedef struct __spinlock {
    volatile u32 value;
} spinlock_t;

void spinlock_lock(spinlock_t* lock);
void spinlock_unlock(spinlock_t* lock);

#endif // SPINLOCK_H