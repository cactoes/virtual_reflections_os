//==========================================
/// @file       crash_handler.h
/// @brief      
//==========================================

#pragma once

#ifndef CRASH_HANDLER_H
#define CRASH_HANDLER_H

#include "libtype.h"
#include "libdef.h"

__noreturn
void crash_handler_entry(void* frame, const char* message, u64 core);

#endif // CRASH_HANDLER_H