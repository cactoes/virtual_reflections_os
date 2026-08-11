//==========================================
/// @file       kstring.hpp
/// @brief      kernel string impl
//==========================================

#pragma once

#ifndef __KSTRING_HPP__
#define __KSTRING_HPP__

#define KSTRING_MAX_LEN         MAX_UINT64
#define STATIC_KSTRING(str)     (kstring_t){ (i8*)str, (u64)(sizeof(str) - 1), false }

#include "common.hpp"
#include "memory/heap.hpp"

/// @brief          returns length of string
/// @param[in] str  string to get length of
/// @return         length of string excluding nullterminator
/// @remarks        replacement for the std/string import
static inline
u64 strlen_internal(const char* cstr) {
    u64 len = 0;
    while (cstr && cstr[len] != '\0')
        len++;

    return len;
}

struct kstring_t {
    u64 length;

    // actual string which is NOT null terminated
    // if is_heap == true it needs to be free'd
    // for safety alway try to free it if you are done with it.
    i8* buffer;
    bool is_heap;

    // for simple compatibility we have these
    kstring_t() : length(0), buffer(nullptr), is_heap(false) {};
    kstring_t(const char* cstr) : length(strlen_internal(cstr)), buffer((i8*)cstr), is_heap(false) {};
    kstring_t(i8* buff, u64 len, bool heap) : length(len), buffer(buff), is_heap(heap) {}
};

/// @brief 
/// @param data 
/// @param size 
/// @return 
/// @remarks            its copy on write
static
kstring_t kstring_create(const i8* data, u64 size) {
    if (!data || size == 0)
        return {};

    kstring_t kstr { (i8*)malloc(size), size, true };

    if (!kstr.buffer) {
        kstr.is_heap = false;
        kstr.length = 0;
        return kstr;
    }

    memcpy(kstr.buffer, data, size);

    return kstr;
}

static
void kstring_destroy(kstring_t* kstr) {
    if (!kstr || !kstr->buffer || !kstr->is_heap)
        return;

    free(kstr->buffer);

    kstr->buffer = nullptr;
    kstr->is_heap = false;
    kstr->length = 0;
}

static
kstring_t kstring_copy(const kstring_t* src) {
    return kstring_create(src->buffer, src->length);
}

static
bool kstring_equals(const kstring_t* kstr1, const kstring_t* kstr2) {
    if (!kstr1 || !kstr2 || kstr1->length != kstr2->length)
        return false;

    return memeq(kstr1->buffer, kstr2->buffer, kstr1->length);
}

static
kstring_t kstring_substring(const kstring_t* kstr, u64 pos, u64 count = KSTRING_MAX_LEN) {
    if (!kstr)
        return {};

    if (pos >= kstr->length)
        return {};

    if (count == KSTRING_MAX_LEN || pos + count >= kstr->length)
        count = kstr->length - pos;

    return kstring_create(kstr->buffer + pos, count);
}

/// @brief          converts a kstring into a cstring
/// @param kstr     pointer to kstring struct
/// @return         pointer to a cstring which HAS to be free'd 
static
char* kstring_to_cstring(const kstring_t* kstr) {
    if (!kstr)
        return nullptr;

    char* cstr = (char*)malloc(kstr->length + 1);
    memcpy(cstr, kstr->buffer, kstr->length);
    cstr[kstr->length] = '\0';
    return cstr;
}

static
void kstring_append_simple(kstring_t* kstr, const i8* data, u64 size) {
    if (!kstr || !data)
        return;

    u64 new_length = kstr->length + size;

    i8* newbuffer = (i8*)malloc(new_length);
    if (!newbuffer)
        return;

    memcpy(newbuffer, kstr->buffer, kstr->length);
    memcpy((u8*)newbuffer + kstr->length, data, size);

    if (kstr->is_heap)
        free(kstr->buffer);

    kstr->buffer = newbuffer;
    kstr->length = new_length;
    kstr->is_heap = true;
}

static
void kstring_append(kstring_t* kstr, const char* cstr) {
    return kstring_append_simple(kstr, (const i8*)cstr, strlen_internal(cstr));
}

static
void kstring_append(kstring_t* kstr, const kstring_t* kstr_src) {
    return kstring_append_simple(kstr, kstr_src->buffer, kstr_src->length);
}

static
std::dynamic_array<kstring_t> kstring_split(const kstring_t* kstr, char ch) {
    if (!kstr)
        return {};

    std::dynamic_array<kstring_t> parts {};

    u64 i = 0;
    const u64 len = kstr->length;
    const i8* str = kstr->buffer;

    while (i < len) {
        while (i < len && str[i] == ch)
            i++;

        if (i == len)
            break;

        u64 start = i;

        while (i < len && str[i] != ch)
            i++;

        if (i > start)
            parts.insert_back(kstring_substring(kstr, start, i - start));
    }

    return parts;
}

#endif // __KSTRING_HPP__