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
#include "utils/map.hpp"
#include "utils/debug.hpp"

#include "filesystems/filesystem.hpp"

#define FILE_DESCRIPTOR_INVALID (int)-1

typedef int file_descriptor_t;

struct vfs_storage_interface_t {
    virtual ~vfs_storage_interface_t() = default;
    virtual bool read_file(const string& path, dynamic_array<uint8_t>* p_content) = 0;
    virtual bool write_file(const string& path, dynamic_array<uint8_t>* p_content) = 0;
    virtual bool create_directory(const string& path) = 0;
};

enum class vfs_node_type_t {
    FILE,
    DIRECTORY,
};

struct vfs_node_meta_t {
    string name;
    vfs_node_type_t type;
    struct {
        bool read       : 1;
        bool write      : 1;
        bool execute    : 1;
    } PACKED;
};

struct vfs_node_t {
    vfs_node_t* parent;
    linked_list<ptr::unique<vfs_node_t>> children {};
    vfs_node_meta_t meta;

    vfs_storage_interface_t* storage = nullptr;
};

struct vfs_mount_point_t {
    string mount_point;
    ptr::unique<vfs_storage_interface_t> storage_interface;
};

class vfs_memory_storage : public vfs_storage_interface_t {
public:
    vfs_memory_storage() { mutex_init(&mutex); }

    bool read_file(const string& path, dynamic_array<uint8_t>* p_content) override {
        mutex_lock_guard guard(&mutex);
        for (auto& [ key, value ] : storage) {
            if (key == path) {
                // TODO @since 30/07/2025 -- 22:34
                // speed up, for now its okay since its still testing
                for (auto& v : *value)
                    p_content->insert_back(v);
                return true;
            }
        }

        return false;
    }

    bool write_file(const string& path, dynamic_array<uint8_t>* p_content) override {
        mutex_lock_guard guard(&mutex);
        for (auto& [ key, value ] : storage) {
            if (key == path) {
                for (const auto& v : *p_content)
                    value->insert_back(v);
                return true;
            }
        }
        ptr::unique<dynamic_array<uint8_t>> value {};
        value = ptr::make_unique<dynamic_array<uint8_t>>();
        for (const auto& v : *p_content)
            value->insert_back(v);

        storage.insert(path, move(value));
        return true;
    }

    bool create_directory(const string& path) override {
        return true;
    }

private:
    linear_map<string, ptr::unique<dynamic_array<uint8_t>>> storage;
    mutex_t mutex;
};

class vfs_disk_storage : public vfs_storage_interface_t {
public:
    vfs_disk_storage(filesystem_api_t* p_fs_api) : fs_api(p_fs_api) { mutex_init(&mutex); }

    bool read_file(const string& path, dynamic_array<uint8_t>* p_content) override {
        mutex_lock_guard guard(&mutex);
        void* data;
        size_t size;
        fs_api->read(fs_api, path.c_str(), &data, &size);
        p_content->assign((uint8_t*)data, size / sizeof(uint8_t));
        return true;
    }

    bool write_file(const string& path, dynamic_array<uint8_t>* p_content) override {
        mutex_lock_guard guard(&mutex);
        size_t size = p_content->length() * sizeof(uint8_t);
        uint8_t* data = p_content->get_data();

        fs_api->write(fs_api, path.c_str(), data, &size);
        return false;
    }

    bool create_directory(const string& path) override {
        return false;
    }

private:
    filesystem_api_t* fs_api;
    mutex_t mutex;
};

class vfs_dbg_stream : public vfs_storage_interface_t {
public:
    vfs_dbg_stream() {
        debug_init();
        mutex_init(&mutex);
    }

    bool read_file(const string& path, dynamic_array<uint8_t>* p_content) override {
        return false;
    }

    bool write_file(const string& path, dynamic_array<uint8_t>* p_content) override {
        for (char ch : *p_content)
            debug_putc(ch);
        return true;
    }

    bool create_directory(const string& path) override {
        return false;
    }

private:
    mutex_t mutex;
};

inline vfs_node_t* vfs_node_get_child(vfs_node_t* p_parent, const string& name) {
    for (auto& child : p_parent->children)
        if (child->meta.name == name)
            return child.get();

    return nullptr;
}

class virtual_file_system {
public:
    virtual_file_system() :
        default_storage(ptr::make_unique<vfs_memory_storage>()),
        root(ptr::make_unique<vfs_node_t>()) {
        root->meta.type = vfs_node_type_t::DIRECTORY;
        root->storage = default_storage.get();
    }

    bool create_directory(const string& path) {
        // mutex_lock_guard guard(&mutex);

        if (resolve_path(path))
            return false;

        vfs_node_t* created_directory = create_directories(path);
        vfs_storage_interface_t* storage = get_storage_interface(path);
        string relative_path = get_relative_path_in_backend(path);
        storage->create_directory(relative_path);

        return true;
    }

    bool create_file_cache(const string& path) {

        size_t last_slash = path.find_last_of('/');
        string dir_path = (last_slash != string::k_npos) ? path.substr(0, last_slash) : "/";
        string filename = (last_slash != string::k_npos) ? path.substr(last_slash + 1) : path;

        vfs_node_t* parent_dir = create_directories(dir_path);

        ptr::unique<vfs_node_t> new_node = ptr::make_unique<vfs_node_t>();
        new_node->meta.name = filename;
        new_node->parent = parent_dir;
        new_node->meta.type = vfs_node_type_t::FILE;
        parent_dir->children.insert_back(move(new_node));

        return true;
    }

    bool create_file(const string& path, dynamic_array<uint8_t>* p_content) {
        // mutex_lock_guard guard(&mutex);

        size_t last_slash = path.find_last_of('/');
        string dir_path = (last_slash != string::k_npos) ? path.substr(0, last_slash) : "/";
        string filename = (last_slash != string::k_npos) ? path.substr(last_slash + 1) : path;

        vfs_node_t* parent_dir = create_directories(dir_path);

        ptr::unique<vfs_node_t> new_node = ptr::make_unique<vfs_node_t>();
        new_node->meta.name = filename;
        new_node->parent = parent_dir;
        new_node->meta.type = vfs_node_type_t::FILE;
        parent_dir->children.insert_back(move(new_node));

        vfs_storage_interface_t* storage = get_storage_interface(path);
        string relative_path = get_relative_path_in_backend(path);
        storage->write_file(relative_path, p_content);

        return true;
    }

    bool read_file(file_descriptor_t fd, dynamic_array<uint8_t>* p_content) {
        // mutex_lock_guard guard(&mutex);

        auto entry = open_files.get(fd);
        if (entry == open_files.end())
            return false;

        vfs_storage_interface_t* storage = get_storage_interface(entry->value);
        string relative_path = get_relative_path_in_backend(entry->value);
        return storage->read_file(relative_path, p_content);
    }

    bool write_file(file_descriptor_t fd, dynamic_array<uint8_t>* p_content) {
        auto entry = open_files.get(fd);
        if (entry == open_files.end())
            return false;

        vfs_storage_interface_t* storage = get_storage_interface(entry->value);
        string relative_path = get_relative_path_in_backend(entry->value);
        return storage->write_file(relative_path, p_content);
    }

    file_descriptor_t open_file(const string& path) {
        // mutex_lock_guard guard(&mutex);

        vfs_node_t* target = resolve_path(path);
        if (!target) // || target->meta.type != vfs_node_type_t::FILE
            return FILE_DESCRIPTOR_INVALID;

        file_descriptor_t file_descriptor = get_next_descriptor();
        open_files[file_descriptor] = path;

        return file_descriptor;
    }

    bool close_file(file_descriptor_t fd) {
        return open_files.remove(fd);
    }

    bool mount(const string& path, ptr::unique<vfs_storage_interface_t> storage) {
        auto node = create_directories(path);
        if (!node || node->storage != nullptr)
            return false;

        for (const auto& mount : mount_points)
            if (mount->mount_point == path)
                return false;

        node->storage = storage.get();

        ptr::unique<vfs_mount_point_t> mp = ptr::make_unique<vfs_mount_point_t>();
        mp->mount_point = path;
        mp->storage_interface = move(storage);
        mount_points.insert_back(move(mp));

        // TODO @since 31/07/2025 -- 10:44
        // do drive enumeration

        return true;
    }

    bool unmount(const string& path) {
        // TODO @since 31/07/2025 -- 10:45
        // remove dirs / file from cache

        size_t counter = 0;
        for (const auto& mount : mount_points) {
            if (mount->mount_point == path)
                return mount_points.delete_at(counter);

            counter++;
        }

        return false;
    }

    dynamic_array<string> list_folder_entries(const string& path) {
        dynamic_array<string> dirs {};

        vfs_node_t* target = resolve_path(path);
        if (!target)
            return dirs;

        for (auto& child : target->children)
            dirs.insert_back(child->meta.name);

        return dirs;
    }

    const vfs_node_meta_t* get_meta(file_descriptor_t fd) {
        auto entry = open_files.get(fd);
        if (entry == open_files.end())
            return nullptr;

        return &resolve_path(entry->value)->meta;
    }

private:
    vfs_node_t* resolve_path(const string& path) {
        if (path == "/" || path.length() == 0)
            return root.get();

        dynamic_array<string> path_parts = str_split(path, '/');
        vfs_node_t* current_node = root.get();

        for (const auto& part : path_parts) {
            if (current_node->meta.type != vfs_node_type_t::DIRECTORY)
                return nullptr;

            current_node = vfs_node_get_child(current_node, part);
            if (!current_node)
                return nullptr;
        }

        return current_node;
    }

    string node_to_path(vfs_node_t* p_node) {
        if (!p_node->parent)
            return "/";

        string path = "";
        vfs_node_t* current = p_node;

        while (current->parent) {
            path = string("/") + current->meta.name + path;
            current = current->parent;
        }

        return path;
    }

    vfs_node_t* create_directories(const string& path) {
        if (path == "/" || path.length() == 0)
            return root.get();
        
        dynamic_array<string> path_parts = str_split(path, '/');
        vfs_node_t* current_node = root.get();

        for (const auto& part : path_parts) {
            vfs_node_t* child_node = vfs_node_get_child(current_node, part);
            if (!child_node) {
                ptr::unique<vfs_node_t> new_node = ptr::make_unique<vfs_node_t>();
                new_node->meta.type = vfs_node_type_t::DIRECTORY;
                new_node->parent = current_node;
                new_node->meta.name = part;
                vfs_node_t* p_new_node = new_node.get();
                current_node->children.insert_back(move(new_node));
                current_node = p_new_node;
            } else if (child_node->meta.type == vfs_node_type_t::DIRECTORY) {
                current_node = child_node;
            } else {
                return nullptr;
            }
        }

        return current_node;
    }

    vfs_storage_interface_t* get_storage_interface(const string& path) {
        vfs_node_t* node = resolve_path(path);
        while (node) {
            if (node->storage)
                return node->storage;
            node = node->parent;
        }

        return default_storage.get();
    }

    string get_relative_path_in_backend(const string& path) {
        vfs_node_t* node = resolve_path(path);
        while (node) {
            if (node->storage) {
                string relative_path = path.substr(node_to_path(node).length());
                return relative_path.length() == 0 ? "/" : relative_path;
            }
            node = node->parent;
        }

        return path;
    }

    file_descriptor_t get_next_descriptor() {
        // for now just do linear descriptors
        static int s_counter = 0;
        return s_counter++;
    }

    ptr::unique<vfs_node_t> root;
    linked_list<ptr::unique<vfs_mount_point_t>> mount_points {};
    ptr::unique<vfs_storage_interface_t> default_storage;
    linear_map<file_descriptor_t, string> open_files {};
};

#endif // __FILESYSTEMS_VFS_HPP__