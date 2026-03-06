#include "storage/vfs.hpp"
#include "storage/filesystems/iso9660.hpp"
#include "storage/filesystems/fat32.hpp"
#include "storage/mbr.hpp"

static vfs_t* global_vfs = nullptr;

vfs_t* get_global_vfs() {
    return global_vfs;
}

void set_global_vfs(vfs_t* vfs) {
    global_vfs = vfs;
}

void vfs_init(vfs_t* vfs) {
    vfs->file_handles = {};
    vfs->last_fd = 0;
    vfs->mount_points = {};
}

bool vfs_mount_file_system(vfs_t* vfs, const char* name, fs_type_t type, std::unique_ptr<void> fs_data) {
    if (vfs->mount_points.contains(std::string(name)))
        return false;

    vfs->mount_points[std::string(name)] = { .name = name, .type = type, .data = move(fs_data)  };
    return true;
}

const vfs_mount_point_t* vfs_get_mount_point(vfs_t* vfs, const char* path) {
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

    // FIXME @since 30/01/2026 -- 02:41
    // proper check
    size_t size = mount_point->name.length() + str_offset;
    if (size >= path.length())
        return "";

    return path.substr(size);
}

file_descriptor_t vfs_open_file(vfs_t* vfs, const char* path) {
    const vfs_mount_point_t* mount_point = vfs_get_mount_point(vfs, path);
    if (!mount_point)
        return FILE_DESCRIPTOR_INVALID;

    const std::string mount_point_relative_path = get_mount_point_relative_path(mount_point, std::string(path));

    bool file_exists = false;
    switch (mount_point->type) {
        case fs_type_t::ISO9660:
            file_exists = iso9660_file_exists((iso9660_fsdata_t*)mount_point->data.get(), mount_point_relative_path.c_str());
            break;
        case fs_type_t::FAT32:
            file_exists = fat32_file_exists((fat32_fsdata_t*)mount_point->data.get(), mount_point_relative_path.c_str());
            break;
        default:
            file_exists = false;
            break;
    }

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

bool vfs_read_file(vfs_t* vfs, file_descriptor_t fd, uint8_t** data, size_t* size) {
    auto fd_iterator = vfs->file_handles.get(fd);
    if (fd_iterator == vfs->file_handles.end())
        return false;

    const std::string& path = fd_iterator->value;
    const vfs_mount_point_t* mount_point = vfs_get_mount_point(vfs, path.c_str());
    if (!mount_point)
        return false;

    *data = nullptr;
    *size = 0;

    const std::string mount_point_relative_path = get_mount_point_relative_path(mount_point, path);
    switch (mount_point->type) {
        case fs_type_t::ISO9660:
            return iso9660_read((iso9660_fsdata_t*)mount_point->data.get(), mount_point_relative_path.c_str(), data, size);
        case fs_type_t::FAT32:
            return fat32_read((fat32_fsdata_t*)mount_point->data.get(), mount_point_relative_path.c_str(), data, size);
        default:
            return false;
    }

    return false;
}

bool vfs_list_directory(vfs_t* vfs, const char* path, std::dynamic_array<vfs_node_t>* out_nodes) {
    if (!vfs || !path || !out_nodes)
        return false;

    const vfs_mount_point_t* mount_point = vfs_get_mount_point(vfs, path);
    if (!mount_point)
        return false;

    const std::string mount_point_relative_path = get_mount_point_relative_path(mount_point, path);

    switch (mount_point->type) {
        case fs_type_t::ISO9660: {
            std::dynamic_array<iso9660_node_t> iso9660_nodes {};
            if (!iso9660_list_directory((iso9660_fsdata_t*)mount_point->data.get(), mount_point_relative_path.c_str(), &iso9660_nodes))
                return false;

            for (const auto& node : iso9660_nodes) {
                vfs_node_t vfs_node {};
                vfs_node.is_directory = node.is_directory;
                vfs_node.name = std::string(node.name);
                vfs_node.size = node.size;
                out_nodes->insert_back(vfs_node);
            }

            return true;
        }
        case fs_type_t::FAT32: {
            std::dynamic_array<fat32_node_t> fat32_nodes {};
            if (!fat32_list_directory((fat32_fsdata_t*)mount_point->data.get(), mount_point_relative_path.c_str(), &fat32_nodes))
                return false;

            for (const auto& node : fat32_nodes) {
                vfs_node_t vfs_node {};
                vfs_node.is_directory = node.is_directory;
                vfs_node.name = std::string(node.name);
                vfs_node.size = node.size;
                out_nodes->insert_back(vfs_node);
            }
            
            return true;
        }
        default:
            return false;
    }

    return false;
}

fs_type_t get_filesystem(block_device_t* device) {
    if (!device)
        return fs_type_t::UNKNOWN;

    uint8_t* buffer = (uint8_t*)malloc(device->block_size);
    if (!block_read(device, 16, buffer))
        return fs_type_t::UNKNOWN;

    if (memeq(&buffer[1], "CD001", 5)) {
        free(buffer);
        return fs_type_t::ISO9660;        
    }

    memzero(buffer, device->block_size);
    if (!block_read(device, 0, buffer))
        return fs_type_t::UNKNOWN;

    if (fat32_validate(buffer, device->block_size)) {
        free(buffer);
        return fs_type_t::FAT32;
    }

    free(buffer);
    return fs_type_t::UNKNOWN;
}

bool vfs_mount_block_device(vfs_t* vfs, std::unique_ptr<block_device_t> device, const char* name) {
    switch (get_filesystem(device.get())) {
        case fs_type_t::ISO9660: {
            iso9660_fsdata_t* data = new (malloc(sizeof(iso9660_fsdata_t))) iso9660_fsdata_t();
            if (!data)
                return false;

            if (!iso9660_init(move(device), data)) {
                free(data);
                return false;
            }

            return vfs_mount_file_system(vfs, name, fs_type_t::ISO9660, std::unique_ptr<void>(data));
        }
        case fs_type_t::FAT32: {
            fat32_fsdata_t* data = new (malloc(sizeof(fat32_fsdata_t))) fat32_fsdata_t();
            if (!data)
                return false;

            if (!fat32_init(move(device), data)) {
                free(data);
                return false;
            }

            return vfs_mount_file_system(vfs, name, fs_type_t::FAT32, std::unique_ptr<void>(data));
        }
        case fs_type_t::UNKNOWN:
        default:
            return false;
    }

    return false;
}

bool vfs_mount_device(vfs_t* vfs, void* device, block_device_type_t type, const char* name) {
    if (!vfs || !device || !name)
        return false;
    
    if (type == block_device_type_t::UNKOWN)
        return false;

    uint8_t* buffer = nullptr;
    uint64_t logical_sector_size = 0;
    uint64_t lba_count = 0;
    switch (type) {
        case block_device_type_t::IDE:
            logical_sector_size = ((ide_device_t*)device)->logical_sector_size;
            lba_count = ((ide_device_t*)device)->lba_count;
            buffer = (uint8_t*)malloc(logical_sector_size);
            if (!buffer) return false;
            if (!ide_read((ide_device_t*)device, 0, buffer, ((ide_device_t*)device)->logical_sector_size)) {
                free(buffer);
                return false;
            }
            break;
        case block_device_type_t::AHCI:
            logical_sector_size = ((ahci_device_t*)device)->logical_sector_size;
            lba_count = ((ahci_device_t*)device)->lba_count;
            buffer = (uint8_t*)malloc(logical_sector_size);
            if (!buffer) return false;
            if (!ahci_read((ahci_device_t*)device, 0, buffer, ((ahci_device_t*)device)->logical_sector_size)) {
                free(buffer);
                return false;
            }
            break;
    }

    std::unique_ptr<block_device_t> initial_block_device = std::make_unique<block_device_t>();
    initial_block_device->disk_device = device;
    initial_block_device->type = type;
    initial_block_device->start_lba = 0;
    initial_block_device->end_lba = lba_count - 1;
    initial_block_device->block_size = logical_sector_size;

    if (vfs_mount_block_device(vfs, move(initial_block_device), name)) {
        free(buffer);
        return true;
    }

    // might be mbr
    if (!is_mbr(buffer, logical_sector_size)) {
        free(buffer);
        return false;
    }

    mbr_t* mbr = (mbr_t*)buffer;

    for (size_t i = 0; i < MBR_PARTITIONS; i++) {
        const mbr_entry_t* partition = &mbr->partitions[i];
        if (!mbr_is_entry_valid(partition))
            continue;

        std::unique_ptr<block_device_t> block_device = std::make_unique<block_device_t>();
        block_device->disk_device = device;
        block_device->type = type;
        block_device->start_lba = partition->lba_start;
        block_device->end_lba = partition->lba_start + partition->sector_count - 1;
        block_device->block_size = logical_sector_size;

        char* name_buffer = (char*)malloc(strlen(name) + 6);
        sprintf(name_buffer, strlen(name) + 6, "%sp%u", name, i);
        (void)vfs_mount_block_device(vfs, move(block_device), name_buffer);
        free(name_buffer);
    }

    free(buffer);
    return true;
}