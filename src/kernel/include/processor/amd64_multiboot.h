//==========================================
/// @file       amd64_multiboot.h
/// @brief      
//==========================================

#pragma once

#ifndef AMD64_MULTIBOOT_H
#define AMD64_MULTIBOOT_H

#define MULTIBOOT2_FRAMEBUFFER_TYPE_INDEXED     0
#define MULTIBOOT2_FRAMEBUFFER_TYPE_RGB         1
#define MULTIBOOT2_FRAMEBUFFER_TYPE_EGA_TEXT    2

#define MULTIBOOT2_FRAMEBUFFER_BPP_32BIT        32
#define MULTIBOOT2_FRAMEBUFFER_BPP_24BIT        24

#include "libtype.h"
#include "libdef.h"

typedef struct __multiboot2_mmap_entry_t {
    u64 addr;
    u64 len;
    u32 type;
    u32 zero;
} __packed multiboot2_mmap_entry_t;

typedef struct __multiboot2_tag_mmap_t {
    u32 type;
    u32 size;
    u32 entry_size;
    u32 entry_version;
    multiboot2_mmap_entry_t entries[];
} __packed multiboot2_tag_mmap_t;

typedef struct __multiboot2_tag_cmdline_t {
    u32 type;
    u32 size;
    char string[];
} __packed multiboot2_tag_cmdline_t;

typedef struct __multiboot2_tag_t {
    u32 type;
    u32 size;
} __packed multiboot2_tag_t;

typedef struct __multiboot2_info_t {
    u32 total_size;
    u32 reserved;
    multiboot2_tag_t tags[];
} __packed multiboot2_info_t;

typedef struct __multiboot2_tag_framebuffer {
    u32 type;
    u32 size;

    u64 framebuffer_addr;
    u32 framebuffer_pitch;
    u32 framebuffer_width;
    u32 framebuffer_height;

    u8 framebuffer_bpp;
    u8 framebuffer_type;
    u16 reserved;

    union {      
        struct {
            u32 framebuffer_palette_num_colors;
        } indexed;

        struct {
            u8 framebuffer_red_field_position;
            u8 framebuffer_red_mask_size;
            u8 framebuffer_green_field_position;
            u8 framebuffer_green_mask_size;
            u8 framebuffer_blue_field_position;
            u8 framebuffer_blue_mask_size;
        } rgb;
    };
} __packed multiboot2_tag_framebuffer_t;

typedef enum __memory_map_type_t {
    MMT_UNKOWN = 0,
    MMT_USABLE = 1,
    MMT_RESERVED = 2,
    MMT_ACPI_RECLAIMABLE = 3,
    MMT_ACPI_NVS = 4
} memory_map_type_t;

typedef enum __multiboot_flags_t {
    MBF_MEM = (1 << 0),
    MBF_BOOT_DEVICE = (1 << 1),
    MBF_CMDLINE = (1 << 2),
    MBF_MODS = (1 << 3),
    MBF_AOUT_SYMS = (1 << 4),
    MBF_ELF_SYMS = (1 << 5),
    MBF_MMAP = (1 << 6),
    MBF_DRIVES = (1 << 7),
    MBF_CONFIG_TABLE = (1 << 8),
    MBF_BOOT_LOADER = (1 << 9),
    MBF_APM_TABLE = (1 << 10),
    MBF_VBE = (1 << 11)
} multiboot_flags_t;

typedef enum __multiboot_tag_type_t {
    MBTT_CMDLINE = 1,
    MBTT_MMAP = 6,
    MBTT_FRAMEBUFFER = 8,
} multiboot_tag_type_t;

const multiboot2_mmap_entry_t* mb2_get_first_entry(multiboot2_info_t* mbi);
const multiboot2_mmap_entry_t* mb2_get_next_entry(multiboot2_info_t* mbi, const multiboot2_mmap_entry_t* prev);
const multiboot2_tag_framebuffer_t* mb2_get_framebuffer_tag(multiboot2_info_t* mbi);
const multiboot2_tag_cmdline_t* mb2_get_cmdline_tag(multiboot2_info_t* mbi);

#endif // AMD64_MULTIBOOT_H