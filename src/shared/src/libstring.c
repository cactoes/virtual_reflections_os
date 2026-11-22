#include "libstring.h"

u64 strlen(const char* str) {
    const char* p = str;
    while (*p)
        p++;

    return (u64)(p - str);
}

static void out_char(fmt_out_t* o, char c) {
    if (o->pos + 1 < o->size)
        o->buf[o->pos] = c;
    o->pos++;
}

static void out_pad(fmt_out_t* o, int count, char c) {
    for (int i = 0; i < count; i++)
        out_char(o, c);
}

static void out_number(fmt_out_t* o, u64 value, u32 base, bool upper, bool negative, int width, bool zero, bool left) {
    const char* digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char tmp[24];
    int n = 0;

    do {
        tmp[n++] = digits[value % base];
        value /= base;
    } while (value);

    const int len = n + (negative ? 1 : 0);
    const int pad = width > len ? width - len : 0;

    if (!left && !zero)
        out_pad(o, pad, ' ');

    if (negative)
        out_char(o, '-');

    if (!left && zero)
        out_pad(o, pad, '0');

    while (n)
        out_char(o, tmp[--n]);

    if (left)
        out_pad(o, pad, ' ');

}

static void out_string(fmt_out_t* o, const char* s, int width, bool left) {
    if (!s)
        s = "(null)";

    int len = 0;
    while (s[len]) len++;

    const int pad = width > len ? width - len : 0;
    if (!left)
        out_pad(o, pad, ' ');

    for (int i = 0; i < len; i++)
        out_char(o, s[i]);

    if (left)
        out_pad(o, pad, ' ');
}

u64 vsprintf(char* buf, u64 size, const char* fmt, va_list ap) {
    fmt_out_t o = { buf, size, 0 };

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            out_char(&o, *fmt);
            continue;
        }

        fmt++;

        bool left = false, zero = false;
        for (;; fmt++) {
            if (*fmt == '-')
                left = true;
            else if (*fmt == '0')
                zero = true;
            else
                break;
        }

        int width = 0;
        if (*fmt == '*') {
            width = va_arg(ap, int);
            if (width < 0) {
                left = true;
                width = -width;
            }
            fmt++;
        } else {
            while (*fmt >= '0' && *fmt <= '9')
                width = width * 10 + (*fmt++ - '0');
        }

        int len_mod = 0;
        if (*fmt == 'z')      {
            len_mod = 1;
            fmt++;
        } else if (*fmt == 'l') {
            len_mod = 1; fmt++;
            if (*fmt == 'l') {
                len_mod = 2;
                fmt++;
            }
        }

        switch (*fmt) {
            case 'd':
            case 'i': {
                long long v = len_mod == 2 ? va_arg(ap, long long) : len_mod == 1 ? va_arg(ap, long) : va_arg(ap, int);
                const bool neg = v < 0;
                out_number(&o, neg ? 0 - (u64)v : (u64)v, 10, false, neg, width, zero, left);
                break;
            }
            case 'u':
            case 'x':
            case 'X': {
                u64 v = len_mod == 2 ? va_arg(ap, unsigned long long) : len_mod == 1 ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
                out_number(&o, v, *fmt == 'u' ? 10 : 16, *fmt == 'X', false, width, zero, left);
                break;
            }
            case 'p':
                out_char(&o, '0');
                out_char(&o, 'x');
                out_number(&o, (u64)va_arg(ap, void*), 16, false, false, 16, true, false);
                break;
            case 'c':
                out_char(&o, (char)va_arg(ap, int));
                break;
            case 's':
                out_string(&o, va_arg(ap, const char*), width, left);
                break;
            case '%':
                out_char(&o, '%');
                break;
            case '\0':
                fmt--;
                break;
            default:
                out_char(&o, '%');
                out_char(&o, *fmt);
                break;
        }
    }

    if (size)
        buf[o.pos < size ? o.pos : size - 1] = '\0';

    return o.pos;
}

u64 sprintf(char* buf, u64 size, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    const u64 n = vsprintf(buf, size, fmt, ap);
    va_end(ap);
    return n;
}