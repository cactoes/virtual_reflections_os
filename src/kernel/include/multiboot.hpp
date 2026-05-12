//==========================================
/// @file       multiboot.hpp
/// @brief      multiboot stuff
//==========================================

#pragma once

#ifndef __MULTIBOOT_HPP__
#define __MULTIBOOT_HPP__

#define MULTIBOOT_MAGIC     0x2BADB002
#define MULTIBOOT2_MAGIC    0x36D76289

#define MULTIBOOT_VER_UNKOWN    0
#define MULTIBOOT_VER1          1
#define MULTIBOOT_VER2          2

#include "common.hpp"

struct multiboot1_mmap_entry_t {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} PACKED;

struct multiboot1_info_t {
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
} PACKED;

struct multiboot2_tag_framebuffer_t {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
    uint8_t  reserved;
    // ... missing color info
} PACKED;

struct multiboot2_mmap_entry_t {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
} PACKED;

struct multiboot2_tag_mmap_t {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    multiboot2_mmap_entry_t entries[];
} PACKED;

struct multiboot2_tag_t {
    uint32_t type;
    uint32_t size;
} PACKED;

struct multiboot2_info_t {
    uint32_t total_size;
    uint32_t reserved;
    multiboot2_tag_t tags[];
} PACKED;

struct multiboot_t {
    uint64_t magic;
    void* info;
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

enum class multiboot_tag_type_t : uint32_t {
    MMAP = 6,
    FRAMEBUFFER = 8,
};

/// @brief                          checks if the multiboot magic was valid
/// @param[in] multiboot_struct     pointer to mb struct
/// @return                         1 for version 1, 2 for version 2, 0 for invalid
int mb_has_valid_magic(multiboot_t* p_multiboot_struct);

/// @brief                          helper for looping over mb entries
/// @param[in] multiboot_struct     pointer to mb struct
/// @return                         pointer to first entry
multiboot1_mmap_entry_t* mb1_get_first_entry(multiboot_t* p_multiboot_struct);

/// @brief                          helper for looping over mb entries
/// @param[in] multiboot_struct     pointer to the mb struct
/// @param[in] prev                 pointer to last mme
/// @return                         pointer to next entry or nullptr if there are no more blocks
multiboot1_mmap_entry_t* mb1_get_next_entry(multiboot_t* p_multiboot_struct, multiboot1_mmap_entry_t* p_prev);

// NOT OPTIMIZED mb2 parsing
const multiboot2_mmap_entry_t* mb2_get_first_entry(multiboot_t* multiboot_struct);
const multiboot2_mmap_entry_t* mb2_get_next_entry(multiboot_t* multiboot_struct, const multiboot2_mmap_entry_t* prev);

#endif // __MULTIBOOT_HPP__