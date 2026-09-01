#include "filesystems/vfs.hpp"
#include "filesystems/iso9660.hpp"
#include "filesystems/fat32.hpp"

static vfs_t* global_vfs = nullptr;

vfs_t* get_global_vfs() {
    return global_vfs;
}

void set_global_vfs(vfs_t* vfs) {
    global_vfs = vfs;
}

void vfs_init(vfs_t* vfs) {
    vfs->mount_points = {};
    vfs->file_handles = {};
    vfs->last_fd = 0;
}

fs_type_t get_filesystem(block_device_t* device) {
    if (!device)
        return fs_type_t::UNKNOWN;

    u8* buffer = (u8*)malloc(device->block_size);
    if (!block_device_read(device, 16, buffer, device->block_size))
        return fs_type_t::UNKNOWN;

    if (memeq(&buffer[1], "CD001", 5)) {
        free(buffer);
        return fs_type_t::ISO9660;        
    }

    memzero(buffer, device->block_size);
    if (!block_device_read(device, 0, buffer, device->block_size))
        return fs_type_t::UNKNOWN;

    if (fat32_validate(buffer, device->block_size)) {
        free(buffer);
        return fs_type_t::FAT32;
    }

    free(buffer);
    return fs_type_t::UNKNOWN;
}

bool vfs_mount_block_device(vfs_t* vfs, block_device_t* block_device, const char* name) {
    if (!name)
        return false;

    if (!vfs || !block_device || vfs->mount_points.contains(std::string(name)))
        return false;

    switch (get_filesystem(block_device)) {
        case fs_type_t::FAT32: {
            fat32_fsdata_t* data = new (malloc(sizeof(fat32_fsdata_t))) fat32_fsdata_t();
            if (!data)
                return false;

            if (!fat32_init(block_device, data)) {
                free(data);
                return false;
            }

            vfs_mount_point_t mount_point {};
            size_t copy_len = MIN(strlen(name), sizeof(mount_point.name) - 1);
            memcpy(mount_point.name, name, copy_len);
            mount_point.name[copy_len] = '\0';
            mount_point.interface = get_fat32_filesystem_interface();
            mount_point.filesystem_data = data;
            vfs->mount_points[name] = mount_point;

            return true;

        }
        case fs_type_t::ISO9660: {
            iso9660_fsdata_t* data = new (malloc(sizeof(iso9660_fsdata_t))) iso9660_fsdata_t();
            if (!data)
                return false;

            if (!iso9660_init(block_device, data)) {
                free(data);
                return false;
            }

            vfs_mount_point_t mount_point {};
            size_t copy_len = MIN(strlen(name), sizeof(mount_point.name) - 1);
            memcpy(mount_point.name, name, copy_len);
            mount_point.name[copy_len] = '\0';
            mount_point.interface = get_iso9660_filesystem_interface();
            mount_point.filesystem_data = data;
            vfs->mount_points[name] = mount_point;

            return true;

        }
        default:
            return false;
    }

    return false;
}

const vfs_mount_point_t* vfs_get_mount_point_from_path(vfs_t* vfs, const char* path) {
    const std::dynamic_array<std::string> path_parts = str_split(std::string(path), '/');
    if (path_parts.length() == 0)
        return nullptr;

    const std::string& mount_name = *path_parts.get_at(0);
    if (!vfs->mount_points.contains(mount_name))
        return nullptr;

    return &vfs->mount_points.get(mount_name)->value;
}

std::string get_mount_point_relative_path(const vfs_mount_point_t* mount_point, const std::string& path) {
    if (!mount_point)
        return path;

    size_t str_offset = str_starts_with(path.c_str(), "/") ? 1 : 0;

    size_t size = strlen(mount_point->name) + str_offset;
    if (size >= path.length())
        return "";

    return path.substr(size);
}

file_descriptor_t vfs_open_file(vfs_t* vfs, const char* path) {
    const vfs_mount_point_t* mount_point = vfs_get_mount_point_from_path(vfs, path);
    if (!mount_point)
        return FILE_DESCRIPTOR_INVALID;

    const std::string mount_point_relative_path = get_mount_point_relative_path(mount_point, std::string(path));

    bool file_exists = mount_point->interface->file_exists(mount_point->filesystem_data, mount_point_relative_path.c_str());
    if (!file_exists)
        return FILE_DESCRIPTOR_INVALID;

    // TODO @since 30/01/2026 -- 01:13
    // better fd logic
    const file_descriptor_t file_descriptor = vfs->last_fd++;
    if (file_descriptor == FILE_DESCRIPTOR_INVALID)
        return FILE_DESCRIPTOR_INVALID;

    vfs->file_handles[file_descriptor] = std::string(path);
    return file_descriptor;
}

bool vfs_close_file(vfs_t* vfs, file_descriptor_t fd) {
    if (!vfs)
        return false;

    if (fd == FILE_DESCRIPTOR_INVALID)
        return false;

    if (!vfs->file_handles.contains(fd))
        return false;

    return vfs->file_handles.remove(fd);
}

bool vfs_read_file(vfs_t* vfs, file_descriptor_t fd, u8** data, u64* size) {
    auto fd_iterator = vfs->file_handles.get(fd);
    if (fd_iterator == vfs->file_handles.end())
        return false;

    const std::string& path = fd_iterator->value;
    const vfs_mount_point_t* mount_point = vfs_get_mount_point_from_path(vfs, path.c_str());
    if (!mount_point)
        return false;

    *data = nullptr;
    *size = 0;

    const std::string mount_point_relative_path = get_mount_point_relative_path(mount_point, path);

    return mount_point->interface->read(mount_point->filesystem_data, mount_point_relative_path.c_str(), data, size);
}

bool vfs_list_directory(vfs_t* vfs, const char* path, std::dynamic_array<vfs_node_t>* out_nodes) {
    if (!vfs || !path || !out_nodes)
        return false;

    const vfs_mount_point_t* mount_point = vfs_get_mount_point_from_path(vfs, path);
    if (!mount_point)
        return false;

    const std::string mount_point_relative_path = get_mount_point_relative_path(mount_point, path);
    
    return mount_point->interface->enumerate_directory(mount_point->filesystem_data, mount_point_relative_path.c_str(), out_nodes);
}

const std::linear_map<std::string, vfs_mount_point_t>* vfs_get_mount_points(vfs_t* vfs) {
    if (!vfs)
        return nullptr;

    return &vfs->mount_points;
}