//==========================================
/// @file       string.hpp
/// @brief      string utils
//==========================================

#pragma once

#ifndef __STRING_HPP__
#define __STRING_HPP__

#include "common.hpp"

#include <stdarg.h>

/// @brief          returns length of string
/// @param[in] str  string to get length of
/// @return         length of string ex nullterminator
size_t strlen(const char* str);

/// @brief                  prints pointer into input buffer
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param[in] num          input pointer
/// @return                 size of string inc null terminator
size_t sprintf(char* buffer, size_t size, void* ptr);

/// @brief                  prints number into input buffer
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param num              input number
/// @param base             number base
/// @return                 size of string inc null terminator
size_t sprintf(char* buffer, size_t size, uint32_t num, int base = 10);

/// @brief                  prints number into input buffer
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param num              input number
/// @param base             number base
/// @return                 size of string inc null terminator
size_t sprintf(char* buffer, size_t size, int32_t num, int base = 10);

/// @brief                  prints number into input buffer
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param num              input number
/// @param base             number base
/// @return                 size of string inc null terminator
size_t sprintf(char* buffer, size_t size, uint64_t num, int base = 10);

/// @brief                  prints number into input buffer
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param num              input number
/// @param base             number base
/// @return                 size of string inc null terminator
size_t sprintf(char* buffer, size_t size, int64_t num, int base = 10);

/// @brief                  prints number into input buffer
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param num              input number
/// @param precision        numbers after decimal
/// @return                 size of string inc null terminator
size_t sprintf(char* buffer, size_t size, double num, int precision = 6);

/// @brief          implementation for sprintf
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param[in] fmt          string with formatting
/// @param args             argument list
/// @return                 size of string inc null terminator
size_t sprintf(char* buffer, size_t size, const char* fmt, va_list args);

/// @brief                  prints input variables into a string
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param[in] fmt          string with formatting
///                         %p  = void*
///                         %s  = char*
///                         %c  = char
///                         %u  = uint32_t
///                         %ul = uint64_t
///                         %uh = uint64_t as hex
///                         %i  = int32_t
///                         %il = int64_t
///                         %ih = int64_t as hex
///                         %f  = float / double
/// @param                  argument list
/// @return                 size of string inc null terminator
size_t sprintf(char* buffer, size_t size, const char* fmt, ...);

bool strncpy(char* dest, const char* src, size_t dest_size);
char* strchr(const char* s, char c);
bool streq(const char* str1, const char* str2);
int strff(const char* str, char ch);
bool str_start_with(const char* str1, const char* target);

#endif // __STRING_HPP__