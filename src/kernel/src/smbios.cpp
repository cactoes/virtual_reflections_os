#include "smbios.hpp"
#include "std/string.hpp"
#include "linker.hpp"

size_t smbios_entry_length(const smbios_entry_header_t* entry) {
    size_t size = 0;
    const char* string_table = (const char*)((u8*)entry + entry->length);
    while (!(string_table[size] == '\0' && string_table[size + 1] == '\0'))
        size++;

    size += 2;
    return entry->length + size;
}

void* smbios_find_struct_entry(const char* signature, size_t signature_size) {
    static u8* smbios_start = (u8*)PTOV_I(0x000F0000);
    static u8* smbios_end = (u8*)PTOV_I(0x000FFFFF);

    for (u8* i = smbios_start; i < smbios_end; i += 16)
        if (memeq(i, signature, signature_size))
            return (void*)i;

    return nullptr;
}

void smbios_iterate(u64 table_address, void* extra, bool(*callback)(smbios_entry_header_t* entry, void* extra)) {
    smbios_entry_header_t* current = (smbios_entry_header_t*)table_address;
    while (current->type != smbios_type_t::END_OF_TABLE) {
        if (!callback(current, extra))
            return;

        size_t entry_size = smbios_entry_length(current);
        current = (smbios_entry_header_t*)((u8*)current + entry_size);
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