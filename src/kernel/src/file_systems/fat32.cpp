#include "file_systems/fat32.hpp"
#include "memory.hpp"
#include "debug.hpp"
#include "vector.hpp"
#include "string.hpp"
#include "drivers/ahci_driver.hpp"

int fat32_drive_init(drive_t* drive, fs_t* fs) {
    return 1;
}

int fat32_drive_deinit(fs_t* fs) {
    if (!fs->data)
        return 1;

    g_heap_free(fs->data);
    return 0;
}

int fat32_read_file(fs_t* fs, drive_t* drive, const char* file_path, void** file_data, size_t* size) {
    return 1;
}

int fat32_write_file(fs_t* fs, drive_t* drive, const char* file_path, void* file_data, size_t* size) {
    return 1;
}
