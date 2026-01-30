#include "storage/vfs.hpp"
#include "storage/filesystems/iso9660.hpp"

static vfs_t* global_vfs = nullptr;

vfs_t* get_global_vfs() {
    return global_vfs;
}

void set_global_vfs(vfs_t* vfs) {
    global_vfs = vfs;
}

bool vfs_mount_file_system(vfs_t* vfs, const char* name, fs_type_t type, void* fs_data) {
    if (vfs->mount_points.contains(std::string(name)))
        return false;

    vfs->mount_points[std::string(name)] = { .name = name, .type = type, .data = fs_data  };
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
        return false;

    const std::string mount_point_relative_path = get_mount_point_relative_path(mount_point, path);

    bool file_exists = false;
    switch (mount_point->type) {
        case fs_type_t::ISO9660:
            file_exists = iso9660_file_exists((iso9660_fsdata_t*)mount_point->data, mount_point_relative_path.c_str());
            break;
        default:
            file_exists = false;
            break;
    }

    if (!file_exists)
        return false;

    // TODO @since 30/01/2026 -- 01:13
    // better fd logic
    const file_descriptor_t file_descriptor = vfs->last_fd++;
    if (file_descriptor == FILE_DESCRIPTOR_INVALID)
        return false;

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
            return iso9660_read((iso9660_fsdata_t*)mount_point->data, mount_point_relative_path.c_str(), data, size);
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
            if (!iso9660_list_directory((iso9660_fsdata_t*)mount_point->data, mount_point_relative_path.c_str(), &iso9660_nodes))
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
        default:
            return false;
    }

    return false;
}