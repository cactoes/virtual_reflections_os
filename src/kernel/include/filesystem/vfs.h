//==========================================
/// @file       vfs.h
/// @brief      
//==========================================

#pragma once

#ifndef VFS_HP
#define VFS_HP

#define VFS_MAX_NAME    256

#define VFS_O_READ      (1u << 0)
#define VFS_O_WRITE     (1u << 1)

#include "libtype.h"

typedef struct __vfs_superblock vfs_superblock_t;
typedef struct __vfs_inode vfs_inode_t;
typedef struct __vfs_dentry vfs_dentry_t;
typedef struct __vfs_file vfs_file_t;

typedef enum __vfs_type {
    VFS_TYPE_FILE = 1,
    VFS_TYPE_DIR,
    VFS_TYPE_DEVICE
} vfs_type_t;

typedef struct __vfs_dirent {
    char name[VFS_MAX_NAME];
    vfs_type_t type;
    u64 ino;
} vfs_dirent_t;

typedef struct __vfs_inode_vtable {
    vfs_inode_t*(*find)(vfs_inode_t* dir, const char* name);

    u64(*read)(vfs_inode_t* inode, u64 offset, void* buffer, u64 size);
    bool(*write)(vfs_inode_t* inode, u64 offset, const void* buffer, u64 size);

    bool(*readdir)(vfs_inode_t* dir, u64 index, vfs_dirent_t* out);
} vfs_inode_vtable_t;

struct __vfs_superblock {
    vfs_dentry_t* root;
    vfs_dentry_t* mountpoint;
    void* private_data;
};

struct __vfs_inode {
    u64 ino;
    vfs_type_t type;
    u64 size;
    vfs_superblock_t* sb;
    const vfs_inode_vtable_t* ops;
    void* private_data;
};

struct __vfs_dentry {
    char name[VFS_MAX_NAME];
    vfs_dentry_t* parent;
    vfs_dentry_t* children;
    vfs_dentry_t* next_sibling;
    vfs_inode_t* inode;
    vfs_superblock_t* mounted;
};

struct __vfs_file {
    vfs_dentry_t* dentry;
    u64 offset;
    u32 flags;
};

bool vfs_mount_root(vfs_superblock_t* sb);
vfs_dentry_t* vfs_resolve(const char* path);
vfs_dentry_t* vfs_dentry_create(vfs_dentry_t* parent, const char* name, vfs_inode_t* inode);
bool vfs_mount(const char* path, vfs_superblock_t* sb);

bool vfs_open(const char* path, u32 flags, vfs_file_t** out);
u64 vfs_read(vfs_file_t* file, void* buffer, u64 size);
void vfs_close(vfs_file_t* file);

#endif // VFS_HP