//==========================================
/// @file       string.hpp
/// @brief      string utils
//==========================================

#pragma once

#ifndef __STRING_HPP__
#define __STRING_HPP__

#define STR_CHECK_BUFF(cur, max, req) ((cur) + (req) < (max))

#include "common.hpp"

#include <stdarg.h>

/// @brief          returns length of string
/// @param[in] str  string to get length of
/// @return         length of string ex nullterminator
size_t strlen(const char* p_str);

/// @brief                  prints pointer into input buffer
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param[in] num          input pointer
/// @return                 size of string inc null terminator
size_t sprintf(char* p_buffer, size_t size, void* p_ptr);

/// @brief                  prints number into input buffer
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param num              input number
/// @param base             number base
/// @return                 size of string inc null terminator
size_t sprintf(char* p_buffer, size_t size, uint32_t num, int base = 10);

/// @brief                  prints number into input buffer
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param num              input number
/// @param base             number base
/// @return                 size of string inc null terminator
size_t sprintf(char* p_buffer, size_t size, int32_t num, int base = 10);

/// @brief                  prints number into input buffer
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param num              input number
/// @param base             number base
/// @return                 size of string inc null terminator
size_t sprintf(char* p_buffer, size_t size, uint64_t num, int base = 10);

/// @brief                  prints number into input buffer
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param num              input number
/// @param base             number base
/// @return                 size of string inc null terminator
size_t sprintf(char* p_buffer, size_t size, int64_t num, int base = 10);

/// @brief                  prints number into input buffer
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param num              input number
/// @param precision        numbers after decimal
/// @return                 size of string inc null terminator
size_t sprintf(char* p_buffer, size_t size, double num, int precision = 6);

/// @brief          implementation for sprintf
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param[in] fmt          string with formatting
/// @param args             argument list
/// @return                 size of string inc null terminator
size_t sprintf(char* p_buffer, size_t size, const char* p_fmt, va_list p_args);

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
size_t sprintf(char* p_buffer, size_t size, const char* p_fmt, ...);

bool strncpy(char* p_dest, const char* p_src, size_t dest_size);
char* strchr(const char* p_s, char c);
bool streq(const char* p_str1, const char* p_str2);
int strff(const char* p_str, char ch);
bool str_start_with(const char* p_str1, const char* p_target);
void unpack_be16_string(const uint16_t* src, int word_count, char* dest, int max_len);

class string {
public:
    static constexpr size_t k_npos = (size_t)-1;

    string();
    string(const char* p_string);
    
    // for now no copy
    string(const string& r_other);
    string& operator=(const string& r_other);
    
    string(string&& other);
    string& operator=(string&& other);
    
    ~string();

    const char* c_str() const;
    size_t length() const;

    string substr(size_t pos, size_t count = (size_t)-1) const;
    size_t find_last_of(char c) const;
    size_t find(const string& str, size_t start = 0) const;

    void assign(const char* p_string);
    void assign(const string& other);
    
    void append(const char* p_other);
    void append(const string& r_other);
    
    bool operator==(const string& r_other) const;
    bool operator==(const char* p_other) const;
    string operator+(const string& r_other);
    string operator+(const char* p_other);
    string& operator+=(const string& r_other);
    string& operator+=(const char* p_other);

    string& operator=(const char* p_other);
    
private:
    char* p_str = nullptr;
    size_t len = 0;
};

#endif // __STRING_HPP__