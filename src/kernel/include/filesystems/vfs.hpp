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

#include "filesystems/filesystem.hpp"

// typedef int file_descriptor_t;

enum class vfs_node_type_t {
    FILE,
    DIRECTORY,
};

template <class T>
struct vfs_kv_t {
    string key;
    T value;
};

struct vfs_node_t {
    vfs_node_t* parent;
    linked_list<ptr::unique<vfs_node_t>> children {};
    vfs_node_type_t type;
    string name;
};

struct vfs_storage_interface_t {
    virtual ~vfs_storage_interface_t() = default;
    virtual bool read_file(const string& path, dynamic_array<uint8_t>* p_content) = 0;
    virtual bool write_file(const string& path, dynamic_array<uint8_t>* p_content) = 0;
    virtual bool create_directory(const string& path) = 0;
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
                for (auto& v : *value.get())
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
        vfs_kv_t<ptr::unique<dynamic_array<uint8_t>>> value {};
        value.key = path;
        value.value = ptr::make_unique<dynamic_array<uint8_t>>();
        for (const auto& v : *p_content)
            value.value->insert_back(v);

        storage.insert_back(move(value));
        return true;
    }

    bool create_directory(const string& path) override {
        return true;
    }

private:
    // TODO @since 30/07/2025 -- 22:31
    // create a simple map
    linked_list<vfs_kv_t<ptr::unique<dynamic_array<uint8_t>>>> storage {};
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

inline vfs_node_t* vfs_node_get_child(vfs_node_t* p_parent, const string& name) {
    for (auto& child : p_parent->children)
        if (child->name == name)
            return child.get();

    return nullptr;
}

class virtual_file_system {
public:
    virtual_file_system() :
        default_storage(ptr::make_unique<vfs_memory_storage>()),
        root(ptr::make_unique<vfs_node_t>()) {
        root->type = vfs_node_type_t::DIRECTORY;
    }

    bool create_directory(const string& path) {
        // mutex_lock_guard guard(&mutex);

        if (resolve_path(path))
            return false;

        vfs_node_t* created_directory = create_directories(path);
        vfs_storage_interface_t* storage = get_backend(path);
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
        new_node->name = filename;
        new_node->parent = parent_dir;
        new_node->type = vfs_node_type_t::FILE;
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
        new_node->name = filename;
        new_node->parent = parent_dir;
        new_node->type = vfs_node_type_t::FILE;
        parent_dir->children.insert_back(move(new_node));

        vfs_storage_interface_t* storage = get_backend(path);
        string relative_path = get_relative_path_in_backend(path);
        storage->write_file(relative_path, p_content);

        return true;
    }

    bool read_file(const string& path, dynamic_array<uint8_t>* p_content) {
        // mutex_lock_guard guard(&mutex);

        vfs_node_t* target = resolve_path(path);
        if (!target || target->type != vfs_node_type_t::FILE)
            return false;

        vfs_storage_interface_t* storage = get_backend(path);
        string relative_path = get_relative_path_in_backend(path);
        storage->read_file(relative_path, p_content);

        return true;
    }

    bool mount(const string& path, ptr::unique<vfs_storage_interface_t> storage) {
        if (!create_directories(path))
            return false;

        for (const auto& mount : mount_points)
            if (mount->mount_point == path)
                return false;

        ptr::unique<vfs_mount_point_t> mp = ptr::make_unique<vfs_mount_point_t>();
        mp->mount_point = path;
        mp->storage_interface = move(storage);
        mount_points.insert_back(move(mp));

        // TODO @since 31/07/2025 -- 10:44
        // add dirs / files to cache

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

private:
    vfs_node_t* resolve_path(const string& path) {
        if (path == "/" || path.length() == 0)
            return root.get();

        dynamic_array<string> path_parts = str_split(path, '/');
        vfs_node_t* current_node = root.get();

        for (const auto& part : path_parts) {
            if (current_node->type != vfs_node_type_t::DIRECTORY)
                return nullptr;

            current_node = vfs_node_get_child(current_node, part);
            if (!current_node)
                return nullptr;
        }

        return current_node;
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
                new_node->type = vfs_node_type_t::DIRECTORY;
                new_node->parent = current_node;
                new_node->name = part;
                vfs_node_t* p_new_node = new_node.get();
                current_node->children.insert_back(move(new_node));
                current_node = p_new_node;
            } else if (child_node->type == vfs_node_type_t::DIRECTORY) {
                current_node = child_node;
            } else {
                return nullptr;
            }
        }

        return current_node;
    }

    vfs_storage_interface_t* get_backend(const string& path) {
        string best = "";
        vfs_storage_interface_t* best_storage = default_storage.get();

        for (const auto& mount : mount_points) {
            if (path.find(mount->mount_point) == 0 &&
                mount->mount_point.length() > best.length()) {
                best = mount->mount_point;
                best_storage = mount->storage_interface.get();
            }
        }

        return best_storage;
    }

    string get_relative_path_in_backend(const string& path) {
        for (const auto& mount : mount_points) {
            if (path.find(mount->mount_point) == 0) {
                string relative = path.substr(mount->mount_point.length());
                return relative.length() == 0 ? "/" : relative;
            }
        }

        return path;
    }

    ptr::unique<vfs_node_t> root;
    linked_list<ptr::unique<vfs_mount_point_t>> mount_points;
    ptr::unique<vfs_storage_interface_t> default_storage;
};

#endif // __FILESYSTEMS_VFS_HPP__