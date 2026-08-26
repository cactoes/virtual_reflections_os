#include "storage/disk_manager.hpp"
#include "std/string.hpp"

static disk_manager_t* global_disk_manager = nullptr;

disk_manager_t* get_global_disk_manager() {
    return global_disk_manager;
}

void set_global_disk_manager(disk_manager_t* disk_manager) {
    global_disk_manager = disk_manager;
}

bool disk_manager_register(disk_manager_t* disk_manager, const char* name, const disk_interface_t* interface, void* disk_data) {
    if (!disk_manager || !name || !interface || !disk_data)
        return false;

    u64 namelen_raw = strlen(name);
    u64 namelen = MIN(namelen_raw, sizeof(disk_t::name) - 1);

    disk_t disk {};
    memcpy(disk.name, name, namelen);
    disk.name[namelen] = '\0';

    disk.interface = interface;
    disk.disk_data = disk_data;
    disk_manager->disks.insert_back(disk);

    return true;
}