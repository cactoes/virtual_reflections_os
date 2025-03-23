//==========================================
/// @file       memory.hpp
/// @brief      kernel memory mapping logic
//==========================================

#pragma once

#ifndef __MEMORY_HPP__
#define __MEMORY_HPP__

#define KERNEL_PAGE_CRITICAL_BITMAP_SIZE 0x2000 // 1GB
#define KERNEL_PAGE_BITMAP_SIZE          0x4000 // 2GB

// helper functions for page tables
#define KPAGING_GET_PE(virtual_addr, offset)    ((((uint64_t)(virtual_addr)) >> (offset)) & 0x1FF)
#define KPAGING_GET_ENTRY(table, entry)         ((uint64_t*)(((uint64_t*)(table))[((uint64_t)(entry))] & ~0xFFF))
#define KPAGING_CHECK_ENTRY(table, entry)       ((((uint64_t*)(table))[((uint64_t)(entry))]) & 1)

// page flags
#define PF_PRESENT              (1 << 0)
#define PF_READ_WRITE           (1 << 1)
#define PF_USER_SUPERVISOR      (1 << 2)
#define PF_WRITE_THROUGH        (1 << 3)
#define PF_CACHE_DISABLE        (1 << 4)
#define PF_ACCESSED             (1 << 5)
#define PF_DIRTY                (1 << 6)
#define PF_PAGE_SIZE            (1 << 7)
#define PF_GLOBAL               (1 << 8)

#include "common.hpp"

struct heap_block_t {
    // start of virtual address
    void* start_real_addr;

    // size of memory block
    size_t size;

    // so we can easly deallocate
    heap_block_t* next;
    
    // bitfield
    struct {
        // is the memory region used
        bool free : 1;

        // is the struct used
        // if false all other fields are invalid
        bool used : 1;
    };
} __attribute__((packed));

struct heap_t {
    heap_block_t* heap_block_array;
    size_t heap_block_array_size;
    size_t heap_block_count;
};

struct memory_map_entry_t {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed)) ;

struct multiboot_info_t {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint32_t vbe_mode;
    uint32_t vbe_interface_seg;
    uint32_t vbe_interface_off;
    uint32_t vbe_interface_len;
} __attribute__((packed));

struct multiboot_t {
    uint64_t magic;
    multiboot_info_t* info;
};

enum class memory_map_type_t : uint32_t {
    UNKOWN           = 0,
    USABLE           = 1,
    RESERVED         = 2,
    ACPI_RECLAIMABLE = 3,
    ACPI_NVS         = 4
};

enum class multiboot_flags_t : uint32_t {
    MEM          = (1 << 0),
    BOOT_DEVICE  = (1 << 1),
    CMDLINE      = (1 << 2),
    MODS         = (1 << 3),
    AOUT_SYMS    = (1 << 4),
    ELF_SYMS     = (1 << 5),
    MMAP         = (1 << 6),
    DRIVES       = (1 << 7),
    CONFIG_TABLE = (1 << 8),
    BOOT_LOADER  = (1 << 9),
    APM_TABLE    = (1 << 10),
    VBE          = (1 << 11)
};

void* vmem_get_page_critical();
void* vmem_get_page();
bool vmem_paging_reserve_at_adress(uint64_t address, size_t count = 1);
void vmem_identity_map(uint64_t* pml4);
bool vmem_map_2kb_page(void* pml4, void* virtual_addr, void* physical_addr);
bool vmem_map_2mb_page(void* pml4, void* virtual_addr, void* physical_addr);
size_t vmem_smart_alloc_pages(void* pml4, void* virtual_addr, size_t size);
void vmem_init(multiboot_t* multiboot_struct, void* pml4);

void heap_init(heap_t* heap, void* pml4, void* virtual_address, size_t size);

void* heap_alloc(heap_t* heap, size_t size);
void heap_free(heap_t* heap, void* ptr);

bool mb_has_valid_magic(multiboot_t* multiboot_struct);
memory_map_entry_t* mb_get_first_entry(multiboot_t* multiboot_struct);
memory_map_entry_t* mb_get_next_entry(multiboot_t* multiboot_struct, memory_map_entry_t* prev);

void set_global_heap(heap_t* heap);
heap_t* get_global_heap();

void* operator new(size_t size);
// void* operator new[](size_t size);

void* operator new(size_t size, void* ptr);
// void* operator new[](size_t size, void* ptr);

void operator delete(void* ptr);
// void operator delete[](void* ptr);

void operator delete(void* ptr, size_t size);
// void operator delete[](void* ptr, size_t size);

#endif // __MEMORY_HPP__