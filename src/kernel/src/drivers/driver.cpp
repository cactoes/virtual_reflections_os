#include "drivers/driver.hpp"
#include "elf.hpp"
#include "utils/map.hpp"
#include "utils/pointer.hpp"

// TODO @since 12/08/2025 -- 00:13
// relocate this kernel api
#include "kernel_api.hpp"

static system_driver_handle_t g_current_handle = 0;
static linear_map<system_driver_handle_t, ptr::unique<system_driver_t>> g_loaded_drivers {};

system_driver_handle_t driver_load(const char* p_name, void* p_driver_file) {
    if (elf_check_file((uint8_t*)p_driver_file) != 0)
        return SYSTEM_DRIVER_HANDLE_INVALID;

    auto program_section_info = elf_parse_program_sections((uint8_t*)p_driver_file);

    uint8_t* base_address = (uint8_t*)heap_alloc(get_global_heap(), program_section_info.size);
    if (!base_address)
        return SYSTEM_DRIVER_HANDLE_INVALID;

    elf_load_program_sections((uint8_t*)p_driver_file, base_address, &program_section_info);

    auto tables = elf_get_tables((uint8_t*)p_driver_file);
    if (!tables.string_table || !tables.symbol_table)
        return SYSTEM_DRIVER_HANDLE_INVALID;

    // TODO @since 13/08/2025 -- 22:55
    // make this more global / better manageable
    linear_map<string, void*> symbol_map {};
    symbol_map["kernel_test_function"] = (void*)&kernel_test_function;

    if (elf_relocate_rel_sections((uint8_t*)p_driver_file, base_address, &tables, &symbol_map) != 0)
        return SYSTEM_DRIVER_HANDLE_INVALID;

    auto system_driver = ptr::make_unique<system_driver_t>();
    system_driver->base_address = (void*)base_address;
    system_driver->file_data_ptr = p_driver_file;
    system_driver->name = p_name;
    system_driver->functions.driver_init = elf_get_function<int>((uint8_t*)p_driver_file, base_address, &tables, &program_section_info, "driver_init");
    system_driver->functions.driver_exit = elf_get_function<int>((uint8_t*)p_driver_file, base_address, &tables, &program_section_info, "driver_exit");
    
    g_current_handle++;

    g_loaded_drivers[g_current_handle] = move(system_driver);

    return g_current_handle;
}

int driver_unload(system_driver_handle_t handle) {
    auto driver_it = g_loaded_drivers.get(handle);
    if (driver_it == g_loaded_drivers.end())
        return 1;

    if (driver_it->value->base_address)
        heap_free(get_global_heap(), driver_it->value->base_address);

    return g_loaded_drivers.remove(handle) ? 0 : 2;
}

int driver_start(system_driver_handle_t handle) {
    // TODO @since 12/08/2025 -- 00:29
    // spawn new thread etc

    auto driver_it = g_loaded_drivers.get(handle);
    if (driver_it == g_loaded_drivers.end())
        return 1;
    
    return driver_it->value->functions.driver_init();
}

int driver_stop(system_driver_handle_t handle) {
    auto driver_it = g_loaded_drivers.get(handle);
    if (driver_it == g_loaded_drivers.end())
        return 1;
    
    return driver_it->value->functions.driver_exit();
}

void* driver_get_function(system_driver_handle_t handle, const char* p_name) {
    auto driver_it = g_loaded_drivers.get(handle);
    if (driver_it == g_loaded_drivers.end())
        return nullptr;

    system_driver_t* driver = driver_it->value.get();

    auto program_section_info = elf_parse_program_sections((uint8_t*)driver->file_data_ptr);

    auto tables = elf_get_tables((uint8_t*)driver->file_data_ptr);
    if (!tables.string_table || !tables.symbol_table)
        return nullptr;

    return (void*)elf_get_function<void>((uint8_t*)driver->file_data_ptr, (uint8_t*)driver->base_address, &tables, &program_section_info, p_name);
}