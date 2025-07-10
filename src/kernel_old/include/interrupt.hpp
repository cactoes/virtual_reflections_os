//==========================================
/// @file       interrupt.hpp
/// @brief      kernel interupts logic
//==========================================

#pragma once

#ifndef __INTERRUPT_HPP__
#define __INTERRUPT_HPP__

#define INT_TYPE_COUNT          ((size_t)(interrupt_type::__ITEM_COUNT))
#define INT_TYPE_CAST(type)     ((uint64_t)(type))

#include "common.hpp"
#include "cpu.hpp"

enum class interrupt_type {
    CRITICAL = 0,
    PIT,
    KEYBOARD,
    MOUSE,
    OTHER,
    __ITEM_COUNT
};

typedef cpu_state_t*(*interrupt_callback)(uint64_t code, cpu_state_t*);

void int_init();
void int_set_callback(interrupt_type type, interrupt_callback int_cb);

#endif // __INTERRUPT_HPP__