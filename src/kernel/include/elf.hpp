//==========================================
/// @file       elf.hpp
/// @brief      elf stuff
//==========================================

#pragma once

#ifndef __ELF_HPP__
#define __ELF_HPP__

#define ELF_IDENTITY_SIZE               16

#define ELF_IDENTITY_MAGIC_0            0
#define ELF_IDENTITY_MAGIC_1            1
#define ELF_IDENTITY_MAGIC_2            2
#define ELF_IDENTITY_MAGIC_3            3
#define ELF_IDENTITY_CLASS              4
#define ELF_IDENTITY_DATA_ENCODING      5
#define ELF_IDENTITY_VERSION            6
#define ELF_IDENTITY_OS_ABI             7
#define ELF_IDENTITY_ABI_VERSION        8
#define ELF_IDENTITY_PADDING_START      9

#define ELF_MAGIC_0                     0x7F
#define ELF_MAGIC_1                     'E'
#define ELF_MAGIC_2                     'L'
#define ELF_MAGIC_3                     'F'

#define ELF_CLASS_32                    1
#define ELF_CLASS_64                    2

#define ELF_DATA_ENCODING_LSB           1
#define ELF_DATA_ENCODING_MSB           2

#define ELF_VERSION_1                   1

enum class elf_type_t {
    NONE = 0,
    RELOCATABLE = 1,
    EXECUTABLE = 2,
    DYNAMIC = 3
};

#define ELF_SECTION_TYPE_NULL           0
#define ELF_SECTION_TYPE_PROGRAM        1
#define ELF_SECTION_TYPE_SYMBOL_TABLE   2
#define ELF_SECTION_TYPE_STRING_TABLE   3
#define ELF_SECTION_TYPE_RELA           4
#define ELF_SECTION_TYPE_HASH           5
#define ELF_SECTION_TYPE_DYNAMIC        6
#define ELF_SECTION_TYPE_NOTE           7
#define ELF_SECTION_TYPE_NOBITS         8
#define ELF_SECTION_TYPE_REL            9
#define ELF_SECTION_TYPE_DYNSYM         11

#define ELF_PROGRAM_TYPE_LOAD           1
#define ELF_PROGRAM_TYPE_DYNAMIC        2

#define ELF_RELOCATION_TYPE_NONE        0
#define ELF_RELOCATION_TYPE_64          1
#define ELF_RELOCATION_TYPE_PC32        2
#define ELF_RELOCATION_TYPE_GLOB_DAT    6
#define ELF_RELOCATION_TYPE_JUMP_SLOT   7
#define ELF_RELOCATION_TYPE_RELATIVE    8

#define ELF_RELOCATE_SYMBOL(s)          ((s) >> 32)
#define ELF_RELOCATE_TYPE(t)            ((t) & MAX_UINT32)

#define ELF_GET_SECTION_HEADER(header) ((elf_section_header_t*)((u64)(header) + (header)->section_header_offset))
#define ELF_GET_SECTION(header, index) &ELF_GET_SECTION_HEADER((header))[(index)]

#include "common.hpp"
#include "std/string.hpp"
#include "std/map.hpp"

struct elf_header_t {
    u8 identity[ELF_IDENTITY_SIZE];
    u16 type;
    u16 machine;
    u32 version;
    u64 entry_point;
    u64 program_header_offset;
    u64 section_header_offset;
    u32 flags;
    u16 header_size;
    u16 program_header_entry_size;
    u16 program_header_count;
    u16 section_header_entry_size;
    u16 section_header_count;
    u16 string_table_index;
};

struct elf_section_header_t {
    u32 name_offset;
    u32 type;
    u64 flags;
    u64 virtual_address;
    u64 file_offset;
    u64 size;
    u32 link;
    u32 info;
    u64 alignment;
    u64 entry_size;
};

struct elf_symbol_t {
    u32 name_offset;
    u8 info;
    u8 other;
    u16 section_index;
    u64 value;
    u64 size;
};

struct elf_program_header_t {
    u32 type;
    u32 flags;
    u64 file_offset;
    u64 virtual_address;
    u64 physical_address;
    u64 file_size;
    u64 memory_size;
    u64 alignment;
};

struct elf_relocation_entry_t {
    u64 offset;
    u64 info;
    i64 addend;
};

struct elf_program_section_info_t {
    size_t size;
    u64 min_address;
    u64 max_address;
};

struct elf_tables_t {
    elf_section_header_t* symbol_table;
    elf_section_header_t* string_table;
};

elf_section_header_t* elf_find_section_by_name(u8* p_elf_data, const char* p_name);

bool elf_find_symbol_address(u8* p_elf_data, elf_section_header_t* p_symbol_table, elf_section_header_t* p_string_table, const char* p_symbol_name, u64* p_symbol_value);

int elf_relocate(u8* p_elf_data, u8* p_base_address, elf_section_header_t* p_target_section, elf_section_header_t* p_symbol_table, elf_section_header_t* p_string_table, std::linear_map<std::string, void*>* p_symbol_map);

int elf_check_file(u8* p_elf_data, elf_type_t elf_bin_type = elf_type_t::DYNAMIC);

elf_program_section_info_t elf_parse_program_sections(u8* p_elf_data);

void elf_load_program_sections(u8* p_elf_data, u8* p_base_address, elf_program_section_info_t* p_psi);

elf_tables_t elf_get_tables(u8* p_elf_data);

int elf_relocate_rel_sections(u8* p_elf_data, u8* p_base_address, elf_tables_t* p_tables, std::linear_map<std::string, void*>* p_symbol_map);

template <typename R, typename... Args>
using elf_func_t = R (*)(Args...);

template <typename R, typename... Args>
elf_func_t<R, Args...> elf_get_function(u8* p_elf_data, u8* p_base_address, elf_tables_t* p_tables, elf_program_section_info_t* p_psi, const char* p_name) {
    u64 offset = 0;
    if (!elf_find_symbol_address(p_elf_data, p_tables->symbol_table, p_tables->string_table, p_name, &offset))
        return nullptr;

    return (R(*)(Args...)) (p_base_address + (offset - p_psi->min_address));
}

#endif // __ELF_HPP__