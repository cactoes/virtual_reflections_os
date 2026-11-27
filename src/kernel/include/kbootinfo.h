//==========================================
/// @file       kbootinfo.h
/// @brief      kernel boot info
//==========================================

#pragma once

#ifndef KBOOTINFO_H
#define KBOOTINFO_H

#define MEMORY_TYPE_MASK    0xfffull
#define MAX_MEMORY_REGIONS  128
#define MAX_EXTRA_VALUES    1
#define CMDLINE_MAX         256
#define MAX_MODULES         1
#define MODULE_STRING_MAX   256

#include "libtype.h"

typedef enum __memory_type {
    MEMORY_USABLE = 1 << 0,
    MEMORY_RESERVED = 1 << 1,
    MEMORY_RECLAIMABLE = 1 << 2
} memory_type_t;

typedef enum __framebuffer_format_t {
    FB_FORMAT_UNKNOWN = 0,
    FB_FORMAT_ARGB8888,
    FB_FORMAT_RGBA8888,
    FB_FORMAT_BGRA8888,
    FB_FORMAT_ABGR8888,
    FB_FORMAT_RGB888,
} framebuffer_format_t;

typedef enum __boot_media_type {
    MEDIA_TYPE_UNKNOWN = 0,
    MEDIA_TYPE_LIVE
} boot_media_type_t;

typedef struct __memory_region {
    u64 addr; // first 12 bits are memory_type_t
    u64 size;
} memory_region_t;

typedef struct __framebuffer {
    u64 physical_address;
    u64 width, height;
    u64 pitch;
    framebuffer_format_t format;
    bool valid;
} framebuffer_t;

typedef struct __memory_module {
    u64 pstart;
    u64 pend;
    char string[MODULE_STRING_MAX];
} memory_module_t;

typedef struct __kboot_info {
    struct memory_t {
        memory_region_t regions[MAX_MEMORY_REGIONS];
        u64 region_count;
        u64 size;
        bool region_overflowed;
    } memory;

    framebuffer_t framebuffer;

    boot_media_type_t boot_media_type;

    char cmdline[CMDLINE_MAX];
    bool cmdline_valid;

    u64 kernel_page_count;

    // architecture specific values for later use
    u32 extra_values_written;
    void* extra_values[MAX_EXTRA_VALUES];

    memory_module_t modules[MAX_MODULES];
    u64 module_count;
} kboot_info_t;

#endif // KBOOTINFO_H