//==========================================
/// @file       libarray.h
/// @brief      array based helpers like: bitmap, ...
//==========================================

#pragma once

#ifndef LIBARRAY_H
#define LIBARRAY_H

#define BITMAP_WORDS(bits)  (((bits) + 63) / 64)

#include "libtype.h"

typedef struct __linked_list {
    void* data;
    struct __linked_list* next;
    struct __linked_list* prev;
} linked_list_t;

typedef struct __linked_list_head {
    linked_list_t* head;
    linked_list_t* tail;
    u64 size;
} linked_list_head_t;

typedef struct __static_array {
    u64 capacity;
    u64 element_size;
    void* data;
} static_array_t;

typedef struct __dynamic_array {
    u64 size;
    u64 capacity;
    u64 element_size;
    void* data;
} dynamic_array_t;

typedef struct __ring_buffer {
    u64 capacity;
    u64 element_size;
    u64 head;
    u64 tail;
    void* data;
} ring_buffer_t;

bool bitmap_get(const u64* bitmap, u64 index);
void bitmap_set(u64* bitmap, u64 index, bool value);
void bitmap_fill(u64* bitmap, u64 words, bool value);

bool static_array_init(static_array_t* array, u64 capacity, u64 element_size);
void static_array_free(static_array_t* array);
bool static_array_insert_at(static_array_t* array, u64 index, const void* element);
bool static_array_remove(static_array_t* array, u64 index);
void* static_array_get(const static_array_t* array, u64 index);

bool linked_list_init(linked_list_head_t* list);
void linked_list_free(linked_list_head_t* list);
bool linked_list_insert_back(linked_list_head_t* list, void* data);
bool linked_list_remove(linked_list_head_t* list, u64 index);
void* linked_list_get(const linked_list_head_t* list, u64 index);

bool dynamic_array_init(dynamic_array_t* array, u64 capacity, u64 element_size);
void dynamic_array_free(dynamic_array_t* array);
bool dynamic_array_insert_back(dynamic_array_t* array, const void* element);
bool dynamic_array_insert_at(dynamic_array_t* array, u64 index, const void* element);
bool dynamic_array_remove(dynamic_array_t* array, u64 index);
bool dynamic_array_resize(dynamic_array_t* array, u64 new_capacity);
void* dynamic_array_get(const dynamic_array_t* array, u64 index);

bool ring_buffer_init(ring_buffer_t* buffer, u64 capacity, u64 element_size);
void ring_buffer_free(ring_buffer_t* buffer);
bool ring_buffer_insert(ring_buffer_t* buffer, const void* element);
bool ring_buffer_peek(const ring_buffer_t* buffer, void* element);
bool ring_buffer_pop(ring_buffer_t* buffer, void* element);
bool ring_buffer_empty(const ring_buffer_t* buffer);
bool ring_buffer_full(const ring_buffer_t* buffer);

#endif // LIBARRAY_H