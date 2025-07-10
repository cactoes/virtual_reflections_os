//==========================================
/// @file       multiboot.hpp
/// @brief      multiboot stuff
//==========================================

#pragma once

#ifndef __MULTIBOOT_HPP__
#define __MULTIBOOT_HPP__

#define MULTIBOOT_MAGIC 0x2BADB002

#include "common.hpp"

struct memory_map_entry_t {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} PACKED;

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
} PACKED;

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

/// @brief                          checks if the multiboot magic was valid
/// @param[in] multiboot_struct     pointer to mb struct
/// @return                         true if mb magic was valid or nullptr if there is no block
bool mb_has_valid_magic(multiboot_t* p_multiboot_struct);

/// @brief                          helper for looping over mb entries
/// @param[in] multiboot_struct     pointer to mb struct
/// @return                         pointer to first entry
memory_map_entry_t* mb_get_first_entry(multiboot_t* p_multiboot_struct);

/// @brief                          helper for looping over mb entries
/// @param[in] multiboot_struct     pointer to the mb struct
/// @param[in] prev                 pointer to last mme
/// @return                         pointer to next entry or nullptr if there are no more blocks
memory_map_entry_t* mb_get_next_entry(multiboot_t* p_multiboot_struct, memory_map_entry_t* p_prev);

#endif // __MULTIBOOT_HPP__