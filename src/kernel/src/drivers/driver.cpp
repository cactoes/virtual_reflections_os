#include "kernel_include/virtual_reflections_exports.hpp"
#include "drivers/driver.hpp"
#include "memory/heap.hpp"
#include "elf.hpp"

driver_manager_t* global_driver_manager = nullptr;

void set_global_driver_manager(driver_manager_t* driver_manager) {
    global_driver_manager = driver_manager;
}

driver_manager_t* get_global_driver_manager() {
    return global_driver_manager;
}

system_driver_handle_t driver_manager_get_driver_handle(driver_manager_t* driver_manager, const char* driver_name) {
    for (auto& driver : driver_manager->loaded_drivers)
        if (driver.value->name == driver_name)
            return driver.key;

    return SYSTEM_DRIVER_HANDLE_INVALID;
}

system_driver_handle_t driver_load(driver_manager_t* driver_manager, const char* p_name, void* p_driver_file) {
    if (elf_check_file((uint8_t*)p_driver_file) != 0)
        return SYSTEM_DRIVER_HANDLE_INVALID;

    auto program_section_info = elf_parse_program_sections((uint8_t*)p_driver_file);

    uint8_t* base_address = (uint8_t*)malloc(program_section_info.size);
    if (!base_address)
        return SYSTEM_DRIVER_HANDLE_INVALID;

    elf_load_program_sections((uint8_t*)p_driver_file, base_address, &program_section_info);

    auto tables = elf_get_tables((uint8_t*)p_driver_file);
    if (!tables.string_table || !tables.symbol_table)
        return SYSTEM_DRIVER_HANDLE_INVALID;

    // TODO @since 13/08/2025 -- 22:55
    // make this more global / better manageable
    std::linear_map<std::string, void*> symbol_map {};
    symbol_map["malloc"] = (void*)&malloc;
    symbol_map["free"] = (void*)&free;
    symbol_map["kprint"] = (void*)&kprint;
    symbol_map["ksleep"] = (void*)&ksleep;
    symbol_map["ktime_since_boot"] = (void*)&ktime_since_boot;
    symbol_map["knet_udp_send"] = (void*)&knet_udp_send;

    symbol_map["memset_impl"] = (void*)&memset_impl;
    symbol_map["memzero_impl"] = (void*)&memzero_impl;
    symbol_map["memcpy_impl"] = (void*)&memcpy_impl;
    symbol_map["memeq_impl"] = (void*)&memeq_impl;

    symbol_map["_Znwm"] = (void*)(void* (*)(__SIZE_TYPE__))&operator new;
    symbol_map["_Znam"] = (void*)(void* (*)(__SIZE_TYPE__))&operator new[];
    symbol_map["_ZdlPv"] = (void*)(void (*)(void*))&operator delete;
    symbol_map["_ZdaPv"] = (void*)(void (*)(void*))&operator delete[];
    symbol_map["_ZdlPvm"] = (void*)(void (*)(void*, __SIZE_TYPE__))&operator delete;
    symbol_map["_ZdaPvm"] = (void*)(void (*)(void*, __SIZE_TYPE__))&operator delete[];
    symbol_map["_ZnwmPv"] = (void*)(void* (*)(__SIZE_TYPE__, void*))&operator new;

    if (elf_relocate_rel_sections((uint8_t*)p_driver_file, base_address, &tables, &symbol_map) != 0)
        return SYSTEM_DRIVER_HANDLE_INVALID;

    auto system_driver = std::make_unique<system_driver_t>();
    system_driver->base_address = (void*)base_address;
    system_driver->file_data_ptr = p_driver_file;
    system_driver->name = p_name;
    system_driver->functions.driver_init = elf_get_function<int>((uint8_t*)p_driver_file, base_address, &tables, &program_section_info, "driver_init");
    system_driver->functions.driver_exit = elf_get_function<int>((uint8_t*)p_driver_file, base_address, &tables, &program_section_info, "driver_exit");
    
    driver_manager->current_handle++;

    driver_manager->loaded_drivers[driver_manager->current_handle] = move(system_driver);

    return driver_manager->current_handle;
}

int driver_unload(driver_manager_t* driver_manager, system_driver_handle_t handle) {
    auto driver_it = driver_manager->loaded_drivers.get(handle);
    if (driver_it == driver_manager->loaded_drivers.end())
        return 1;

    if (driver_it->value->base_address)
        free(driver_it->value->base_address);

    return driver_manager->loaded_drivers.remove(handle) ? 0 : 2;
}

int driver_start(driver_manager_t* driver_manager, system_driver_handle_t handle) {
    // TODO @since 12/08/2025 -- 00:29
    // spawn new thread etc

    auto driver_it = driver_manager->loaded_drivers.get(handle);
    if (driver_it == driver_manager->loaded_drivers.end())
        return 1;
    
    return driver_it->value->functions.driver_init();
}

int driver_stop(driver_manager_t* driver_manager, system_driver_handle_t handle) {
    auto driver_it = driver_manager->loaded_drivers.get(handle);
    if (driver_it == driver_manager->loaded_drivers.end())
        return 1;
    
    return driver_it->value->functions.driver_exit();
}

void* driver_get_function(driver_manager_t* driver_manager, system_driver_handle_t handle, const char* p_name) {
    auto driver_it = driver_manager->loaded_drivers.get(handle);
    if (driver_it == driver_manager->loaded_drivers.end())
        return nullptr;

    system_driver_t* driver = driver_it->value.get();

    auto program_section_info = elf_parse_program_sections((uint8_t*)driver->file_data_ptr);

    auto tables = elf_get_tables((uint8_t*)driver->file_data_ptr);
    if (!tables.string_table || !tables.symbol_table)
        return nullptr;

    return (void*)elf_get_function<void>((uint8_t*)driver->file_data_ptr, (uint8_t*)driver->base_address, &tables, &program_section_info, p_name);
}

uint64_t driver_query_capability(driver_manager_t* driver_manager, system_driver_handle_t handle, const char* feature) {
    driver_query_capability_t driver_query_capability_fn = (driver_query_capability_t)driver_get_function(driver_manager, handle, "query_capability");
    if (!driver_query_capability_fn)
        return -1;
    
    return driver_query_capability_fn(feature);
}