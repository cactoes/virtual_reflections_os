#include "storage/storage_manager.hpp"

static storage_manager_t* global_storage_manager = nullptr;

storage_manager_t* get_global_storage_manager() {
    return global_storage_manager;
}

void set_global_storage_manager(storage_manager_t* storage_manager) {
    global_storage_manager = storage_manager;
}

bool storage_manager_init(storage_manager_t* storage_manager) {
    if (!storage_manager)
        return false;

    storage_manager->ide.devices = {};
    storage_manager->ide.devices.resize(4);
    storage_manager->ahci.driver_ctx = {};
    storage_manager->ahci.devices = {};

    return true;
}