//==========================================
/// @file       debug.h
/// @brief      
//==========================================

#pragma once

#ifndef DEBUG_H
#define DEBUG_H

bool kdbg_init();
void kdbg_printf(const char* fmt, ...);

#endif // DEBUG_H