#include "string.hpp"
#include "memory.hpp"

#include <stdarg.h>

#define check_buff(cur, max, req) ((cur) + (req) < (max))

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

size_t strlen(const char* str) {
    if (str == nullptr || *str == '\0')
        return 0;

    size_t len = 0;
    while (str[len] != '\0')
        len++;

    return len;
}

size_t sprintf(char* buffer, size_t size, void* ptr) {
    if (size < 17 || buffer == nullptr)
        return 0;

    static const char numbers[] = "0123456789abcdef";

    uint64_t number = (uint64_t)ptr;

    memset(buffer, '0', 16ul);

    int i = 16;
    for (; number && i; number /= 16)
        buffer[--i] = numbers[number % 16];

    return 17;
}

size_t sprintf(char* buffer, size_t size, uint32_t num, int base) {
    if (size < 11 || buffer == nullptr)
        return 0;

    static const char numbers[] = "0123456789abcdef";

    if (base >= sizeof(numbers))
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
        tmp[--i] = numbers[number % base];

    char* str = &tmp[i];
    auto strlength = strlen(str);

    memcpy(buffer, &tmp[i], strlength);

    return strlength + 1;
}

size_t sprintf(char* buffer, size_t size, int32_t num, int base) {
    if (size < 12 || buffer == nullptr)
        return 0;

    static const char numbers[] = "0123456789abcdef";

    if (base >= sizeof(numbers))
        return 0;

    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return 2;
    }

    uint32_t number = (num < 0) ? -((uint32_t)num) : (uint32_t)num;

    char tmp[12] = { 0 };
    int i = sizeof(tmp) - 1;
    for (; number && i; number /= base)
        tmp[--i] = numbers[number % base];

    if (num < 0)
        tmp[--i] = '-';

    char* str = &tmp[i];
    auto strlength = strlen(str);

    memcpy(buffer, &tmp[i], strlength);

    return strlength + 1;
}

size_t sprintf(char* buffer, size_t size, uint64_t num, int base) {
    if (size < 21 || buffer == nullptr)
        return 0;

    static const char numbers[] = "0123456789abcdef";

    if (base >= sizeof(numbers))
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
        tmp[--i] = numbers[number % base];

    char* str = &tmp[i];
    auto strlength = strlen(str);

    memcpy(buffer, &tmp[i], strlength);

    return strlength + 1;
}

size_t sprintf(char* buffer, size_t size, int64_t num, int base) {
    if (size < 22 || buffer == nullptr)
        return 0;

    static const char numbers[] = "0123456789abcdef";

    if (base >= sizeof(numbers))
        return 0;

    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return 2;
    }

    uint64_t number = (num < 0) ? -((uint64_t)num) : (uint64_t)num;

    char tmp[22] = { 0 };
    int i = sizeof(tmp) - 1;
    for (; number && i; number /= base)
        tmp[--i] = numbers[number % base];

    if (num < 0)
        tmp[--i] = '-';

    char* str = &tmp[i];
    auto strlength = strlen(str);

    memcpy(buffer, &tmp[i], strlength);

    return strlength + 1;
}

size_t sprintf(char* buffer, size_t size, double num, int precision) {
    if (buffer == nullptr)
        return 0;

    static const char numbers[] = "0123456789";

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

size_t sprintf(char* buffer, size_t size, const char* fmt, va_list args) {
    size_t len = 0;
    size_t string_length = strlen(fmt);

    for (size_t i = 0; i < string_length; i++) {
        const char& ch = fmt[i];

        if (ch == '%') {
            if (i + 1 < string_length) {
                i++;
            } else {
                if (check_buff(len, size, 1))
                    buffer[len++] = '%';
                continue;
            }

            const char& ch2 = fmt[i];
            switch (ch2) {
                case 'p': {
                    if (check_buff(len, size, 16)) {
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
                    if (check_buff(len, size, strlength)) {
                        memcpy(&buffer[len], arg, strlength);
                        len += strlength;
                    }
                    break;
                }
                case 'c': {
                    char arg = (char)va_arg(args, int);
                    if (check_buff(len, size, 1)) {
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
                            if (check_buff(len, size, strsize)) {
                                memcpy(&buffer[len], tmpbuf, strsize);
                                len += strsize;
                            }

                            break;
                        }

                        if (fmt[i+1] == 'h') {
                            i++;
                            char tmpbuf[21] = { 0 };
                            auto strsize = sprintf(tmpbuf, 21, va_arg(args, uint64_t), 16) - 1;
                            if (check_buff(len, size, strsize)) {
                                memcpy(&buffer[len], tmpbuf, strsize);
                                len += strsize;
                            }

                            break;
                        }
                    }
                    char tmpbuf[11] = { 0 };
                    auto strsize = sprintf(tmpbuf, 11, va_arg(args, uint32_t)) - 1;
                    if (check_buff(len, size, strsize)) {
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
                            if (check_buff(len, size, strsize)) {
                                memcpy(&buffer[len], tmpbuf, strsize);
                                len += strsize;
                            }

                            break;
                        }

                        if (fmt[i+1] == 'h') {
                            i++;
                            char tmpbuf[22] = { 0 };
                            auto strsize = sprintf(tmpbuf, 22, va_arg(args, int64_t), 16) - 1;
                            if (check_buff(len, size, strsize)) {
                                memcpy(&buffer[len], tmpbuf, strsize);
                                len += strsize;
                            }

                            break;
                        }
                    }
                    char tmpbuf[12] = { 0 };
                    auto strsize = sprintf(tmpbuf, 12, va_arg(args, int32_t)) - 1;
                    if (check_buff(len, size, strsize)) {
                        memcpy(&buffer[len], tmpbuf, strsize);
                        len += strsize;
                    }
                    break;
                }
                case 'f': {
                    char tmpbuf[50] = { 0 };
                    auto strsize = sprintf(tmpbuf, 50, va_arg(args, double)) - 1;
                    if (check_buff(len, size, strsize)) {
                        memcpy(&buffer[len], tmpbuf, strsize);
                        len += strsize;
                    }
                    break;
                }
                default:
                    if (check_buff(len, size, 2)) {
                        buffer[len++] = '%';
                        buffer[len++] = ch2;
                    }
                    break;
            }

        } else {
            if (!check_buff(len, size, 1))
                break;
            
            buffer[len++] = ch;
        }
    }

    buffer[len] = '\0';
    len++;

    return len;
}

size_t sprintf(char* buffer, size_t size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    size_t result = sprintf(buffer, size, fmt, args);
    va_end(args);

    return result;
}