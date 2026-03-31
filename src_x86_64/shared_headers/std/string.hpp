//==========================================
/// @file       string.hpp
/// @brief      string utils
//==========================================

#pragma once

#ifndef __STRING_HPP__
#define __STRING_HPP__

#define STR_CHECK_BUFF(cur, max, req) ((cur) + (req) < (max))

#include <stdarg.h>
#include "common.hpp"
#include "std/array.hpp"

/// @brief          returns length of string
/// @param[in] str  string to get length of
/// @return         length of string ex nullterminator
static size_t strlen(const char* str) {
    if (str == nullptr || *str == '\0')
        return 0;

    size_t len = 0;
    while (str[len] != '\0')
        len++;

    return len;
}

/// @brief                  prints pointer into input buffer
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param[in] num          input pointer
/// @return                 size of string inc null terminator
static size_t sprintf(char* buffer, size_t size, void* ptr) {
    if (size < 17 || !buffer)
        return 0;

    static const char s_numbers[] = "0123456789abcdef";

    uint64_t number = (uint64_t)ptr;

    memset(buffer, '0', 16ul);

    int i = 16;
    for (; number && i; number /= 16)
        buffer[--i] = s_numbers[number % 16];

    return 17;
}

/// @brief                  prints number into input buffer
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param num              input number
/// @param base             number base
/// @return                 size of string inc null terminator
static size_t sprintf(char* buffer, size_t size, uint32_t num, int base = 10) {
    if (size < 11 || !buffer)
        return 0;

    static const char s_numbers[] = "0123456789abcdef";

    if (base >= sizeof(s_numbers))
        return 0;

    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return 2;
    }

    uint32_t number = num;

    char tmp[11] = { 0 };
    int i = sizeof(tmp) - 1;
    for (; number && i; number /= base)
        tmp[--i] = s_numbers[number % base];

    char* p_str = &tmp[i];
    auto strlength = strlen(p_str);

    memcpy(buffer, &tmp[i], strlength);

    return strlength + 1;
}

/// @brief                  prints number into input buffer
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param num              input number
/// @param base             number base
/// @return                 size of string inc null terminator
static size_t sprintf(char* buffer, size_t size, int32_t num, int base = 10) {
    if (size < 12 || !buffer)
        return 0;

    static const char s_numbers[] = "0123456789abcdef";

    if (base >= sizeof(s_numbers))
        return 0;

    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return 2;
    }

    uint32_t number = ABS((uint32_t)num);

    char tmp[12] = { 0 };
    int i = sizeof(tmp) - 1;
    for (; number && i; number /= base)
        tmp[--i] = s_numbers[number % base];

    if (num < 0)
        tmp[--i] = '-';

    char* p_str = &tmp[i];
    auto strlength = strlen(p_str);

    memcpy(buffer, &tmp[i], strlength);

    return strlength + 1;
}

/// @brief                  prints number into input buffer
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param num              input number
/// @param base             number base
/// @return                 size of string inc null terminator
static size_t sprintf(char* buffer, size_t size, uint64_t num, int base = 10) {
    if (size < 21 || !buffer)
        return 0;

    static const char s_numbers[] = "0123456789abcdef";

    if (base >= sizeof(s_numbers))
        return 0;

    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return 2;
    }

    uint64_t number = num;

    char tmp[21] = { 0 };
    int i = sizeof(tmp) - 1;
    for (; number && i; number /= base)
        tmp[--i] = s_numbers[number % base];

    char* str = &tmp[i];
    auto strlength = strlen(str);

    memcpy(buffer, &tmp[i], strlength);

    return strlength + 1;
}

/// @brief                  prints number into input buffer
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param num              input number
/// @param base             number base
/// @return                 size of string inc null terminator
static size_t sprintf(char* buffer, size_t size, int64_t num, int base = 10) {
    if (size < 22 || !buffer)
        return 0;

    static const char s_numbers[] = "0123456789abcdef";

    if (base >= sizeof(s_numbers))
        return 0;

    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return 2;
    }

    uint64_t number = ABS((uint64_t)num);

    char tmp[22] = { 0 };
    int i = sizeof(tmp) - 1;
    for (; number && i; number /= base)
        tmp[--i] = s_numbers[number % base];

    if (num < 0)
        tmp[--i] = '-';

    char* str = &tmp[i];
    auto strlength = strlen(str);

    memcpy(buffer, &tmp[i], strlength);

    return strlength + 1;
}

/// @brief                  prints number into input buffer
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param num              input number
/// @param precision        numbers after decimal
/// @return                 size of string inc null terminator
static size_t sprintf(char* buffer, size_t size, double num, int precision = 6) {
    if (!buffer)
        return 0;

    static const char s_numbers[] = "0123456789";

    int int_part = (int)num;
    int dec_part = (num - int_part) * ((int)pow(10.0, precision));

    char inc_buff[22] = { 0 };
    auto inc_buff_size = sprintf(inc_buff, 22, int_part);

    char dec_buff[22] = { 0 };
    auto dec_buff_size = sprintf(dec_buff, 22, dec_part);

    if (inc_buff_size - 1 + dec_buff_size - 1 + 2 > size)
        return 0;

    memcpy(&buffer[0], inc_buff, inc_buff_size - 1);
    buffer[inc_buff_size - 1] = '.';
    memcpy(&buffer[inc_buff_size], dec_buff, dec_buff_size);

    return inc_buff_size - 1 + dec_buff_size - 1 + 2;
}

/// @brief                  implementation for sprintf
/// @param[inout] buffer    input buffer for string
/// @param size             size of input buffer
/// @param[in] fmt          string with formatting
/// @param args             argument list
/// @return                 size of string inc null terminator
static size_t sprintf(char* buffer, size_t size, const char* fmt, va_list args) {
    size_t len = 0;
    size_t string_length = strlen(fmt);

    for (size_t i = 0; i < string_length; i++) {
        const char& ch = fmt[i];

        if (ch == '%') {
            if (i + 1 < string_length) {
                i++;
            } else {
                if (STR_CHECK_BUFF(len, size, 1))
                    buffer[len++] = '%';
                continue;
            }

            const char& ch2 = fmt[i];
            switch (ch2) {
                case 'p': {
                    if (STR_CHECK_BUFF(len, size, 16)) {
                        char tmpbuf[17] = { 0 };
                        sprintf(tmpbuf, 17, va_arg(args, void*));
                        memcpy(&buffer[len], tmpbuf, 16);
                        len += 16;
                    }
                    break;
                }
                case 's': {
                    char* arg = va_arg(args, char*);
                    auto strlength = strlen(arg);
                    if (STR_CHECK_BUFF(len, size, strlength)) {
                        memcpy(&buffer[len], arg, strlength);
                        len += strlength;
                    }
                    break;
                }
                case 'c': {
                    char arg = (char)va_arg(args, int);
                    if (STR_CHECK_BUFF(len, size, 1)) {
                        memcpy(&buffer[len], &arg, 1);
                        len += 1;
                    }
                    break;
                }
                case 'u': {
                    if (i + 1 < string_length) {
                        if (fmt[i+1] == 'l') {
                            i++;
                            char tmpbuf[21] = { 0 };
                            auto strsize = sprintf(tmpbuf, 21, va_arg(args, uint64_t)) - 1;
                            if (STR_CHECK_BUFF(len, size, strsize)) {
                                memcpy(&buffer[len], tmpbuf, strsize);
                                len += strsize;
                            }

                            break;
                        }

                        if (fmt[i+1] == 'h') {
                            i++;
                            char tmpbuf[21] = { 0 };
                            auto strsize = sprintf(tmpbuf, 21, va_arg(args, uint64_t), 16) - 1;
                            if (STR_CHECK_BUFF(len, size, strsize)) {
                                memcpy(&buffer[len], tmpbuf, strsize);
                                len += strsize;
                            }

                            break;
                        }
                    }
                    char tmpbuf[11] = { 0 };
                    auto strsize = sprintf(tmpbuf, 11, va_arg(args, uint32_t)) - 1;
                    if (STR_CHECK_BUFF(len, size, strsize)) {
                        memcpy(&buffer[len], tmpbuf, strsize);
                        len += strsize;
                    }
                    break;
                }
                case 'i': {
                    if (i + 1 < string_length) {
                        if (fmt[i+1] == 'l') {
                            i++;
                            char tmpbuf[22] = { 0 };
                            auto strsize = sprintf(tmpbuf, 22, va_arg(args, int64_t)) - 1;
                            if (STR_CHECK_BUFF(len, size, strsize)) {
                                memcpy(&buffer[len], tmpbuf, strsize);
                                len += strsize;
                            }

                            break;
                        }

                        if (fmt[i+1] == 'h') {
                            i++;
                            char tmpbuf[22] = { 0 };
                            auto strsize = sprintf(tmpbuf, 22, va_arg(args, int64_t), 16) - 1;
                            if (STR_CHECK_BUFF(len, size, strsize)) {
                                memcpy(&buffer[len], tmpbuf, strsize);
                                len += strsize;
                            }

                            break;
                        }
                    }
                    char tmpbuf[12] = { 0 };
                    auto strsize = sprintf(tmpbuf, 12, va_arg(args, int32_t)) - 1;
                    if (STR_CHECK_BUFF(len, size, strsize)) {
                        memcpy(&buffer[len], tmpbuf, strsize);
                        len += strsize;
                    }
                    break;
                }
                case 'f': {
                    char tmpbuf[50] = { 0 };
                    auto strsize = sprintf(tmpbuf, 50, va_arg(args, double)) - 1;
                    if (STR_CHECK_BUFF(len, size, strsize)) {
                        memcpy(&buffer[len], tmpbuf, strsize);
                        len += strsize;
                    }
                    break;
                }
                default:
                    if (STR_CHECK_BUFF(len, size, 2)) {
                        buffer[len++] = '%';
                        buffer[len++] = ch2;
                    }
                    break;
            }

        } else {
            if (!STR_CHECK_BUFF(len, size, 1))
                break;
            
            buffer[len++] = ch;
        }
    }

    buffer[len] = '\0';
    len++;

    return len;
}

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
static size_t sprintf(char* buffer, size_t size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    size_t result = sprintf(buffer, size, fmt, args);
    va_end(args);

    return result;
}

static bool strncpy(char* dest, const char* src, size_t dest_size) {
    size_t src_len = strlen(src);

    if (src_len > dest_size)
        return false;

    memcpy(dest, src, src_len);

    return true;
}

static char* strchr(const char* str, char ch) {
    while (*str) {
        if (*str == ch)
            return (char*)str;
        str++;
    }

    if (ch == '\0')
        return (char*)str;

    return nullptr;
}

static bool streq(const char* str1, const char* str2) {
    while (*str1 && *str2 && *str1 == *str2) {
        str1++;
        str2++;
    }

    return *str1 == *str2;
}

static int strff(const char* str, char ch) {
    for (int i = 0; str[i]; i++) {
        if (str[i] == ch)
            return i;
    }

    return -1;
}

static bool str_starts_with(const char* str1, const char* target) {
    while (*target) {
        if (*str1 != *target)
            return false;
        str1++;
        target++;
    }

    return true;
}

static bool str_ends_with(const char* str, const char* target) {
    size_t str_len = strlen(str);
    size_t target_len = strlen(target);

    if (target_len > str_len)
        return false;

    return streq(str + str_len - target_len, target);
}

static void str_unpack_be16(const uint16_t* src, int word_count, char* dst, int max_len) {
    int pos = 0;
    memzero(dst, max_len);

    for (int i = 0; i < word_count && pos + 1 < max_len; i++) {
        dst[pos++] = (char)(src[i] >> 8);
        dst[pos++] = (char)(src[i] & 0xFF);
    }

    while (pos > 0 && dst[pos - 1] == ' ')
        pos--;

    dst[pos < max_len ? pos : max_len - 1] = '\0';
}

namespace std {

class string {
public:
    static constexpr size_t npos = (size_t)-1;

    string() = default;
    
    string(char ch) { assign(ch); }
    string(const char* string) { assign(string); }
    string(const std::string& other) { assign(other); }

    string& operator=(const std::string& other) {
        if (this == &other)
            return *this;

        char* new_str = (char*)malloc(other.len * sizeof(char) + 1);
        memzero(new_str, other.len * sizeof(char) + 1);
        strncpy(new_str, other.str, other.len);

        free(str);

        str = new_str;
        len = other.len;

        return *this;
    }
    
    string(string&& other) {
        str = other.str;
        len = other.len;

        other.str = nullptr;
        other.len = 0;
    }

    string& operator=(string&& other) {
        if (this != &other) {
            if (str)
                free(str);

            str = other.str;
            len = other.len;

            other.str = nullptr;
            other.len = 0;
        }

        return *this;
    }

    string& operator=(const char* other) {
        assign(other);
        return *this;
    }
    
    ~string() {
        if (str) {
            free(str);
            str = nullptr;
            len = 0;
        }
    }

    const char* c_str() const { return str; }
    size_t length() const { return len; }

    string substr(size_t pos, size_t count = npos) const {
        if (pos >= len)
            return string();

        if (pos + count > len || count == npos)
            count = len - pos;

        char* buffer = (char*)malloc(count + 1);
        for (size_t i = 0; i < count; i++)
            buffer[i] = str[pos + i];

        buffer[count] = '\0';

        string result(buffer);
        free(buffer);
        return result;
    }

    size_t find_last_of(char c) const {
        if (len == 0)
            return npos;

        for (size_t i = len; i > 0; i--)
            if (str[i - 1] == c)
                return i - 1;

        return npos;
    }

    size_t find(const std::string& string, size_t start = 0) const {
        if (!str || !string.str || string.length() == 0)
            return (start <= len) ? start : npos;

        if (string.length() > len || start >= len)
            return npos;

        for (size_t i = start; i <= len - string.length(); i++) {
            bool match = true;
            for (size_t j = 0; j < string.length(); j++) {
                if (str[i + j] != string.str[j]) {
                    match = false;
                    break;
                }
            }

            if (match)
                return i;
        }

        return npos;
    }

    void assign(const char ch) {
        if (str) {
            free(str);
            str = nullptr;
        }

        len = 1;
        str = (char*)malloc(2);
        str[0] = ch;
        str[1] = 0;
    }

    void assign(const char* string) {
        if (str) {
            free(str);
            str = nullptr;
        }

        len = strlen(string);
        str = (char*)malloc(len * sizeof(char) + 1);
        memzero(str, len + 1);
        memcpy(str, string, len);
    }

    void assign(const std::string& other) {
        if (this == &other)
            return;

        if (str) {
            free(str);
            str = nullptr;
        }

        len = other.len;
        str = (char*)malloc(len * sizeof(char) + 1);
        strncpy(str, other.c_str(), len);
        str[len] = 0;
    }
    
    void append(const char* other) {
        size_t other_len = strlen(other);
        size_t new_len = len + other_len;
        
        char* new_str = (char*)malloc(new_len + 1);
        memzero(new_str, new_len + 1);
        memcpy(new_str, str, len);
        memcpy(new_str + len, other, other_len);

        free(str);
        str = new_str;
        len = new_len;
    }

    void append(const std::string& other) {
        append(other.str);
    }
    
    bool operator==(const std::string& other) const { return streq(str, other.str); }
    bool operator==(const char* other) const { return streq(str, other); }
    bool operator!=(const std::string& other) const { return !streq(str, other.str); }
    bool operator!=(const char* other) const { return !streq(str, other); }
    
    string operator+(const std::string& other) const {
        string n(str);
        n.append(other);
        return n;
    }

    string operator+(const char* other) const {
        string n(str);
        n.append(other);
        return n;
    }

    string operator+(const char other) const {
        string n(str);
        n.append(other);
        return n;
    }

    string& operator+=(const std::string& other) {
        append(other);
        return *this;
    }

    string& operator+=(const char* other) {
        append(other);
        return *this;
    }

    string& operator+=(const char other) {
        append(other);
        return *this;
    }

private:
    char* str = nullptr;
    size_t len = 0;
};

} // namespace std

static std::string size_format_to_string(size_t size) {
    constexpr const char* sizes[] { "B", "KB", "MB", "GB", "TB" };
    size_t size_index = 0;
    double size_current = size;
    while (size_current > 1024.0 && size_index < ARRAY_LENGTH(sizes) - 1) {
        size_index++;
        size_current /= 1024.0;
    }

    char buffer[256];
    sprintf(buffer, sizeof(buffer), "%f%s", size_current, sizes[size_index]);
    return std::string(buffer);
}

static std::string time_format_to_string(size_t ms) {
    uint64_t total_seconds = ms / 1000;
    uint64_t seconds = total_seconds % 60;
    uint64_t minutes = (total_seconds / 60) % 60;
    uint64_t hours = (total_seconds / 3600) % 24;
    uint64_t days = total_seconds / 86400;

    char buffer[256];

    if (days > 0) {
        sprintf(buffer, sizeof(buffer), "%uld %ulh %ulm %uls", days, hours, minutes, seconds);
    } else if (hours > 0) {
        sprintf(buffer, sizeof(buffer), "%ulh %zum %uls", hours, minutes, seconds);
    } else if (minutes > 0) {
        sprintf(buffer, sizeof(buffer), "%ulm %uls", minutes, seconds);
    } else {
        sprintf(buffer, sizeof(buffer), "%uls", seconds);
    }

    return std::string(buffer);
}

static std::dynamic_array<std::string> str_split(const std::string& instr, char ch) {
    std::dynamic_array<std::string> parts {};
    // on average there will most likely be at least 5 items in the array
    // we can kinda optimize it here, mostly for the vfs :)
    parts.resize(5);

    size_t i = 0;
    const size_t len = instr.length();
    const char* str = instr.c_str();

    while (i < len) {
        // Skip leading separators
        while (i < len && str[i] == ch)
            i++;

        if (i == len)
            break;

        size_t start = i;

        while (i < len && str[i] != ch)
            i++;

        if (i > start) {
            parts.insert_back(instr.substr(start, i - start));
        }
    }

    return parts;
}

#endif // __STRING_HPP__