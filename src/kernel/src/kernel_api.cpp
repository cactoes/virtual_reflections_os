#include "kernel_api.hpp"
#include "memory/heap.hpp"

enum print_mode_t {
    STD,
    DBG
};

extern void printf(print_mode_t mode, const char* p_str, ...);

void* kalloc(size_t size) {
    return heap_alloc(get_global_heap(), size);
}

void kfree(void* ptr) {
    heap_free(get_global_heap(), ptr);
}

void kprint(const char* str) {
    printf(DBG, "[DRIVER] %s\n", str);
}