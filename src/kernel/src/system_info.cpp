#include "system_info.hpp"
#include "smbios.hpp"
#include "cpu.hpp"
#include "linker.hpp"
#include "memory/paging.hpp"

system_info_manager_t* global_boot_info_manager;

void set_global_system_info_manager(system_info_manager_t* system_info_manager) {
    global_boot_info_manager = system_info_manager;
}

system_info_manager_t* get_global_system_info_manager() {
    return global_boot_info_manager;
}

void system_info_parse_memory_size(system_info_manager_t* system_info_manager, multiboot2_info_t* multiboot_struct) {
    extern pmem_info_t global_pmem_info;

    system_info_manager->memory_size = global_pmem_info.memory_size;
}

bool system_info_dump(smbios_entry_header_t* entry, void* system_info_manager) {
    if (entry->type == smbios_type_t::SYSTEM_INFORMATION) {
        const smbios_entry_system_information_t* sysinfo = (smbios_entry_system_information_t*)entry;

        ((system_info_manager_t*)system_info_manager)->manufacturer = smbios_get_string_at_index(entry, sysinfo->manufacturer_str_index);
        ((system_info_manager_t*)system_info_manager)->product_name = smbios_get_string_at_index(entry, sysinfo->product_name_str_index);
        ((system_info_manager_t*)system_info_manager)->version = smbios_get_string_at_index(entry, sysinfo->version_str_index);
        ((system_info_manager_t*)system_info_manager)->serial_number = smbios_get_string_at_index(entry, sysinfo->serial_number_str_index);

        return false;
    }

    return true;
}

void system_info_parse_system_information(system_info_manager_t* system_info_manager) {
    // BUG @since 28/05/2026 -- 09:00
    // only works on NON uefi

    if (smbios_t* struct_pointer = (smbios_t*)smbios_find_struct_entry(SMBIOS_SIGNATUE, ARRAY_SIZE(SMBIOS_SIGNATUE) - 1)) {
        smbios_iterate(PTOV_I(struct_pointer->table_address), system_info_manager, system_info_dump);
        return;
    }

    if (smbios64_t* struct_pointer = (smbios64_t*)smbios_find_struct_entry(SMBIOS64_SIGNATUE, ARRAY_SIZE(SMBIOS64_SIGNATUE) - 1)) {
        smbios_iterate(PTOV_I(struct_pointer->table_address), system_info_manager, system_info_dump);
        return;
    }
}

void system_info_get_cpu_name(system_info_manager_t* system_info_manager) {
    char buffer[49];
    memzero(buffer, sizeof(buffer));
    if (cpu_get_name(buffer, sizeof(buffer)))
        system_info_manager->cpu_name = buffer;
}

void system_info_get_boot_uuid(system_info_manager_t* system_info_manager, multiboot2_info_t* multiboot2_struct) {
    char* cmdline = nullptr;
    u64 cmdline_len = 0;
    const char root_text[] = "root=";

    for (multiboot2_tag_t* tag = (multiboot2_tag_t*)multiboot2_struct->tags; tag->type != 0 && tag->size != 8; tag = (multiboot2_tag_t*)((u64)tag + align_up(tag->size, 8))) {
        if (tag->type != (u32)multiboot_tag_type_t::CMDLINE)
            continue;

        cmdline = (char*)(((u8*)tag) + sizeof(multiboot2_tag_t));
        cmdline_len = tag->size - sizeof(multiboot2_tag_t) - 1;
    }

    if (!cmdline || cmdline_len < UUID_LEN + sizeof(root_text) - 1)
        return;

    char uuid[UUID_LEN + 1] {};

    for (u64 i = 0; i < cmdline_len; i++) {
        const char* currentptr = &cmdline[i];
        if (str_starts_with(currentptr, root_text) && cmdline_len - (sizeof(root_text) - 1) - i >= UUID_LEN) {
            memcpy(&uuid[0], currentptr + sizeof(root_text) - 1, UUID_LEN);
        }
    }

    system_info_manager->boot_uuid = uuid;
}