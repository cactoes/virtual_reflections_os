//==========================================
/// @file       initramfs.h
/// @brief      read-only cpio filesystem
//==========================================

#pragma once

#ifndef INITRAMFS_H
#define INITRAMFS_H

#define CPIO_HEADER_SIZE 110u
#define CPIO_PATH_MAX    4096u
#define CPIO_MODE_TYPE   0xf000u
#define CPIO_MODE_FILE   0x8000u
#define CPIO_MODE_DIR    0x4000u

#include "libtype.h"
#include "filesystem/vfs.h"

typedef struct initramfs_node initramfs_node_t;

struct initramfs_node {
    vfs_inode_t inode;
    char name[VFS_MAX_NAME];
    const u8* data;
    u32 mode;
    bool explicit_entry;
    initramfs_node_t* children;
    initramfs_node_t* last_child;
    initramfs_node_t* next_sibling;
    initramfs_node_t* next_allocated;
};

typedef struct initramfs_state {
    const u8* archive;
    u64 archive_size;
    u64 next_ino;
    initramfs_node_t* root;
    initramfs_node_t* allocated;
} initramfs_state_t;

bool initramfs_create(const void* archive, u64 archive_size, vfs_superblock_t** out);

#endif // INITRAMFS_H