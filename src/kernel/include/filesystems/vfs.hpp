//==========================================
/// @file       vfs.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __VFS_HPP__
#define __VFS_HPP__

#define FILE_DESCRIPTOR_INVALID     MAX_UINT64
#define MOUNT_POINT_MAX_NAME_LEN    256

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

struct vfs_node_t {
    std::string name;
    size_t size;
    bool is_directory;
};

struct filesystem_interface_t {
    const block_device_t*(*get_block_device)(void* filesystem_data);
    bool(*read)(void* filesystem_data, const char* path, u8** out, u64* size);
    bool(*file_exists)(void* filesystem_data, const char* path);
    bool(*enumerate_directory)(void* filesystem_data, const char* path, std::dynamic_array<vfs_node_t>* out);
};

struct vfs_mount_point_t {
    char name[MOUNT_POINT_MAX_NAME_LEN];
    const filesystem_interface_t* interface;
    void* filesystem_data;
};

struct vfs_t {
    std::linear_map<std::string, vfs_mount_point_t> mount_points;
    std::linear_map<file_descriptor_t, std::string> file_handles;
    file_descriptor_t last_fd;
};

vfs_t* get_global_vfs();
void set_global_vfs(vfs_t* vfs);

void vfs_init(vfs_t* vfs);

bool vfs_mount_block_device(vfs_t* vfs, block_device_t* block_device, const char* name);

file_descriptor_t vfs_open_file(vfs_t* vfs, const char* path);
bool vfs_close_file(vfs_t* vfs, file_descriptor_t fd);
bool vfs_read_file(vfs_t* vfs, file_descriptor_t fd, u8** data, u64* size);
bool vfs_list_directory(vfs_t* vfs, const char* path, std::dynamic_array<vfs_node_t>* out_nodes);

#endif // __VFS_HPP__