#include "smbios.hpp"

size_t smbios_entry_length(const smbios_entry_header_t* entry) {
    size_t size;
    const char* string_table = (const char*)((uint8_t*)entry + entry->length);
    for (size_t i = 0; string_table[i] != '0' && string_table[i + 1] != '0'; i++)
        size++;
    
    return entry->length + size;
}

void smbios_test() {
    // parse memory size
    // size_t total_memory_size = 0;
    // for (auto mm_entry = mb_get_first_entry((multiboot_t*)p_multiboot_struct); mm_entry; mm_entry = mb_get_next_entry((multiboot_t*)p_multiboot_struct, mm_entry)) {
    //     // reserve physical pages for reserved memory
    //     if (mm_entry->type == (uint32_t)memory_map_type_t::USABLE) {
    //         total_memory_size += mm_entry->len;
    //     }
    // }

    // uint8_t* smbios_start = (uint8_t*)0x000F0000;
    // uint8_t* smbios_end = (uint8_t*)0x000FFFFF;
    // const char sig[] { '_', 'S', 'M', '_', };
    // uint8_t* smbios_struct_pointer = nullptr;

    // for (uint8_t* i = smbios_start; i < smbios_end; i += 16) {
    //     if (memeq(i, sig, ARRAY_SIZE(sig))) {
    //         smbios_struct_pointer = i;
    //         break;
    //     }
    // }

    // printf(DBG, "smbios_struct_pointer: 0x%p\n", smbios_struct_pointer);

    // smbios_t* smbios_struct = (smbios_t*)smbios_struct_pointer;
    // smbios_entry_header_t* current = (smbios_entry_header_t*)(uint64_t)smbios_struct->table_address;
    // while (current->type != smbios_type_t::END_OF_TABLE) {
    //     if (current->type == smbios_type_t::SYSTEM_INFORMATION) {
    //         printf(DBG, "LENGTH: %uh", current->length);
    //     }

    //     size_t entry_size = smbios_entry_length(current);
    //     current = (smbios_entry_header_t*)((uint8_t*)current + entry_size);
    // }

    // printf(DBG, "table_address: %p\n", smbios_struct->table_address);
}