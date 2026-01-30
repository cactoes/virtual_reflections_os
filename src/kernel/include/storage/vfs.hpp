#pragma once

#ifndef __VFS_HPP__
#define __VFS_HPP__

#define FILE_DESCRIPTOR_INVALID MAX_UINT64

#include "common.hpp"
#include "std/string.hpp"
#include "std/map.hpp"

typedef uint64_t file_descriptor_t;

enum class fs_type_t {
    UNKNOWN = 0,
    ISO9660
};

struct vfs_mount_point_t {
    std::string name;
    fs_type_t type;
    void* data;
};

struct vfs_node_t {
    std::string name;
    size_t size;
    bool is_directory;
};

struct vfs_t {
    std::linear_map<std::string, vfs_mount_point_t> mount_points;
    std::linear_map<file_descriptor_t, std::string> file_handles;
    file_descriptor_t last_fd;
};

vfs_t* get_global_vfs();
void set_global_vfs(vfs_t* vfs);

void vfs_init(vfs_t* vfs);

bool vfs_mount_file_system(vfs_t* vfs, const char* name, fs_type_t type, void* fs_data);
const vfs_mount_point_t* vfs_get_mount_point(vfs_t* vfs, const char* path);
file_descriptor_t vfs_open_file(vfs_t* vfs, const char* path);
bool vfs_close_file(vfs_t* vfs, file_descriptor_t fd);
bool vfs_read_file(vfs_t* vfs, file_descriptor_t fd, uint8_t** data, size_t* size);
bool vfs_list_directory(vfs_t* vfs, const char* path, std::dynamic_array<vfs_node_t>* out_nodes);

#endif // __VFS_HPP__