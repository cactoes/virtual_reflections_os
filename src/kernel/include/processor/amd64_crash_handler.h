//==========================================
/// @file       amd64_crash_handler.h
/// @brief      
//==========================================

#pragma once

#ifndef AMD64_CRASH_HANDLER_H
#define AMD64_CRASH_HANDLER_H

#include "processor/amd64_interrupt.h"
#include "libarray.h"

interrupt_frame_t amd64_get_frame();
void amd64_crash_capture();
static_array_t amd64_get_crash_registers(const void* frame);

#endif // AMD64_CRASH_HANDLER_H