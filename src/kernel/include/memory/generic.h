//==========================================
/// @file       generic.h
/// @brief      generic (virtual) memory allocator implementation
//==========================================

#pragma once

#ifndef GENERIC_H
#define GENERIC_H

#define KERNEL_HEAP_BASE        0xfffff81000000000ull
#define KERNEL_HEAP_END         0xfffff83000000000ull
#define KERNEL_MMIO_BASE        0xfffff84000000000ull
#define KERNEL_MMIO_END         0xfffff86000000000ull

#define USER_SPACE_BASE         0x0000000000000000ull
#define USER_SPACE_END          0x00007fffffffffffull

#define VIRTUAL_HEAP_BLOCK_FREE     (1u << 0)
#define VIRTUAL_HEAP_BLOCK_ACCESSED (1u << 1)
#define VIRTUAL_HEAP_BLOCK_METADATA (1u << 2)

#include "libtype.h"
#include "memory/vmm.h" // for the VMEM_* defines

typedef struct __virtual_heap_block virtual_heap_block_t;
typedef struct __virtual_heap_block_table virtual_heap_block_table_t;

struct __virtual_heap_block {
    void* start;
    u64 size;
    virtual_heap_block_t* prev;
    virtual_heap_block_t* next;
    u64 flags;
};

struct __virtual_heap_block_table {
    // wierd calculation to ensure we keep this struct under 4kb
    virtual_heap_block_t blocks[(0x1000 - sizeof(virtual_heap_block_table_t*) * 2) / sizeof(virtual_heap_block_t)];
    virtual_heap_block_table_t* next;
    virtual_heap_block_table_t* prev;
};

typedef struct __virtual_heap {
    u64 heap_start, heap_limit;
    // current edge of virtual addresses
    u64 heap_edge;
    u64 heap_size_current;
    virtual_heap_block_table_t* table;
} virtual_heap_t;

typedef bool(*heap_block_filter_t)(const virtual_heap_block_t* block, const void* arg);

bool virtual_heap_create(void* vaddr_start, void* vaddr_limit, u64 start_size, virtual_heap_t* virtual_heap, u64 flags);
void* virtual_alloc_aligned(virtual_heap_t* virtual_heap, u64 size, u64 alignment);
void* virtual_alloc(virtual_heap_t* virtual_heap, u64 size);
bool virtual_free(virtual_heap_t* virtual_heap, void* ptr);

bool virtual_mmio_create(virtual_heap_t* mmio, void* vaddr_start, void* vaddr_limit);
void* virtual_mmio_map_ex(virtual_heap_t* mmio, void* paddr, u64 size, u64 flags);
void* virtual_mmio_map(virtual_heap_t* mmio, void* paddr, u64 size);
bool virtual_mmio_unmap(virtual_heap_t* mmio, void* ptr);

bool kmem_bind(void* heap_start, void* heap_limit, u64 start_size);
void* kalloc(u64 size);
void* kalloc_aligned(u64 size, u64 align);
void kfree(void* ptr);

bool kmmio_bind(void* heap_start, void* heap_limit);
void* kmmio_map(void* paddr, u64 size);
void kmmio_unmap(void* ptr);

static inline
bool is_aligned(u64 addr, u64 align) {
    return (addr & (align - 1)) == 0;
}

static inline
u64 align_up(u64 addr, u64 align) {
    return (addr + (align - 1)) & ~(align - 1);
}

static inline
u64 align_down(u64 addr, u64 align) {
    return (addr) & ~(align - 1);
}

#endif // GENERIC_H