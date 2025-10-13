#include "drivers/storage/storage.hpp"
#include "filesystems/filesystem.hpp"
#include "filesystems/iso9660.hpp"
#include "filesystems/fat32.hpp"
#include "filesystems/vfs.hpp"

bool mount_disk(ptr::unique<storage_driver_interface_t> interface, const char* path) {
    ptr::unique<filesystem_interface_t> fs_interface;

    switch (filesystem_identify(interface.get())) {
        case filesystem_type_t::ISO9660: {
            // init file system
            iso9660_data_t fs_data {};
            if (iso9660_init((storage_driver_interface_t*)interface.get(), &fs_data) != 0)
                return false;

            // init file system interface
            fs_interface = ptr::make_unique<iso9660_filesystem_interface_t>(move(interface), fs_data);
            break;
        }
        case filesystem_type_t::FAT32: {
            // init file system
            fat32_data_t fs_data {};
            if (fat32_init((storage_driver_interface_t*)interface.get(), &fs_data) != 0)
                return false;

            // init file system interface
            fs_interface = ptr::make_unique<fat32_filesystem_interface_t>(move(interface), fs_data);
            break;
        }
        default:
            return false;
    }

    // init vfs storage interface
    auto vfs_storage_interface = ptr::make_unique<vfs_disk_storage_interface>(move(fs_interface));
    return vfs_mount(get_global_vfs(), path, move(vfs_storage_interface));
}