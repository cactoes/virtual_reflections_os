#include "libstring.h"

u64 strlen(const char* str) {
    const char* p = str;

    while (*p)
        p++;

    return (u64)(p - str);
}