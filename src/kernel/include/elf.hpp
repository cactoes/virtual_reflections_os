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
#define ELF_MAGIC_3                     'F

#define ELF_TYPE_NONE                   0
#define ELF_TYPE_RELOCATABLE            1
#define ELF_TYPE_EXECUTABLE             2
#define ELF_TYPE_DYNAMIC                3

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
#define ELF_RELOCATE_RELOC_TYPE(t)      ((t) & 0xffffffffL)

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

elf_section_header_t* elf_find_section_by_name(elf_header_t* p_elf_header, uint8_t* p_elf_data, const char* p_name) {
    if (p_elf_header->string_table_index == 0)
        return nullptr;

    elf_section_header_t* sections = (elf_section_header_t*)(p_elf_data + p_elf_header->section_header_offset);
    elf_section_header_t* string_table = &sections[p_elf_header->string_table_index];
    const char* string_data = (const char*)(p_elf_data + string_table->file_offset);

    for (size_t i = 0; i < p_elf_header->section_header_count; i++) {
        const char* section_name = string_data + sections[i].name_offset;
        if (streq(section_name, p_name))
            return &sections[i];
    }

    return nullptr;
}

bool elf_find_symbol_address(uint8_t* p_elf_data, elf_section_header_t* p_symbol_table, elf_section_header_t* p_string_table, const char* p_symbol_name, uint64_t* p_symbol_value) {
    int symbol_count = p_symbol_table->size / p_symbol_table->entry_size;
    
    elf_symbol_t* symbol_table = (elf_symbol_t*)(p_elf_data + p_symbol_table->file_offset);
    const char* string_table = (const char*)(p_elf_data + p_string_table->file_offset);

    for (size_t i = 0; i < symbol_count; i++) {
        const char* current_name = string_table + symbol_table[i].name_offset;
        if (streq(current_name, p_symbol_name)) {
            *p_symbol_value = symbol_table[i].value;
            return true;
        }
    }

    return false;
}

int elf_relocate(uint8_t* p_elf_data, void* p_base_address, elf_section_header_t* p_target_section, elf_section_header_t* p_symbol_table, elf_section_header_t* p_string_table, linear_map<string, void*>* p_symbol_map) {
    int relocation_count = p_target_section->size / p_target_section->entry_size;
    elf_relocation_entry_t* relocations = (elf_relocation_entry_t*)(p_elf_data + p_target_section->file_offset);

    elf_symbol_t* symbol_table = (elf_symbol_t*)(p_elf_data + p_symbol_table->file_offset);
    const char* string_table = (const char*)(p_elf_data + p_string_table->file_offset);

    for (size_t i = 0; i < relocation_count; i++) {
        elf_relocation_entry_t* entry = &relocations[i];

        uint32_t symbol_index = ELF_RELOCATE_SYMBOL(entry->info);
        uint32_t relocation_type = ELF_RELOCATE_SYMBOL(entry->info);

        uint64_t* p_relocation_address = (uint64_t*)((uint64_t)p_base_address + entry->offset);
        uint64_t symbol_value = 0;

        if (symbol_index != 0) {
            const char* symbol_name = string_table + symbol_table[i].name_offset;

            if (symbol_table[i].value != 0 || symbol_table[i].section_index != 0) {
                symbol_value = (uint64_t)p_base_address + symbol_table[i].value;
            } else {
                auto target = p_symbol_map->get(symbol_name);
                if (target == p_symbol_map->end())
                    return -1;
                
                symbol_value = (uint64_t)target->value;
            }
        }

        switch (relocation_type) {
            case ELF_RELOCATION_TYPE_NONE:
                break;
            case ELF_RELOCATION_TYPE_64:
                *p_relocation_address = symbol_value + entry->addend;
                break;
                
            case ELF_RELOCATION_TYPE_PC32: {
                int32_t* addr32 = (int32_t*)p_relocation_address;
                uint64_t pc = (uint64_t)p_relocation_address;
                *addr32 = (int32_t)(symbol_value + entry->addend - pc);
                break;
            }
            case ELF_RELOCATION_TYPE_GLOB_DAT:
            case ELF_RELOCATION_TYPE_JUMP_SLOT:
                *p_relocation_address = symbol_value;
                break;
            case ELF_RELOCATION_TYPE_RELATIVE:
                *p_relocation_address = (uint64_t)p_base_address + entry->addend;
                break;
            default:
                return -1;
        }
    }
}

#endif // __ELF_HPP__