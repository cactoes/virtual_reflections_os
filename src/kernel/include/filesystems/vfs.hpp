//==========================================
/// @file       vfs.hpp
/// @brief      virtual file system implementation
//==========================================

#pragma once

#ifndef __FILESYSTEMS_VFS_HPP__
#define __FILESYSTEMS_VFS_HPP__

#include "common.hpp"
#include "string.hpp"
#include "utils/pointer.hpp"
#include "utils/mutex.hpp"
#include "utils/vector.hpp"

template <typename T>
struct key_value_t {
    string key;
    T value;
};

enum class vfs_node_type_t {
    FILE,
    DIRECTORY
};

struct vfs_file_meta_t {
    string name;
    size_t size;
    vfs_node_type_t type;
};

class vfs_directory;

class vfs_node {
public:
    vfs_node(const string& name, vfs_node_type_t type);

    string get_path() const;

    virtual ~vfs_node() = default;

public:
    ptr::unique<vfs_file_meta_t> meta;
    vfs_directory* parent;
};

class vfs_file : public vfs_node {
public:
    vfs_file(const string& name)
        : vfs_node(name, vfs_node_type_t::FILE) {};

    void set_content(const vector<uint8_t>& data);

private:
    vector<uint8_t> content {};
    size_t position = 0;
};

class vfs_directory : public vfs_node {
public:
    vfs_directory(const string& name)
        : vfs_node(name, vfs_node_type_t::DIRECTORY) {};

    vfs_node* get_child(const string& name);
    void add_child(ptr::unique<vfs_node> node);

private:
    vector<key_value_t<ptr::unique<vfs_node>>> children {};
};

class vfs_storage_backend_interface {
public:
    virtual ~vfs_storage_backend_interface() = default;
    virtual bool read_file(const string& path, vector<uint8_t>& content) = 0;
    virtual bool write_file(const string& path, const vector<uint8_t>& content) = 0;
    virtual bool delete_file(const string& path) = 0;
    virtual vector<string> list_directory(const string& path) = 0;
    virtual bool create_directory(const string& path) = 0;
    virtual bool delete_directory(const string& path) = 0;
    virtual bool exists(const string& path) = 0;
};

class vfs_memory_storage_backend : vfs_storage_backend_interface {
public:
    vfs_memory_storage_backend() = default;

    bool read_file(const string& path, vector<uint8_t>& content) override;
    bool write_file(const string& path, const vector<uint8_t>& content) override;
    bool delete_file(const string& path) override;
    vector<string> list_directory(const string& path) override;
    bool create_directory(const string& path) override;
    bool delete_directory(const string& path) override;
    bool exists(const string& path) override;

private:
    vector<key_value_t<ptr::unique<vfs_node>>> storage {};
    mutex_t mutex {};
};

class vfs_disk_storage_backend : vfs_storage_backend_interface {
public:
    bool read_file(const string& path, vector<uint8_t>& content) override;
    bool write_file(const string& path, const vector<uint8_t>& content) override;
    bool delete_file(const string& path) override;
    vector<string> list_directory(const string& path) override;
    bool create_directory(const string& path) override;
    bool delete_directory(const string& path) override;
    bool exists(const string& path) override;

private:
    string root_path;
    mutex_t mutex {};
};

struct vfs_mount_point_t {
    string path;
    ptr::unique<vfs_storage_backend_interface> storage;
    string device_name;
    string filesystem_type;
};

class vfs {
public:
    vfs();

    bool mount(const string& vfs_path, ptr::unique<vfs_storage_backend_interface> backend, const string& device, const string& type);
    bool create_file(const string& vfs_path, vector<uint8_t>& content);

private:
    vector<string> split_path(const string& path);
    vfs_directory* create_directories_recursive(const string& path);
    vfs_node* resolve_path(const string& path);

    vfs_storage_backend_interface* get_backend_for_path(const string& path);
    string get_backend_relative_path(const string& vfs_path);

    ptr::unique<vfs_directory> root {};
    vector<ptr::unique<vfs_mount_point_t>> mount_points {};
    ptr::unique<vfs_storage_backend_interface> default_storage_backend {};
    mutex_t mutex {};
};

#endif // __FILESYSTEMS_VFS_HPP__