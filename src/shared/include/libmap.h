//==========================================
/// @file       libmap.h
/// @brief      
//==========================================

#pragma once

#ifndef LIBMAP_H
#define LIBMAP_H

#define BITMAP_WORDS(bits)  (((bits) + 63) / 64)

#define HASHMAP_SLOT_EMPTY 0
#define HASHMAP_SLOT_OCCUPIED 1
#define HASHMAP_SLOT_TOMBSTONE 2

#include "libtype.h"

typedef struct __hashmap_entry {
    u64 key;
    void* value;
    u8 state;
} hashmap_entry_t;

typedef struct __hashmap {
    hashmap_entry_t* entries;
    u64 capacity;
    u64 size;
    u64 tombstones;
    u64 element_size;
} hashmap_t;

bool bitmap_get(const u64* bitmap, u64 index);
void bitmap_set(u64* bitmap, u64 index, bool value);
void bitmap_fill(u64* bitmap, u64 words, bool value);

bool hashmap_init(hashmap_t* map, u64 capacity, u64 element_size);
void hashmap_free(hashmap_t* map);
bool hashmap_insert(hashmap_t* map, u64 key, void* value);
bool hashmap_get(hashmap_t* map, u64 key, void* value);
bool hashmap_remove(hashmap_t* map, u64 key);

#endif // LIBMAP_H