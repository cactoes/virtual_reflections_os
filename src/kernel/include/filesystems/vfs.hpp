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
    vector<ptr::unique<vfs_node_t>> children {};
    vfs_node_type_t type;
    string name;
};

struct vfs_storage_interface_t {
    virtual ~vfs_storage_interface_t() = default;
    virtual bool read_file(const string& path, vector<uint8_t>* p_content) = 0;
    virtual bool write_file(const string& path, vector<uint8_t>* p_content) = 0;
    virtual bool create_directory(const string& path) = 0;
};

struct vfs_mount_point_t {
    string mount_point;
    ptr::unique<vfs_storage_interface_t> storage_interface;
};

class vfs_memory_storage : public vfs_storage_interface_t {
public:
    vfs_memory_storage() { mutex_init(&mutex); }

    bool read_file(const string& path, vector<uint8_t>* p_content) override {
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

    bool write_file(const string& path, vector<uint8_t>* p_content) override {
        mutex_lock_guard guard(&mutex);
        for (auto& [ key, value ] : storage) {
            if (key == path) {
                for (const auto& v : *p_content)
                    value->insert_back(v);
                return true;
            }
        }
        vfs_kv_t<ptr::unique<vector<uint8_t>>> value {};
        value.key = path;
        value.value = ptr::make_unique<vector<uint8_t>>();
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
    vector<vfs_kv_t<ptr::unique<vector<uint8_t>>>> storage {};
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

    bool create_file(const string& path, vector<uint8_t>* p_content) {
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

    bool read_file(const string& path, vector<uint8_t>* p_content) {
        // mutex_lock_guard guard(&mutex);

        vfs_node_t* target = resolve_path(path);
        if (!target || target->type != vfs_node_type_t::FILE)
            return false;

        vfs_storage_interface_t* storage = get_backend(path);
        string relative_path = get_relative_path_in_backend(path);
        storage->read_file(relative_path, p_content);

        return true;
    }

private:
    vfs_node_t* resolve_path(const string& path) {
        if (path == "/" || path.length() == 0)
            return root.get();

        vector<string> path_parts = str_split(path, '/');
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
        
        vector<string> path_parts = str_split(path, '/');
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
    vector<ptr::unique<vfs_mount_point_t>> mount_points;
    ptr::unique<vfs_storage_interface_t> default_storage;
};

// template <typename T>
// struct key_value_t {
//     string key;
//     T value;
// };

// enum class vfs_node_type_t {
//     FILE,
//     DIRECTORY,
// };

// struct vfs_node_t {
//     vfs_node_t* parent;
//     vector<ptr::unique<vfs_node_t>> children {};
//     vfs_node_type_t type;
//     string name;

//     ptr::unique<uint8_t> data;
//     size_t data_size;
// };

// struct virtual_file_system_t {
//     mutex_t mutex;
//     vfs_node_t root;

//     virtual_file_system_t() {
//         root.name = "/";
//         root.type = vfs_node_type_t::DIRECTORY;
//         root.parent = nullptr;

//         mutex_init(&mutex);
//     }
// };

// typedef int file_descriptor_t;

// inline vfs_node_t* internal_vfs_get_child(vfs_node_t* p_node, const string& name) {
//     for (const auto& child : p_node->children) {
//         if (child->name == name)
//             return child.get();
//     }

//     return nullptr;
// }

// inline vfs_node_t* internal_vfs_reslove_path(virtual_file_system_t* p_vfs, const string& path) {
//     if (path == "/" || path.length() == 0)
//         return &p_vfs->root;

//     auto parts = str_split(path, '/');
//     vfs_node_t* current = &p_vfs->root;

//     for (const auto& part : parts) {
//         if (current->type == vfs_node_type_t::DIRECTORY) {
//             current = internal_vfs_get_child(current, part);
//             if (!current)
//                 return nullptr;
//         } else {
//             return nullptr;
//         }
//     }

//     return current;
// }

// inline ptr::unique<vfs_node_t> internal_vfs_create_child(const string& name, vfs_node_t* p_parent, vfs_node_type_t type) {
//     auto new_node = ptr::make_unique<vfs_node_t>();

//     new_node->name = name;
//     new_node->parent = p_parent;
//     new_node->type = vfs_node_type_t::DIRECTORY;

//     return move(new_node);
// }

// inline vfs_node_t* internal_vfs_create_directories(virtual_file_system_t* p_vfs, const string& path) {
//     if (path == "/" || path.length() == 0)
//         return &p_vfs->root;

//     auto parts = str_split(path, '/');
//     vfs_node_t* current = &p_vfs->root;

//     for (const auto& part : parts) {
//         vfs_node_t* child = internal_vfs_get_child(current, part);

//         if (!child) {
//             auto new_node = internal_vfs_create_child(part, current, vfs_node_type_t::DIRECTORY);
//             vfs_node_t* p_new_node = new_node.get();
//             current->children.insert_back(move(new_node));
//             current = p_new_node;
//         } else if (child->type == vfs_node_type_t::DIRECTORY) {
//             current = child;
//         } else {
//             return nullptr;
//         }
//     }

//     return current;
// }

// inline bool vfs_create_file(virtual_file_system_t* p_vfs, const string& path, const uint8_t* p_data, size_t size) {
//     mutex_lock_guard guard(&p_vfs->mutex);

//     size_t last_slash = path.find_last_of('/');
//     string dir_path = (last_slash != string::k_npos) ? path.substr(0, last_slash) : "/";
//     string filename = (last_slash != string::k_npos) ? path.substr(last_slash + 1) : path;

//     vfs_node_t* parent_directory = internal_vfs_create_directories(p_vfs, dir_path);
//     if (!parent_directory || internal_vfs_get_child(parent_directory, filename))
//         return false;

//     // store in memory / cache / local for now
//     auto new_node = internal_vfs_create_child(filename, parent_directory, vfs_node_type_t::FILE);
//     new_node->data = ptr::unique<uint8_t>((uint8_t*)GALLOC(size));
//     new_node->data_size = size;
//     memcpy(new_node->data.get(), p_data, size);
//     parent_directory->children.insert_back(move(new_node));

//     // todo, store in backend

//     return true;
// }

// inline bool vfs_read_file(virtual_file_system_t* p_vfs, const string& path, void** p_data, size_t* p_size) {
//     mutex_lock_guard guard(&p_vfs->mutex);

//     vfs_node_t* node = internal_vfs_reslove_path(p_vfs, path);
//     if (node->type == vfs_node_type_t::FILE) {
//         *p_data = node->data.get();
//         *p_size = node->data_size;
//         return true;
//     }

//     return false;
// }

// inline bool vfs_create_directory(virtual_file_system_t* p_vfs, const string& path) {
//     mutex_lock_guard guard(&p_vfs->mutex);

//     if (internal_vfs_reslove_path(p_vfs, path))
//         return false;

//     vfs_node_t* created_directory = internal_vfs_create_directories(p_vfs, path);
    
//     // todo, save to backend

//     return created_directory != nullptr;
// }

// template <typename T>
// struct key_value_t {
//     string key;
//     T value;

//     key_value_t(string k, T v) : key(move(k)), value(move(v)) {}
// };

// enum class vfs_node_type_t {
//     FILE,
//     DIRECTORY
// };

// struct vfs_file_meta_t {
//     string name;
//     size_t size;
//     vfs_node_type_t type;

//     vfs_file_meta_t(string str, vfs_node_type_t type) : name(move(str)), type(type) {}
// };

// class vfs_directory;

// class vfs_node {
// public:
//     vfs_node(const string& name, vfs_node_type_t type);

//     string get_path() const;

//     virtual ~vfs_node() = default;

// public:
//     ptr::unique<vfs_file_meta_t> meta;
//     vfs_directory* parent;
// };

// class vfs_file : public vfs_node {
// public:
//     vfs_file(const string& name)
//         : vfs_node(name, vfs_node_type_t::FILE) {};

//     void set_content(const vector<uint8_t>& data);
//     const vector<uint8_t>& get_content() const;

// private:
//     vector<uint8_t> content {};
//     size_t position = 0;
// };

// class vfs_directory : public vfs_node {
// public:
//     vfs_directory(const string& name)
//         : vfs_node(name, vfs_node_type_t::DIRECTORY) {};

//     vfs_node* get_child(const string& name);
//     void add_child(ptr::unique<vfs_node> node);

// private:
//     vector<key_value_t<ptr::unique<vfs_node>>> children {};
// };

// class vfs_storage_backend_interface {
// public:
//     virtual ~vfs_storage_backend_interface() = default;
//     virtual bool read_file(const string& path, vector<uint8_t>& content) = 0;
//     virtual bool write_file(const string& path, const vector<uint8_t>& content) = 0;
//     virtual bool delete_file(const string& path) = 0;
//     virtual vector<string> list_directory(const string& path) = 0;
//     virtual bool create_directory(const string& path) = 0;
//     virtual bool delete_directory(const string& path) = 0;
//     virtual bool exists(const string& path) = 0;
// };

// class vfs_memory_storage_backend : public vfs_storage_backend_interface {
// public:
//     vfs_memory_storage_backend() = default;

//     bool read_file(const string& path, vector<uint8_t>& content) override;
//     bool write_file(const string& path, const vector<uint8_t>& content) override;
//     bool delete_file(const string& path) override;
//     vector<string> list_directory(const string& path) override;
//     bool create_directory(const string& path) override;
//     bool delete_directory(const string& path) override;
//     bool exists(const string& path) override;

// private:
//     vector<key_value_t<vector<uint8_t>>> storage {};
//     mutex_t mutex {};
// };

// class vfs_disk_storage_backend : public vfs_storage_backend_interface {
// public:
//     bool read_file(const string& path, vector<uint8_t>& content) override;
//     bool write_file(const string& path, const vector<uint8_t>& content) override;
//     bool delete_file(const string& path) override;
//     vector<string> list_directory(const string& path) override;
//     bool create_directory(const string& path) override;
//     bool delete_directory(const string& path) override;
//     bool exists(const string& path) override;

// private:
//     string root_path;
//     mutex_t mutex {};
// };

// struct vfs_mount_point_t {
//     string path;
//     ptr::unique<vfs_storage_backend_interface> storage;
//     string device_name;
//     string filesystem_type;

//     vfs_mount_point_t(string path,ptr::unique<vfs_storage_backend_interface> storage, string device_name, string fst)
//         : path(move(path)), storage(move(storage)), device_name(move(device_name)), filesystem_type(move(fst)) {}
// };

// class vfs {
// public:
//     vfs();

//     bool mount(const string& vfs_path, ptr::unique<vfs_storage_backend_interface> backend, const string& device, const string& type);
//     bool create_file(const string& vfs_path, const vector<uint8_t>& content);
//     bool read_file(const string& path, vector<uint8_t>& content);
//     bool create_directory(const string& path);

// private:
//     vector<string> split_path(const string& path);
//     vfs_directory* create_directories_recursive(const string& path);
//     vfs_node* resolve_path(const string& path);

//     vfs_storage_backend_interface* get_backend_for_path(const string& path);
//     string get_backend_relative_path(const string& vfs_path);

//     ptr::unique<vfs_directory> root {};
//     vector<ptr::unique<vfs_mount_point_t>> mount_points {};
//     ptr::unique<vfs_storage_backend_interface> default_storage_backend {};
//     mutex_t mutex {};
// };

#endif // __FILESYSTEMS_VFS_HPP__