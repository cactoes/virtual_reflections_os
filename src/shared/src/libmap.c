#include "libmap.h"
#include "libmem.h"

// forward declarations
void* __malloc_impl(u64 size);
void __free_impl(void* ptr);

bool bitmap_get(const u64* bitmap, u64 index) {
    return (bitmap[index / 64] >> (index % 64)) & 1;
}

void bitmap_set(u64* bitmap, u64 index, bool value) {
    if (value)
        bitmap[index / 64] |= (1ull << (index % 64));
    else
        bitmap[index / 64] &= ~(1ull << (index % 64));
}

void bitmap_fill(u64* bitmap, u64 words, bool value) {
    for (u64 i = 0; i < words; i++)
        bitmap[i] = value ? ~0ull : 0ull;
}

static
u64 hashmap_hash_u64(u64 value) {
    // SplitMix64 finalizer: distributes nearby integer keys well.
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ull;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebull;
    value ^= value >> 31;
    return value;
}

static
bool hashmap_allocate_entries(u64 capacity, u64 element_size, hashmap_entry_t** out_entries) {
    if (!out_entries || capacity == 0 || element_size == 0)
        return false;

    *out_entries = nullptr;

    const u64 max_u64 = (u64)-1;
    if (capacity > max_u64 / sizeof(hashmap_entry_t))
        return false;

    const u64 entries_bytes = capacity * sizeof(hashmap_entry_t);
    if (element_size > (max_u64 - entries_bytes) / capacity)
        return false;

    const u64 total_bytes = entries_bytes + capacity * element_size;
    hashmap_entry_t* entries = (hashmap_entry_t*)__malloc_impl(total_bytes);
    if (!entries)
        return false;

    u8* value_storage = (u8*)entries + entries_bytes;
    for (u64 i = 0; i < capacity; i++) {
        entries[i].key = 0;
        entries[i].value = value_storage + i * element_size;
        entries[i].state = HASHMAP_SLOT_EMPTY;
    }

    *out_entries = entries;
    return true;
}

static
bool hashmap_find_slot(const hashmap_t* map, u64 key, u64* out_index) {
    if (!out_index)
        return false;

    *out_index = map ? map->capacity : 0;
    if (!map || !map->entries || map->capacity == 0)
        return false;

    u64 index = hashmap_hash_u64(key) % map->capacity;
    u64 first_tombstone = map->capacity;

    for (u64 probes = 0; probes < map->capacity; probes++) {
        const hashmap_entry_t* entry = &map->entries[index];

        if (entry->state == HASHMAP_SLOT_EMPTY) {
            *out_index = first_tombstone != map->capacity ? first_tombstone : index;
            return false;
        }

        if (entry->state == HASHMAP_SLOT_OCCUPIED && entry->key == key) {
            *out_index = index;
            return true;
        }

        if (entry->state == HASHMAP_SLOT_TOMBSTONE &&
            first_tombstone == map->capacity) {
            first_tombstone = index;
        }

        index++;
        if (index == map->capacity)
            index = 0;
    }

    if (first_tombstone != map->capacity)
        *out_index = first_tombstone;

    return false;
}

static
bool hashmap_rehash(hashmap_t* map, u64 new_capacity) {
    if (!map || !map->entries || new_capacity < map->size || new_capacity == 0)
        return false;

    hashmap_entry_t* new_entries;
    if (!hashmap_allocate_entries(new_capacity, map->element_size, &new_entries))
        return false;

    for (u64 i = 0; i < map->capacity; i++) {
        const hashmap_entry_t* old_entry = &map->entries[i];
        if (old_entry->state != HASHMAP_SLOT_OCCUPIED)
            continue;

        u64 index = hashmap_hash_u64(old_entry->key) % new_capacity;
        while (new_entries[index].state == HASHMAP_SLOT_OCCUPIED) {
            index++;
            if (index == new_capacity)
                index = 0;
        }

        new_entries[index].key = old_entry->key;
        memcpy(new_entries[index].value, old_entry->value, map->element_size);
        new_entries[index].state = HASHMAP_SLOT_OCCUPIED;
    }

    __free_impl(map->entries);
    map->entries = new_entries;
    map->capacity = new_capacity;
    map->tombstones = 0;
    return true;
}

bool hashmap_init(hashmap_t* map, u64 capacity, u64 element_size) {
    if (!map)
        return false;

    map->entries = nullptr;
    map->capacity = 0;
    map->size = 0;
    map->tombstones = 0;
    map->element_size = 0;

    if (element_size == 0)
        return false;

    if (capacity < 8)
        capacity = 8;

    hashmap_entry_t* entries;
    if (!hashmap_allocate_entries(capacity, element_size, &entries))
        return false;

    map->entries = entries;
    map->capacity = capacity;
    map->element_size = element_size;
    return true;
}

void hashmap_free(hashmap_t* map) {
    if (map && map->entries) {
        __free_impl(map->entries);
        map->entries = nullptr;
        map->capacity = 0;
        map->size = 0;
        map->tombstones = 0;
        map->element_size = 0;
    }
}

bool hashmap_insert(hashmap_t* map, u64 key, void* value) {
    if (!map || !map->entries || map->capacity == 0 || map->element_size == 0 || !value)
        return false;

    u64 index;
    if (hashmap_find_slot(map, key, &index)) {
        memcpy(map->entries[index].value, value, map->element_size);
        return true;
    }

    const u64 load_threshold = map->capacity - map->capacity / 4;
    if (map->size + map->tombstones + 1 >= load_threshold) {
        u64 new_capacity = map->capacity;

        if (map->tombstones <= map->size) {
            if (map->capacity > (u64)-1 / 2)
                return false;

            new_capacity = map->capacity * 2;
        }

        if (!hashmap_rehash(map, new_capacity))
            return false;

        (void)hashmap_find_slot(map, key, &index);
    }

    if (index >= map->capacity)
        return false;

    hashmap_entry_t* entry = &map->entries[index];
    if (entry->state == HASHMAP_SLOT_TOMBSTONE)
        map->tombstones--;

    entry->key = key;
    memcpy(entry->value, value, map->element_size);
    entry->state = HASHMAP_SLOT_OCCUPIED;
    map->size++;
    return true;
}

bool hashmap_get(hashmap_t* map, u64 key, void* value) {
    if (!map || !map->entries || map->capacity == 0 ||
        map->element_size == 0 || !value)
        return false;

    u64 index;
    if (!hashmap_find_slot(map, key, &index))
        return false;

    memcpy(value, map->entries[index].value, map->element_size);
    return true;
}

bool hashmap_remove(hashmap_t* map, u64 key) {
    if (!map || !map->entries || map->capacity == 0)
        return false;

    u64 index;
    if (!hashmap_find_slot(map, key, &index))
        return false;

    hashmap_entry_t* entry = &map->entries[index];
    entry->state = HASHMAP_SLOT_TOMBSTONE;
    map->size--;
    map->tombstones++;
    return true;
}