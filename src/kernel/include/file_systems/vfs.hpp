//==========================================
/// @file       vfs.hpp
/// @brief      virtual file system stuff
//==========================================

#pragma once

#ifndef __VFS_HPP__
#define __VFS_HPP__

#include "common.hpp"
#include "vector.hpp"

enum class vfs_node_type_t {
    FILE,
    DIRECTORY,
    BLOCK_DEVICE,
    CHAR_DEVICE
};

enum class fs_type_t {
    NONE,
    ISO9660,
    FAT32
};

enum class drive_type_t {
    NONE,
    ATAPI,
    SATA
};

struct drive_t {
    drive_type_t type;
    void* device;
    
    int (*read)(drive_t*, uint32_t lba, void* buffer, size_t* size);
    int (*write)(drive_t*, uint32_t lba, void* buffer, size_t* size);
};

struct fs_t {
    fs_type_t type;
    void* data;

    int (*read)(fs_t*, drive_t* drive, const char* file_path, void** buffer, size_t* size);
    int (*write)(fs_t*, drive_t* drive, const char* file_path, void* buffer, size_t* size);
};

struct vfs_node_t {
    const char* name;
    vfs_node_type_t node_type;

    vfs_node_t* parent;
    vector<vfs_node_t*> children;

    drive_t drive;
    fs_t fs;
};

enum class vfs_file_type_t {
    DEFAULT = 0,
    SYS_STD_OUT,
    SYS_STD_ERR,
    SYS_DBG,
    UNKNOWN
};

struct vfs_file_t {
    vfs_file_type_t file_type = vfs_file_type_t::DEFAULT;
    void* buffer;
    size_t size;
    size_t readptr;
};

void vfs_init();
vfs_node_t* vfs_get_root();
vfs_node_t* vfs_mount_dev(const char* name, drive_type_t drive_type, fs_type_t fs_type);
void vfs_read(const char* file, vfs_file_t* file_ptr);
int vfs_write(const char* file, vfs_file_t* file_ptr, const char* data, size_t size);
void vfs_close_file(vfs_file_t* file);
bool vfs_consume(vfs_file_t* file, void* out, size_t size);

#endif // __VFS_HPP__