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