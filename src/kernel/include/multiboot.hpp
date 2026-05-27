//==========================================
/// @file       multiboot.hpp
/// @brief      multiboot stuff
//==========================================

#pragma once

#ifndef __MULTIBOOT_HPP__
#define __MULTIBOOT_HPP__

#define MULTIBOOT2_FRAMEBUFFER_TYPE_INDEXED     0
#define MULTIBOOT2_FRAMEBUFFER_TYPE_RGB         1
#define MULTIBOOT2_FRAMEBUFFER_TYPE_EGA_TEXT    2

#define MULTIBOOT2_FRAMEBUFFER_BPP_32BIT        32
#define MULTIBOOT2_FRAMEBUFFER_BPP_24BIT        24

#define MULTIBOOT_MAGIC     0x2BADB002
#define MULTIBOOT2_MAGIC    0x36D76289

#define MULTIBOOT_VER_UNKOWN    0
#define MULTIBOOT_VER1          1
#define MULTIBOOT_VER2          2

#include "common.hpp"

struct multiboot1_mmap_entry_t {
    u32 size;
    u64 addr;
    u64 len;
    u32 type;
} PACKED;

struct multiboot1_info_t {
    u32 flags;
    u32 mem_lower;
    u32 mem_upper;
    u32 boot_device;
    u32 cmdline;
    u32 mods_count;
    u32 mods_addr;
    u32 num;
    u32 size;
    u32 addr;
    u32 shndx;
    u32 mmap_length;
    u32 mmap_addr;
    u32 drives_length;
    u32 drives_addr;
    u32 config_table;
    u32 boot_loader_name;
    u32 apm_table;
    u32 vbe_control_info;
    u32 vbe_mode_info;
    u32 vbe_mode;
    u32 vbe_interface_seg;
    u32 vbe_interface_off;
    u32 vbe_interface_len;
} PACKED;

struct multiboot2_tag_framebuffer_t {
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
} PACKED;

struct multiboot2_mmap_entry_t {
    u64 addr;
    u64 len;
    u32 type;
    u32 zero;
} PACKED;

struct multiboot2_tag_mmap_t {
    u32 type;
    u32 size;
    u32 entry_size;
    u32 entry_version;
    multiboot2_mmap_entry_t entries[];
} PACKED;

struct multiboot2_tag_t {
    u32 type;
    u32 size;
} PACKED;

struct multiboot2_info_t {
    u32 total_size;
    u32 reserved;
    multiboot2_tag_t tags[];
} PACKED;

// struct multiboot_t {
//     // u64 magic;
//     void* info;
// };

enum class memory_map_type_t : u32 {
    UNKOWN           = 0,
    USABLE           = 1,
    RESERVED         = 2,
    ACPI_RECLAIMABLE = 3,
    ACPI_NVS         = 4
};

enum class multiboot_flags_t : u32 {
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

enum class multiboot_tag_type_t : u32 {
    MMAP = 6,
    FRAMEBUFFER = 8,
};

/// @brief                          checks if the multiboot magic was valid
/// @param[in] multiboot_struct     pointer to mb struct
/// @return                         1 for version 1, 2 for version 2, 0 for invalid
// int mb_has_valid_magic(multiboot_t* p_multiboot_struct);

/// @brief                          helper for looping over mb entries
/// @param[in] multiboot_struct     pointer to mb struct
/// @return                         pointer to first entry
multiboot1_mmap_entry_t* mb1_get_first_entry(multiboot2_info_t* p_multiboot_struct);

/// @brief                          helper for looping over mb entries
/// @param[in] multiboot_struct     pointer to the mb struct
/// @param[in] prev                 pointer to last mme
/// @return                         pointer to next entry or nullptr if there are no more blocks
multiboot1_mmap_entry_t* mb1_get_next_entry(multiboot2_info_t* p_multiboot_struct, multiboot1_mmap_entry_t* p_prev);

// NOT OPTIMIZED mb2 parsing
const multiboot2_mmap_entry_t* mb2_get_first_entry(multiboot2_info_t* multiboot_struct);
const multiboot2_mmap_entry_t* mb2_get_next_entry(multiboot2_info_t* multiboot_struct, const multiboot2_mmap_entry_t* prev);

multiboot2_tag_framebuffer_t* mb2_get_framebuffer(multiboot2_info_t* multiboot_struct);

#endif // __MULTIBOOT_HPP__