#include "string.hpp"

#include <stdarg.h>
#include "memory/heap.hpp"

double pow(double base, int exponent) {
    if (exponent == 0) return 1;

    bool is_negative = exponent < 0;
    if (is_negative) exponent = -exponent;

    double result = 1.0;

    while (exponent > 0) {
        if (exponent % 2 == 1) {
            result *= base;
        }
        base *= base;
        exponent /= 2;
    }

    return is_negative ? 1.0 / result : result;
}

size_t strlen(const char* p_str) {
    if (p_str == nullptr || *p_str == '\0')
        return 0;

    size_t len = 0;
    while (p_str[len] != '\0')
        len++;

    return len;
}

size_t sprintf(char* p_buffer, size_t size, void* p_ptr) {
    if (size < 17 || p_buffer == nullptr)
        return 0;

    static const char s_numbers[] = "0123456789abcdef";

    uint64_t number = (uint64_t)p_ptr;

    memset(p_buffer, '0', 16ul);

    int i = 16;
    for (; number && i; number /= 16)
        p_buffer[--i] = s_numbers[number % 16];

    return 17;
}

size_t sprintf(char* p_buffer, size_t size, uint32_t num, int base) {
    if (size < 11 || p_buffer == nullptr)
        return 0;

    static const char s_numbers[] = "0123456789abcdef";

    if (base >= sizeof(s_numbers))
        return 0;

    if (num == 0) {
        p_buffer[0] = '0';
        p_buffer[1] = '\0';
        return 2;
    }

    uint32_t number = num;

    char tmp[11] = { 0 };
    int i = sizeof(tmp) - 1;
    for (; number && i; number /= base)
        tmp[--i] = s_numbers[number % base];

    char* p_str = &tmp[i];
    auto strlength = strlen(p_str);

    memcpy(p_buffer, &tmp[i], strlength);

    return strlength + 1;
}

size_t sprintf(char* p_buffer, size_t size, int32_t num, int base) {
    if (size < 12 || p_buffer == nullptr)
        return 0;

    static const char s_numbers[] = "0123456789abcdef";

    if (base >= sizeof(s_numbers))
        return 0;

    if (num == 0) {
        p_buffer[0] = '0';
        p_buffer[1] = '\0';
        return 2;
    }

    uint32_t number = (num < 0) ? -((uint32_t)num) : (uint32_t)num;

    char tmp[12] = { 0 };
    int i = sizeof(tmp) - 1;
    for (; number && i; number /= base)
        tmp[--i] = s_numbers[number % base];

    if (num < 0)
        tmp[--i] = '-';

    char* p_str = &tmp[i];
    auto strlength = strlen(p_str);

    memcpy(p_buffer, &tmp[i], strlength);

    return strlength + 1;
}

size_t sprintf(char* p_buffer, size_t size, uint64_t num, int base) {
    if (size < 21 || p_buffer == nullptr)
        return 0;

    static const char s_numbers[] = "0123456789abcdef";

    if (base >= sizeof(s_numbers))
        return 0;

    if (num == 0) {
        p_buffer[0] = '0';
        p_buffer[1] = '\0';
        return 2;
    }

    uint64_t number = num;

    char tmp[21] = { 0 };
    int i = sizeof(tmp) - 1;
    for (; number && i; number /= base)
        tmp[--i] = s_numbers[number % base];

    char* str = &tmp[i];
    auto strlength = strlen(str);

    memcpy(p_buffer, &tmp[i], strlength);

    return strlength + 1;
}

size_t sprintf(char* p_buffer, size_t size, int64_t num, int base) {
    if (size < 22 || p_buffer == nullptr)
        return 0;

    static const char s_numbers[] = "0123456789abcdef";

    if (base >= sizeof(s_numbers))
        return 0;

    if (num == 0) {
        p_buffer[0] = '0';
        p_buffer[1] = '\0';
        return 2;
    }

    uint64_t number = (num < 0) ? -((uint64_t)num) : (uint64_t)num;

    char tmp[22] = { 0 };
    int i = sizeof(tmp) - 1;
    for (; number && i; number /= base)
        tmp[--i] = s_numbers[number % base];

    if (num < 0)
        tmp[--i] = '-';

    char* str = &tmp[i];
    auto strlength = strlen(str);

    memcpy(p_buffer, &tmp[i], strlength);

    return strlength + 1;
}

size_t sprintf(char* p_buffer, size_t size, double num, int precision) {
    if (p_buffer == nullptr)
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

    memcpy(&p_buffer[0], inc_buff, inc_buff_size - 1);
    p_buffer[inc_buff_size - 1] = '.';
    memcpy(&p_buffer[inc_buff_size], dec_buff, dec_buff_size);

    return inc_buff_size - 1 + dec_buff_size - 1 + 2;
}

size_t sprintf(char* p_buffer, size_t size, const char* p_fmt, va_list p_args) {
    size_t len = 0;
    size_t string_length = strlen(p_fmt);

    for (size_t i = 0; i < string_length; i++) {
        const char& ch = p_fmt[i];

        if (ch == '%') {
            if (i + 1 < string_length) {
                i++;
            } else {
                if (STR_CHECK_BUFF(len, size, 1))
                    p_buffer[len++] = '%';
                continue;
            }

            const char& ch2 = p_fmt[i];
            switch (ch2) {
                case 'p': {
                    if (STR_CHECK_BUFF(len, size, 16)) {
                        char tmpbuf[17] = { 0 };
                        sprintf(tmpbuf, 17, va_arg(p_args, void*));
                        memcpy(&p_buffer[len], tmpbuf, 16);
                        len += 16;
                    }
                    break;
                }
                case 's': {
                    char* arg = va_arg(p_args, char*);
                    auto strlength = strlen(arg);
                    if (STR_CHECK_BUFF(len, size, strlength)) {
                        memcpy(&p_buffer[len], arg, strlength);
                        len += strlength;
                    }
                    break;
                }
                case 'c': {
                    char arg = (char)va_arg(p_args, int);
                    if (STR_CHECK_BUFF(len, size, 1)) {
                        memcpy(&p_buffer[len], &arg, 1);
                        len += 1;
                    }
                    break;
                }
                case 'u': {
                    if (i + 1 < string_length) {
                        if (p_fmt[i+1] == 'l') {
                            i++;
                            char tmpbuf[21] = { 0 };
                            auto strsize = sprintf(tmpbuf, 21, va_arg(p_args, uint64_t)) - 1;
                            if (STR_CHECK_BUFF(len, size, strsize)) {
                                memcpy(&p_buffer[len], tmpbuf, strsize);
                                len += strsize;
                            }

                            break;
                        }

                        if (p_fmt[i+1] == 'h') {
                            i++;
                            char tmpbuf[21] = { 0 };
                            auto strsize = sprintf(tmpbuf, 21, va_arg(p_args, uint64_t), 16) - 1;
                            if (STR_CHECK_BUFF(len, size, strsize)) {
                                memcpy(&p_buffer[len], tmpbuf, strsize);
                                len += strsize;
                            }

                            break;
                        }
                    }
                    char tmpbuf[11] = { 0 };
                    auto strsize = sprintf(tmpbuf, 11, va_arg(p_args, uint32_t)) - 1;
                    if (STR_CHECK_BUFF(len, size, strsize)) {
                        memcpy(&p_buffer[len], tmpbuf, strsize);
                        len += strsize;
                    }
                    break;
                }
                case 'i': {
                    if (i + 1 < string_length) {
                        if (p_fmt[i+1] == 'l') {
                            i++;
                            char tmpbuf[22] = { 0 };
                            auto strsize = sprintf(tmpbuf, 22, va_arg(p_args, int64_t)) - 1;
                            if (STR_CHECK_BUFF(len, size, strsize)) {
                                memcpy(&p_buffer[len], tmpbuf, strsize);
                                len += strsize;
                            }

                            break;
                        }

                        if (p_fmt[i+1] == 'h') {
                            i++;
                            char tmpbuf[22] = { 0 };
                            auto strsize = sprintf(tmpbuf, 22, va_arg(p_args, int64_t), 16) - 1;
                            if (STR_CHECK_BUFF(len, size, strsize)) {
                                memcpy(&p_buffer[len], tmpbuf, strsize);
                                len += strsize;
                            }

                            break;
                        }
                    }
                    char tmpbuf[12] = { 0 };
                    auto strsize = sprintf(tmpbuf, 12, va_arg(p_args, int32_t)) - 1;
                    if (STR_CHECK_BUFF(len, size, strsize)) {
                        memcpy(&p_buffer[len], tmpbuf, strsize);
                        len += strsize;
                    }
                    break;
                }
                case 'f': {
                    char tmpbuf[50] = { 0 };
                    auto strsize = sprintf(tmpbuf, 50, va_arg(p_args, double)) - 1;
                    if (STR_CHECK_BUFF(len, size, strsize)) {
                        memcpy(&p_buffer[len], tmpbuf, strsize);
                        len += strsize;
                    }
                    break;
                }
                default:
                    if (STR_CHECK_BUFF(len, size, 2)) {
                        p_buffer[len++] = '%';
                        p_buffer[len++] = ch2;
                    }
                    break;
            }

        } else {
            if (!STR_CHECK_BUFF(len, size, 1))
                break;
            
            p_buffer[len++] = ch;
        }
    }

    p_buffer[len] = '\0';
    len++;

    return len;
}

size_t sprintf(char* p_buffer, size_t size, const char* p_fmt, ...) {
    va_list args;
    va_start(args, p_fmt);
    size_t result = sprintf(p_buffer, size, p_fmt, args);
    va_end(args);

    return result;
}

bool strncpy(char* p_dest, const char* p_src, size_t dest_size) {
    size_t src_len = strlen(p_src);

    if (src_len > dest_size)
        return false;

    memcpy(p_dest, p_src, src_len);

    return true;
}

char* strchr(const char* p_s, char c) {
    while (*p_s) {
        if (*p_s == c)
            return (char*)p_s;
        p_s++;
    }

    if (c == '\0')
        return (char*)p_s;

    return nullptr;
}

bool streq(const char* p_str1, const char* p_str2) {
    while (*p_str1 && *p_str2 && *p_str1 == *p_str2) {
        p_str1++;
        p_str2++;
    }

    return *p_str1 == *p_str2;
}

int strff(const char* p_str, char ch) {
    for (int i = 0; p_str[i]; i++) {
        if (p_str[i] == ch)
            return i;
    }

    return -1;
}

bool str_start_with(const char* p_str1, const char* p_target) {
    while (*p_target) {
        if (*p_str1 != *p_target)
            return false;
        p_str1++;
        p_target++;
    }

    return true;
}

bool str_ends_with(const char* p_str, const char* p_target) {
    size_t str_len = strlen(p_str);
    size_t target_len = strlen(p_target);

    if (target_len > str_len)
        return false;

    return streq(p_str + str_len - target_len, p_target);
}

void unpack_be16_string(const uint16_t* src, int word_count, char* dst, int max_len) {
    int pos = 0;
    memzero(dst, max_len);

    for (int i = 0; i < word_count && pos + 1 < max_len; ++i) {
        dst[pos++] = (char)(src[i] >> 8);
        dst[pos++] = (char)(src[i] & 0xFF);
    }
    
    while (pos > 0 && dst[pos - 1] == ' ')
        pos--;

    dst[pos < max_len ? pos : max_len - 1] = '\0';
}

string::string() {
}

string::string(const char* p_string) {
    assign(p_string);
}

string::string(const string& other) {
    assign(move(other));
}

string::string(char ch) {
    if (p_str) {
        GFREE(p_str);
        p_str = nullptr;
        len = 0;
    }

    len = 1;
    p_str = (char*)GALLOC(len * 2);
    memzero(p_str, len + 1);
    p_str[0] = ch;
}

string& string::operator=(const string& other) {
    if (this == &other)
        return *this;

    char* new_str = (char*)GALLOC(other.len * sizeof(char) + 1);
    memzero(new_str, other.len * sizeof(char) + 1);
    strncpy(new_str, other.p_str, other.len);

    GFREE(p_str);

    p_str = new_str;
    len = other.len;

    return *this;
}

string::string(string&& other) {
    p_str = other.p_str;
    len = other.len;

    other.p_str = nullptr;
    other.len = 0;
}

string& string::operator=(string&& other) {
    if (this != &other) {
        if (p_str)
            GFREE(p_str);

        p_str = other.p_str;
        len = other.len;

        other.p_str = nullptr;
        other.len = 0;
    }
    return *this;
}

string::~string() {
    if (p_str) {
        GFREE(p_str);
        p_str = nullptr;
    }
}

const char* string::c_str() const {
    return p_str;
}

size_t string::length() const {
    return len;
}

string string::substr(size_t pos, size_t count) const {
    if (pos >= len)
        return string("");

    if (pos + count > len || count == k_npos)
        count = len - pos;

    char* buffer = (char*)GALLOC(count + 1);
    for (size_t i = 0; i < count; i++) {
        buffer[i] = p_str[pos + i];
    }
    buffer[count] = '\0';

    string result(buffer);
    GFREE(buffer);
    return result;
}

size_t string::find_last_of(char c) const {
    if (len == 0) {
        return k_npos; // not found
    }

    for (size_t i = len; i > 0; i--) {
        if (p_str[i - 1] == c) {
            return i - 1;
        }
    }

    return k_npos;
}

size_t string::find(const string& str, size_t start) const {
    if (!p_str || !str.p_str || str.length() == 0)
        return (start <= len) ? start : k_npos;

    if (str.length() > len || start >= len)
        return k_npos;

    for (size_t i = start; i <= len - str.length(); ++i) {
        bool match = true;
        for (size_t j = 0; j < str.length(); ++j) {
            if (p_str[i + j] != str.p_str[j]) {
                match = false;
                break;
            }
        }

        if (match)
            return i;
    }

    return k_npos;
}

void string::assign(const char* p_string) {
    if (p_str) {
        GFREE(p_str);
        p_str = nullptr;
        len = 0;
    }

    len = strlen(p_string);
    p_str = (char*)GALLOC(len * sizeof(char) + 1);
    memzero(p_str, len + 1);
    memcpy(p_str, p_string, len);
}

void string::assign(const string& other) {
    if (p_str) {
        GFREE(p_str);
        p_str = nullptr;
        len = 0;
    }

    len = other.len;
    p_str = (char*)GALLOC(len * sizeof(char) + 1);
    strncpy(p_str, other.c_str(), len);
    p_str[len] = '\0';
}

void string::append(const char* p_other) {
    size_t other_len = strlen(p_other);
    size_t new_len = len + other_len;
    
    char* p_new_str = (char*)GALLOC(new_len + 1);
    memzero(p_new_str, new_len + 1);
    memcpy(p_new_str, p_str, len);
    memcpy(p_new_str + len, p_other, other_len);

    GFREE(p_str);

    p_str = p_new_str;
    len = new_len;
}

void string::append(const string& r_other) {
    append(r_other.p_str);
}

bool string::operator==(const string& r_other) const {
    return streq(p_str, r_other.p_str);
}

bool string::operator==(const char* p_other) const {
    return streq(p_str, p_other);
}

string string::operator+(const string& r_other) {
    string n(p_str);
    n.append(r_other);
    return n;
}

string string::operator+(const char* p_other) {
    string n(p_str);
    n.append(p_other);
    return n;
}

string& string::operator+=(const string& r_other) {
    append(r_other);
    return *this;
}

string& string::operator+=(const char* p_other) {
    append(p_other);
    return *this;
}

string& string::operator=(const char* p_other) {
    assign(p_other);
    return *this;
}