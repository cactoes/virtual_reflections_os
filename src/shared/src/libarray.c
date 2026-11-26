#include "libarray.h"
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

bool static_array_init(static_array_t* array, u64 capacity, u64 element_size) {
    if (!array || capacity == 0 || element_size == 0)
        return false;

    array->capacity = capacity;
    array->data = __malloc_impl(capacity * element_size);
    if (!array->data)
        return false;

    return true;
}

void static_array_free(static_array_t* array) {
    if (array && array->data) {
        __free_impl(array->data);
        array->data = nullptr;
        array->capacity = 0;
    }
}

bool static_array_insert_at(static_array_t* array, u64 index, const void* element) {
    if (!array || !element || index >= array->capacity)
        return false;
    
    void* dst = (u8*)array->data + (index * array->element_size);
    memcpy(dst, element, array->element_size);

    return true;
}

bool static_array_remove(static_array_t* array, u64 index) {
    if (!array || index >= array->capacity)
        return false;

    void* dst = (u8*)array->data + (index * array->element_size);
    void* src = (u8*)array->data + ((index + 1) * array->element_size);
    u64 bytes_to_move = (array->capacity - index - 1) * array->element_size;

    if (bytes_to_move > 0)
        memcpy(dst, src, bytes_to_move);

    return true;
}

void* static_array_get(const static_array_t* array, u64 index) {
    if (!array || index >= array->capacity)
        return nullptr;

    return (u8*)array->data + (index * array->element_size);
}

bool linked_list_init(linked_list_head_t* list) {
    if (!list)
        return false;

    list->head = nullptr;
    list->tail = nullptr;
    list->size = 0;
    return true;
}

void linked_list_free(linked_list_head_t* list) {
    if (!list)
        return;

    linked_list_t* current = list->head;
    while (current) {
        linked_list_t* next = current->next;
        __free_impl(current);
        current = next;
    }

    list->head = nullptr;
    list->tail = nullptr;
    list->size = 0;
}

bool linked_list_insert_back(linked_list_head_t* list, void* data) {
    if (!list || !data)
        return false;

    linked_list_t* new_node = (linked_list_t*)__malloc_impl(sizeof(linked_list_t));
    if (!new_node)
        return false;

    new_node->data = data;
    new_node->next = nullptr;
    new_node->prev = list->tail;

    if (list->tail)
        list->tail->next = new_node;
    else
        list->head = new_node;

    list->tail = new_node;
    list->size++;
    return true;
}

bool linked_list_remove(linked_list_head_t* list, u64 index) {
    if (!list || index >= list->size)
        return false;

    linked_list_t* current = list->head;
    for (u64 i = 0; i < index; i++)
        current = current->next;

    if (current->prev)
        current->prev->next = current->next;
    else
        list->head = current->next;

    if (current->next)
        current->next->prev = current->prev;
    else
        list->tail = current->prev;

    __free_impl(current);
    list->size--;
    return true;
}

void* linked_list_get(const linked_list_head_t* list, u64 index) {
    if (!list || index >= list->size)
        return nullptr;

    linked_list_t* current = list->head;
    for (u64 i = 0; i < index; i++)
        current = current->next;

    return current->data;
}

bool dynamic_array_init(dynamic_array_t* array, u64 capacity, u64 element_size) {
    if (!array || capacity == 0 || element_size == 0)
        return false;

    array->size = 0;
    array->capacity = capacity;
    array->element_size = element_size;
    array->data = __malloc_impl(capacity * element_size);
    if (!array->data)
        return false;

    return true;
}

void dynamic_array_free(dynamic_array_t* array) {
    if (array && array->data) {
        __free_impl(array->data);
        array->data = nullptr;
        array->size = 0;
        array->capacity = 0;
        array->element_size = 0;
    }
}

bool dynamic_array_insert_back(dynamic_array_t* array, const void* element) {
    if (!array || !element)
        return false;

    if (array->size >= array->capacity) {
        if (!dynamic_array_resize(array, array->capacity * 2))
            return false;
    }

    void* dst = (u8*)array->data + (array->size * array->element_size);
    memcpy(dst, element, array->element_size);
    array->size++;
    return true;
}

bool dynamic_array_insert_at(dynamic_array_t* array, u64 index, const void* element) {
    if (!array || !element || index > array->size)
        return false;

    if (array->size >= array->capacity) {
        if (!dynamic_array_resize(array, array->capacity * 2))
            return false;
    }

    void* dst = (u8*)array->data + (index * array->element_size);
    void* src = (u8*)array->data + (index * array->element_size);
    u64 bytes_to_move = (array->size - index) * array->element_size;

    if (bytes_to_move > 0)
        memcpy((u8*)dst + array->element_size, src, bytes_to_move);

    memcpy(dst, element, array->element_size);
    array->size++;
    return true;
}

bool dynamic_array_remove(dynamic_array_t* array, u64 index) {
    if (!array || index >= array->size)
        return false;

    void* dst = (u8*)array->data + (index * array->element_size);
    void* src = (u8*)array->data + ((index + 1) * array->element_size);
    u64 bytes_to_move = (array->size - index - 1) * array->element_size;

    if (bytes_to_move > 0)
        memcpy(dst, src, bytes_to_move);

    array->size--;
    return true;
}

bool dynamic_array_resize(dynamic_array_t* array, u64 new_capacity) {
    if (!array || new_capacity == 0)
        return false;

    void* new_data = __malloc_impl(new_capacity * array->element_size);
    if (!new_data)
        return false;

    u64 bytes_to_copy = (array->size < new_capacity) ? (array->size * array->element_size) : (new_capacity * array->element_size);
    memcpy(new_data, array->data, bytes_to_copy);

    __free_impl(array->data);
    array->data = new_data;
    array->capacity = new_capacity;

    return true;
}

void* dynamic_array_get(const dynamic_array_t* array, u64 index) {
    if (!array || index >= array->size)
        return nullptr;

    return (u8*)array->data + (index * array->element_size);
}