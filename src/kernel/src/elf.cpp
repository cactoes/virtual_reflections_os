#include "elf.hpp"

elf_section_header_t* elf_find_section_by_name(uint8_t* p_elf_data, const char* p_name) {
    elf_header_t* p_elf_header = (elf_header_t*)p_elf_data;

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

int elf_relocate(uint8_t* p_elf_data, uint8_t* p_base_address, elf_section_header_t* p_target_section, elf_section_header_t* p_symbol_table, elf_section_header_t* p_string_table, linear_map<string, void*>* p_symbol_map) {
    int relocation_count = p_target_section->size / p_target_section->entry_size;
    elf_relocation_entry_t* relocations = (elf_relocation_entry_t*)(p_elf_data + p_target_section->file_offset);

    elf_symbol_t* symbol_table = (elf_symbol_t*)(p_elf_data + p_symbol_table->file_offset);
    const char* string_table = (const char*)(p_elf_data + p_string_table->file_offset);

    for (size_t i = 0; i < relocation_count; i++) {
        elf_relocation_entry_t* entry = &relocations[i];

        uint32_t symbol_index = ELF_RELOCATE_SYMBOL(entry->info);
        uint32_t relocation_type = ELF_RELOCATE_TYPE(entry->info);

        uint64_t* p_relocation_address = (uint64_t*)(p_base_address + entry->offset);
        uint64_t symbol_value = 0;

        if (symbol_index != 0) {
            const char* symbol_name = string_table + symbol_table[symbol_index].name_offset;

            if (symbol_table[symbol_index].value != 0 || symbol_table[symbol_index].section_index != 0) {
                symbol_value = (uint64_t)(p_base_address + symbol_table[symbol_index].value);
            } else {
                auto target = p_symbol_map->get(symbol_name);
                if (target == p_symbol_map->end())
                    return 1;
                
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
                *p_relocation_address = (uint64_t)(p_base_address + entry->addend);
                break;
            default:
                return 2;
        }
    }

    return 0;
}

int elf_check_file(uint8_t* p_elf_data, elf_type_t elf_bin_type) {
    const static uint8_t s_elf_header[] {
        ELF_MAGIC_0, ELF_MAGIC_1, ELF_MAGIC_2, ELF_MAGIC_3,
        ELF_CLASS_64, ELF_DATA_ENCODING_LSB, ELF_VERSION_1
    };

    elf_header_t* p_header = (elf_header_t*)p_elf_data;

    if (!memeq(&p_header->identity[ELF_IDENTITY_MAGIC_0], s_elf_header, ARRAY_LENGTH(s_elf_header)))
        return 1;

    if ((elf_type_t)p_header->type != elf_bin_type)
        return 2;

    return 0;
}

elf_program_section_info_t elf_parse_program_sections(uint8_t* p_elf_data) {
    elf_header_t* p_elf_header = (elf_header_t*)p_elf_data;
    elf_program_header_t* p_program_headers = (elf_program_header_t*)(p_elf_data + p_elf_header->program_header_offset);

    elf_program_section_info_t psi {};

    psi.min_address = (uint64_t)-1;
    psi.max_address = 0;

    for (size_t i = 0; i < p_elf_header->program_header_count; i++) {
        elf_program_header_t& program_header = p_program_headers[i];

        if (program_header.type != ELF_PROGRAM_TYPE_LOAD)
            continue;
        
        if (program_header.virtual_address < psi.min_address)
            psi.min_address = program_header.virtual_address;
        
        if (program_header.virtual_address + program_header.memory_size > psi.max_address)
            psi.max_address = program_header.virtual_address + program_header.memory_size;
    }

    psi.size = psi.max_address - psi.min_address;
    return psi;
}

void elf_load_program_sections(uint8_t* p_elf_data, uint8_t* p_base_address, elf_program_section_info_t* p_psi) {
    elf_header_t* p_elf_header = (elf_header_t*)p_elf_data;
    elf_program_header_t* p_program_headers = (elf_program_header_t*)(p_elf_data + p_elf_header->program_header_offset);

    for (size_t i = 0; i < p_elf_header->program_header_count; i++) {
        elf_program_header_t& program_header = p_program_headers[i];

        if (program_header.type != ELF_PROGRAM_TYPE_LOAD)
            continue;

        uint8_t* segment_address = p_base_address + (program_header.virtual_address - p_psi->min_address);
    
        memcpy(segment_address, p_elf_data + program_header.file_offset, program_header.file_size);
        
        if (program_header.memory_size > program_header.file_size)
            memzero(segment_address + program_header.file_size, program_header.memory_size - program_header.file_size);
    }
}

elf_tables_t elf_get_tables(uint8_t* p_elf_data) {
    elf_tables_t tables {};

    elf_section_header_t* dynamic_symbols = elf_find_section_by_name(p_elf_data, ".dynsym");
    elf_section_header_t* symbol_table = elf_find_section_by_name(p_elf_data, ".symtab");
    tables.symbol_table = dynamic_symbols ? dynamic_symbols : symbol_table;

    elf_section_header_t* dynamic_strings = elf_find_section_by_name(p_elf_data, ".dynstr");
    elf_section_header_t* string_table = elf_find_section_by_name(p_elf_data, ".strtab");
    tables.string_table = dynamic_strings ? dynamic_strings : string_table;

    return tables;
}

int elf_relocate_rel_sections(uint8_t* p_elf_data, uint8_t* p_base_address, elf_tables_t* p_tables, linear_map<string, void*>* p_symbol_map) {
    if (elf_section_header_t* rela_dyn = elf_find_section_by_name(p_elf_data, ".rela.dyn")) {
        if (elf_relocate(p_elf_data, p_base_address, rela_dyn, p_tables->symbol_table, p_tables->string_table, p_symbol_map) != 0) {
            return 1;
        }
    }

    if (elf_section_header_t* rela_plt = elf_find_section_by_name(p_elf_data, ".rela.plt")) {
        if (elf_relocate(p_elf_data, p_base_address, rela_plt, p_tables->symbol_table, p_tables->string_table, p_symbol_map) != 0) {
            return 2;
        }
    }

    return 0;
}