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

#define ELF_GET_SECTION_HEADER(header) ((elf_section_header_t*)((uint64_t)(header) + (header)->section_header_offset))
#define ELF_GET_SECTION(header, index) &ELF_GET_SECTION_HEADER((header))[(index)]

#include "common.hpp"
#include "string.hpp"
#include "utils/map.hpp"

struct elf_header_t {
    uint8_t identity[ELF_IDENTITY_SIZE];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint64_t entry_point;
    uint64_t program_header_offset;
    uint64_t section_header_offset;
    uint32_t flags;
    uint16_t header_size;
    uint16_t program_header_entry_size;
    uint16_t program_header_count;
    uint16_t section_header_entry_size;
    uint16_t section_header_count;
    uint16_t string_table_index;
};

struct elf_section_header_t {
    uint32_t name_offset;
    uint32_t type;
    uint64_t flags;
    uint64_t virtual_address;
    uint64_t file_offset;
    uint64_t size;
    uint32_t link;
    uint32_t info;
    uint64_t alignment;
    uint64_t entry_size;
};

struct elf_symbol_t {
    uint32_t name_offset;
    uint8_t info;
    uint8_t other;
    uint16_t section_index;
    uint64_t value;
    uint64_t size;
};

struct elf_program_header_t {
    uint32_t type;
    uint32_t flags;
    uint64_t file_offset;
    uint64_t virtual_address;
    uint64_t physical_address;
    uint64_t file_size;
    uint64_t memory_size;
    uint64_t alignment;
};

struct elf_relocation_entry_t {
    uint64_t offset;
    uint64_t info;
    int64_t addend;
};

struct elf_program_section_info_t {
    size_t size;
    uint64_t min_address;
    uint64_t max_address;
};

struct elf_tables_t {
    elf_section_header_t* symbol_table;
    elf_section_header_t* string_table;
};

elf_section_header_t* elf_find_section_by_name(uint8_t* p_elf_data, const char* p_name);

bool elf_find_symbol_address(uint8_t* p_elf_data, elf_section_header_t* p_symbol_table, elf_section_header_t* p_string_table, const char* p_symbol_name, uint64_t* p_symbol_value);

int elf_relocate(uint8_t* p_elf_data, uint8_t* p_base_address, elf_section_header_t* p_target_section, elf_section_header_t* p_symbol_table, elf_section_header_t* p_string_table, linear_map<string, void*>* p_symbol_map);

int elf_check_file(uint8_t* p_elf_data, elf_type_t elf_bin_type = elf_type_t::DYNAMIC);

elf_program_section_info_t elf_parse_program_sections(uint8_t* p_elf_data);

void elf_load_program_sections(uint8_t* p_elf_data, uint8_t* p_base_address, elf_program_section_info_t* p_psi);

elf_tables_t elf_get_tables(uint8_t* p_elf_data);

int elf_relocate_rel_sections(uint8_t* p_elf_data, uint8_t* p_base_address, elf_tables_t* p_tables, linear_map<string, void*>* p_symbol_map);

template <typename R, typename... Args>
using elf_func_t = R (*)(Args...);

template <typename R, typename... Args>
elf_func_t<R, Args...> elf_get_function(uint8_t* p_elf_data, uint8_t* p_base_address, elf_tables_t* p_tables, elf_program_section_info_t* p_psi, const char* p_name) {
    uint64_t offset = 0;
    if (!elf_find_symbol_address(p_elf_data, p_tables->symbol_table, p_tables->string_table, p_name, &offset))
        return nullptr;

    return (R(*)(Args...)) (p_base_address + (offset - p_psi->min_address));
}

#endif // __ELF_HPP__