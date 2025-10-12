#include "smbios.hpp"
#include "string.hpp"

enum print_mode_t {
    STD,
    DBG
};

extern void printf(print_mode_t mode, const char* p_str, ...);

size_t smbios_entry_length(const smbios_entry_header_t* entry) {
    size_t size = 0;
    const char* string_table = (const char*)((uint8_t*)entry + entry->length);
    while (!(string_table[size] == '\0' && string_table[size + 1] == '\0'))
        size++;

    size += 2;
    return entry->length + size;
}

void* smbios_find_struct_entry(const char* signature, size_t signature_size) {
    static uint8_t* smbios_start = (uint8_t*)0x000F0000;
    static uint8_t* smbios_end = (uint8_t*)0x000FFFFF;

    for (uint8_t* i = smbios_start; i < smbios_end; i += 16)
        if (memeq(i, signature, signature_size))
            return (void*)i;

    return nullptr;
}

void smbios_iterate(uint64_t table_address, void* extra, bool(*callback)(smbios_entry_header_t* entry, void* extra)) {
    smbios_entry_header_t* current = (smbios_entry_header_t*)table_address;
    while (current->type != smbios_type_t::END_OF_TABLE) {
        if (!callback(current, extra))
            return;

        size_t entry_size = smbios_entry_length(current);
        current = (smbios_entry_header_t*)((uint8_t*)current + entry_size);
    }
}

const char* smbios_get_string_at_index(const smbios_entry_header_t* entry, size_t i) {
    if (i == 0)
        return nullptr;

    const char* strings = (const char*)entry + entry->length;
    for (size_t current = 1; *strings; strings += strlen(strings) + 1, current++)
        if (current == i)
            return strings;

    return nullptr;
}