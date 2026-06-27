//==========================================
/// @file       vfs.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __VFS_HPP__
#define __VFS_HPP__

#define FILE_DESCRIPTOR_INVALID MAX_UINT64

#include "common.hpp"
#include "std/string.hpp"
#include "std/map.hpp"
#include "std/pointer.hpp"
#include "drivers/storage/ahci.hpp"
#include "drivers/storage/ide.hpp"
#include "drivers/storage/block_device.hpp"

typedef u64 file_descriptor_t;
typedef file_descriptor_t fd_t;

enum class fs_type_t {
    UNKNOWN = 0,
    ISO9660,
    FAT32
};

struct vfs_mount_point_t {
    std::string name;
    fs_type_t type;
    std::unique_ptr<void> data;
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

struct vfs_storage_info_t {
    std::string serial;
    std::string firmware;
    std::string model;
    u64 capacity;
};

vfs_t* get_global_vfs();
void set_global_vfs(vfs_t* vfs);

void vfs_init(vfs_t* vfs);

bool vfs_mount_file_system(vfs_t* vfs, const char* name, fs_type_t type, std::unique_ptr<void> fs_data);
const vfs_mount_point_t* vfs_get_mount_point(vfs_t* vfs, const char* path);
file_descriptor_t vfs_open_file(vfs_t* vfs, const char* path);
bool vfs_close_file(vfs_t* vfs, file_descriptor_t fd);
bool vfs_read_file(vfs_t* vfs, file_descriptor_t fd, u8** data, size_t* size);
bool vfs_list_directory(vfs_t* vfs, const char* path, std::dynamic_array<vfs_node_t>* out_nodes);

bool vfs_mount_block_device(vfs_t* vfs, std::unique_ptr<block_device_t> device, const char* name);
bool vfs_mount_device(vfs_t* vfs, void* device, block_device_type_t type, const char* name);
bool vfs_get_storage_info(vfs_t* vfs, const char* path, vfs_storage_info_t* storage_info);

#endif // __VFS_HPP__