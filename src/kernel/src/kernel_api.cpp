#include "kernel_api.hpp"
#include "memory/heap.hpp"

enum print_mode_t {
    STD,
    DBG
};

extern void printf(print_mode_t mode, const char* p_str, ...);

void kprint(const char* str) {
    printf(DBG, "[DRIVER] %s\n", str);
}