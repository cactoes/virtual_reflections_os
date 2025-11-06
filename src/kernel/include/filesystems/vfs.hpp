//==========================================
/// @file       vfs.hpp
/// @brief      virtual file system implementation
/// TODO        rethink the the mounting & disk stuff etc ...
//==========================================

#pragma once

#ifndef __FILESYSTEMS_VFS_HPP__
#define __FILESYSTEMS_VFS_HPP__

#include "common.hpp"
#include "std/string.hpp"

#include "std/pointer.hpp"
#include "utils/mutex.hpp"
#include "std/array.hpp"
#include "std/map.hpp"
#include "utils/debug.hpp"
#include "utils/vector.hpp"

#include "filesystems/filesystem.hpp"

#define FILE_DESCRIPTOR_INVALID (int)-1

struct vfs_node_t;

typedef int file_descriptor_t;

struct vfs_storage_info_t {
    std::string model;
    std::string serial;
    std::string firmare;

    size_t capacity;
};

struct vfs_storage_interface_t {
    virtual ~vfs_storage_interface_t() = default;

    virtual bool read_file(const std::string& path, std::dynamic_array<uint8_t>* content) = 0;

    virtual bool write_file(const std::string& path, std::dynamic_array<uint8_t>* content) = 0;
    virtual bool write_file(const std::string& path, uint8_t* content, size_t size) = 0;

    virtual bool enumerate_directory(const std::string& path, std::dynamic_array<std::unique_ptr<vfs_node_t>>* out_array) = 0;

    virtual bool create_directory(const std::string& path) = 0;

    virtual vfs_storage_info_t get_storage_info() const = 0;
};

class vfs_memory_storage_interface : public vfs_storage_interface_t {
public:
    vfs_memory_storage_interface();

    bool read_file(const std::string& path, std::dynamic_array<uint8_t>* content) override;

    bool write_file(const std::string& path, std::dynamic_array<uint8_t>* content) override;
    bool write_file(const std::string& path, uint8_t* content, size_t size) override;

    bool enumerate_directory(const std::string& path, std::dynamic_array<std::unique_ptr<vfs_node_t>>* out_array) {
        return false;
    }

    bool create_directory(const std::string& path) override;

    vfs_storage_info_t get_storage_info() const {
        return vfs_storage_info_t{};
    }

private:
    std::linear_map<std::string, std::unique_ptr<std::dynamic_array<uint8_t>>> storage;
    mutex_t mutex;
};

class vfs_disk_storage_interface : public vfs_storage_interface_t {
public:
    vfs_disk_storage_interface(std::unique_ptr<filesystem_interface_t> api);

    bool read_file(const std::string& path, std::dynamic_array<uint8_t>* content) override;

    bool write_file(const std::string& path, std::dynamic_array<uint8_t>* content) override;
    bool write_file(const std::string& path, uint8_t* content, size_t size) override;

    bool enumerate_directory(const std::string& path, std::dynamic_array<std::unique_ptr<vfs_node_t>>* out_array) override;

    bool create_directory(const std::string& path) override;

    vfs_storage_info_t get_storage_info() const {
        storage_info_t storage_info = api->get_storage_interface()->get_storage_info();

        vfs_storage_info_t info {};
        info.model = move(storage_info.model);
        info.serial = move(storage_info.serial);
        info.firmare = move(storage_info.firmare);
        info.capacity = storage_info.capacity;
        return info;
    }

private:
    std::unique_ptr<filesystem_interface_t> api;
    mutex_t mutex;
};

class vfs_out_stream_interface : public vfs_storage_interface_t {
public:
    vfs_out_stream_interface(void(*writer)(const char*));

    bool read_file(const std::string& path, std::dynamic_array<uint8_t>* content) override {
        return false;
    }

    bool write_file(const std::string& path, std::dynamic_array<uint8_t>* content) override;
    bool write_file(const std::string& path, uint8_t* content, size_t size) override;

    bool enumerate_directory(const std::string& path, std::dynamic_array<std::unique_ptr<vfs_node_t>>* out_array);

    bool create_directory(const std::string& path) override {
        return false;
    }

    vfs_storage_info_t get_storage_info() const {
        return vfs_storage_info_t{};
    }

private:
    void(*writer_fn)(const char*);
    mutex_t mutex;
};

struct vfs_mount_point_t {
    std::string mount_point_path;
    std::unique_ptr<vfs_storage_interface_t> interface;
};

enum class vfs_node_type_t {
    // basic file
    FILE,

    // basic directory
    DIRECTORY,

    // interfaceable device (unimplemented)
    DEVICE,
};

struct vfs_node_meta_t {
    std::string name;
    struct {
        bool read       : 1;
        bool write      : 1;
        bool execute    : 1;
    } PACKED permissions;

    struct {
        bool is_mount_point    : 1;
        bool is_interface_root : 1;
    } PACKED flags;
};

struct vfs_node_t {
    // node info
    vfs_node_type_t type;
    vfs_node_meta_t meta;

    // pointer to parent node
    vfs_node_t* parent;

    // pointer to the root of the mount point
    vfs_node_t* root_mount_point;
    vfs_mount_point_t* mount_point;

    // pointer to the root that has the storage interface
    vfs_node_t* root_storage_interface;
    vfs_storage_interface_t* storage_interface;

    // list of children
    linked_list<std::unique_ptr<vfs_node_t>> children {};
};

struct vfs_t {
    // for open files / file descriptors
    mutex_t fd_mutex {};

    // for storage interfaces
    mutex_t si_mutex {};

    // for node tree
    mutex_t nt_mutex {};

    std::unique_ptr<vfs_storage_interface_t> root_storage_interface;
    std::unique_ptr<vfs_node_t> root_node;

    std::linear_map<file_descriptor_t, std::string> open_files {};
    linked_list<std::unique_ptr<vfs_mount_point_t>> mount_points {};

    int fd_counter = 0;
};

void set_global_vfs(vfs_t* vfs);
vfs_t* get_global_vfs();

bool vfs_init(vfs_t* vfs);
bool vfs_create_directory(vfs_t* vfs, const std::string& path);
bool vfs_create_file(vfs_t* vfs, const std::string& path);
file_descriptor_t vfs_open_file(vfs_t* vfs, const std::string& path);
bool vfs_close_file(vfs_t* vfs, file_descriptor_t fd);
bool vfs_read_file(vfs_t* vfs, file_descriptor_t fd, std::dynamic_array<uint8_t>* content);
bool vfs_write_file(vfs_t* vfs, file_descriptor_t fd, std::dynamic_array<uint8_t>* content);
bool vfs_write_file(vfs_t* vfs, file_descriptor_t fd, uint8_t* content, size_t size);
bool vfs_mount(vfs_t* vfs, const std::string& path, std::unique_ptr<vfs_storage_interface_t> storage_interface);
bool vfs_add_file_cache(vfs_t* vfs, const std::string& path);
const vfs_node_meta_t* vfs_get_meta(vfs_t* vfs, file_descriptor_t fd);
bool vfs_list_directory(vfs_t* vfs, const std::string& path, std::dynamic_array<vfs_node_t*>* out_array);
bool vfs_get_disk_info(vfs_t* vfs, const std::string& path, vfs_storage_info_t* disk_info);

/*
// void open_file("/COM/x"); // void open_file("/pipe/x");

1. select driver based on path
2. based on driver check operation etc
if disk -> select storage driver

- void open_file("/device/x");
    -> select file system driver

- fat32 / iso9660 / ...
    -> select storage driver

- usb / sata / ide
    -> perform read

- void open_file("/pipe/x");
    -> select pipe driver
    -> write to named pipe
*/

#endif // __FILESYSTEMS_VFS_HPP__