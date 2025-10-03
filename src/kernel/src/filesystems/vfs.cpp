#include "filesystems/vfs.hpp"

static vfs_t* global_vfs_instance = nullptr;

vfs_memory_storage_interface::vfs_memory_storage_interface() {
    mutex_init(&mutex);
}

bool vfs_memory_storage_interface::read_file(const string& path, dynamic_array<uint8_t>* content) {
    if (auto it = storage.get(path); it != storage.end()) {
        for (auto& v : *it->value)
            content->insert_back(v);

        return true;
    }

    return false;
}

bool vfs_memory_storage_interface::write_file(const string& path, dynamic_array<uint8_t>* content) {
    return write_file(path, content->get_data(), content->length());
}

bool vfs_memory_storage_interface::write_file(const string& path, uint8_t* content, size_t size) {
    mutex_lock_guard guard(&mutex);

    ptr::unique<dynamic_array<uint8_t>> target_storage_file;

    if (auto it = storage.get(path); it != storage.end())
        target_storage_file = move(it->value);
    else
        target_storage_file = ptr::make_unique<dynamic_array<uint8_t>>();

    if (target_storage_file.get() == nullptr)
        return false;

    target_storage_file->clear();

    target_storage_file->resize(size);
    for (size_t i = 0; i < size; i++)
        target_storage_file->insert_back(content[i]);

    storage[path] = move(target_storage_file);

    return true;
}

bool vfs_memory_storage_interface::create_directory(const string& path) {
    return true;
}

vfs_disk_storage_interface::vfs_disk_storage_interface(filesystem_api_t* api) {
    mutex_init(&mutex);
    this->api = api;
}

bool vfs_disk_storage_interface::read_file(const string& path, dynamic_array<uint8_t>* content) {
    void* data;
    size_t size;
    api->read(api, path.c_str(), &data, &size);
    content->assign((uint8_t*)data, size / sizeof(uint8_t));
    return true;
}

bool vfs_disk_storage_interface::write_file(const string& path, dynamic_array<uint8_t>* content) {
    return write_file(path, content->get_data(), content->length());
}

bool vfs_disk_storage_interface::write_file(const string& path, uint8_t* content, size_t size) {
    return api->write(api, path.c_str(), content, &size) == 0;
}

bool vfs_disk_storage_interface::create_directory(const string& path) {
    return true;
}

void set_global_vfs(vfs_t* vfs) {
    global_vfs_instance = vfs;
}

vfs_t* get_global_vfs() {
    return global_vfs_instance;
}

bool vfs_init(vfs_t* vfs) {
    vfs->root_storage_interface = ptr::make_unique<vfs_memory_storage_interface>();

    vfs->root_node = ptr::make_unique<vfs_node_t>();
    vfs->root_node->type = vfs_node_type_t::DIRECTORY;
    vfs->root_node->storage_interface = vfs->root_storage_interface.get();
    vfs->root_node->root_mount_point = nullptr;
    vfs->root_node->root_storage_interface = vfs->root_node.get();
    
    vfs->root_node->meta.name = "/";
    vfs->root_node->meta.flags.is_interface_root = true;
    vfs->root_node->meta.flags.is_mount_point = false;
    vfs->root_node->meta.permissions.read = true;
    vfs->root_node->meta.permissions.write = true;

    return true;
}

vfs_node_t* vfs_node_get_child(vfs_node_t* parent, const string& name) {
    for (auto& child : parent->children)
        if (child->meta.name == name)
            return child.get();

    return nullptr;
}

vfs_node_t* vfs_resolve_path(vfs_t* vfs, const string& path) {
    if (path == "/" || path.length() == 0)
        return vfs->root_node.get();

    dynamic_array<string> path_parts = str_split(path, '/');
    vfs_node_t* current_node = vfs->root_node.get();

    for (const auto& part : path_parts) {
        if (current_node->type != vfs_node_type_t::DIRECTORY)
            return nullptr;

        current_node = vfs_node_get_child(current_node, part);
        if (!current_node)
            return nullptr;
    }

    return current_node;
}

vfs_storage_interface_t* vfs_get_storage_interface(vfs_t* vfs, const string& path) {
    vfs_node_t* node = vfs_resolve_path(vfs, path);
    if (node)
        return node->root_storage_interface->storage_interface;

    if (node->storage_interface)
        return node->storage_interface;

    return nullptr;
}

vfs_node_t* vfs_create_cache_directories(vfs_t* vfs, const string& path) {
    if (path == "/" || path.length() == 0)
        return vfs->root_node.get();

    dynamic_array<string> path_parts = str_split(path, '/');
    vfs_node_t* current_node = vfs->root_node.get();

    for (const auto& part : path_parts) {
        vfs_node_t* child_node = vfs_node_get_child(current_node, part);
        if (!child_node) {
            ptr::unique<vfs_node_t> new_node = ptr::make_unique<vfs_node_t>();
            new_node->meta.name = part;
            new_node->parent = current_node;
            new_node->root_mount_point = current_node->root_mount_point;
            new_node->root_storage_interface = current_node->root_storage_interface;
            new_node->type = vfs_node_type_t::DIRECTORY;

            vfs_node_t* new_node_ptr = new_node.get();
            current_node->children.insert_back(move(new_node));
            current_node = new_node_ptr;

            continue;
        }

        if (child_node->type == vfs_node_type_t::DIRECTORY) {
            current_node = child_node;
            continue;
        }

        return nullptr;
    }

    return current_node;
}

string vfs_node_to_path(vfs_node_t* node) {
    if (!node->parent)
        return "/";

    string path = "";
    vfs_node_t* current = node;

    while (current->parent) {
        path = string("/") + current->meta.name + path;
        current = current->parent;
    }

    return path;
}

string vfs_translate_to_backend_path(vfs_t* vfs, const string& path) {
    vfs_node_t* node = vfs_resolve_path(vfs, path);
    
    if (node) {
        vfs_node_t* storage_node = node->root_storage_interface;
        string relative_path = path.substr(vfs_node_to_path(storage_node).length());
        if (relative_path.find("/") != 0)
            relative_path = string("/") + relative_path;
        return relative_path;
    }

    return path;
}

file_descriptor_t vfs_get_next_descriptor(vfs_t* vfs) {
    return vfs->fd_counter++;
}

bool vfs_create_directory(vfs_t* vfs, const string& path) {
    if (vfs_resolve_path(vfs, path))
        return true;

    vfs_node_t* created_path_in_cache = vfs_create_cache_directories(vfs, path);
    if (!created_path_in_cache)
        return false;

    vfs_storage_interface_t* storage_interface = vfs_get_storage_interface(vfs, path);
    if (!storage_interface)
        return false;

    string relative_path = vfs_translate_to_backend_path(vfs, path);
    return storage_interface->create_directory(relative_path);
}

bool vfs_create_file(vfs_t* vfs, const string& path) {
    size_t last_slash = path.find_last_of('/');
    string dir_path = (last_slash != string::k_npos) ? path.substr(0, last_slash) : "/";
    string filename = (last_slash != string::k_npos) ? path.substr(last_slash + 1) : path;

    vfs_create_directory(vfs, dir_path);
    vfs_node_t* parent_dir = vfs_resolve_path(vfs, dir_path);

    ptr::unique<vfs_node_t> new_node = ptr::make_unique<vfs_node_t>();
    new_node->meta.name = filename;
    new_node->parent = parent_dir;

    new_node->root_mount_point = parent_dir->root_mount_point;
    new_node->root_storage_interface = parent_dir->root_storage_interface;
    new_node->type = vfs_node_type_t::FILE;

    parent_dir->children.insert_back(move(new_node));

    vfs_storage_interface_t* storage_interface = vfs_get_storage_interface(vfs, path);
    string relative_path = vfs_translate_to_backend_path(vfs, path);
    
    uint8_t zero_data[1] { 0 };
    return storage_interface->write_file(relative_path, zero_data, 1);
}

file_descriptor_t vfs_open_file(vfs_t* vfs, const string& path) {
    if (!vfs_resolve_path(vfs, path))
        return FILE_DESCRIPTOR_INVALID;

    file_descriptor_t fd = vfs_get_next_descriptor(vfs);
    vfs->open_files[fd] = path;

    return fd;
}

bool vfs_close_file(vfs_t* vfs, file_descriptor_t fd) {
    return vfs->open_files.remove(fd);
}

bool vfs_read_file(vfs_t* vfs, file_descriptor_t fd, dynamic_array<uint8_t>* content) {
    auto it = vfs->open_files.get(fd);
    if (it == vfs->open_files.end())
        return false;

    vfs_storage_interface_t* storage_interface = vfs_get_storage_interface(vfs, it->value);
    string relative_path = vfs_translate_to_backend_path(vfs, it->value);
    return storage_interface->read_file(relative_path, content);
}

bool vfs_write_file(vfs_t* vfs, file_descriptor_t fd, dynamic_array<uint8_t>* content) {
    auto it = vfs->open_files.get(fd);
    if (it == vfs->open_files.end())
        return false;

    vfs_storage_interface_t* storage_interface = vfs_get_storage_interface(vfs, it->value);
    string relative_path = vfs_translate_to_backend_path(vfs, it->value);
    return storage_interface->write_file(relative_path, content);
}

bool vfs_write_file(vfs_t* vfs, file_descriptor_t fd, uint8_t* content, size_t size) {
    auto it = vfs->open_files.get(fd);
    if (it == vfs->open_files.end())
        return false;

    vfs_storage_interface_t* storage_interface = vfs_get_storage_interface(vfs, it->value);
    string relative_path = vfs_translate_to_backend_path(vfs, it->value);
    return storage_interface->write_file(relative_path, content, size);
}

bool vfs_mount(vfs_t* vfs, const string& path, ptr::unique<vfs_storage_interface_t> storage_interface) {
    for (const auto& mount : vfs->mount_points)
        if (mount->mount_point_path == path)
            return false;

    vfs_node_t* new_node = vfs_create_cache_directories(vfs, path);
    if (!new_node) // || new_node->root_storage_interface
        return false;

    ptr::unique<vfs_mount_point_t> mp = ptr::make_unique<vfs_mount_point_t>();
    mp->mount_point_path = path;
    mp->interface = move(storage_interface);
    vfs_mount_point_t* mp_pointer = mp.get();
    vfs->mount_points.insert_back(move(mp));

    new_node->storage_interface = mp_pointer->interface.get();
    new_node->root_storage_interface = new_node;
    new_node->root_mount_point = new_node;
    new_node->mount_point = mp_pointer;
    new_node->meta.flags.is_mount_point = true;
    new_node->meta.flags.is_interface_root = true;

    // TODO @since 03/10/2025 -- 03:21
    // drive enumeration

    return true;
}

bool vfs_add_file_cache(vfs_t* vfs, const string& path) {
    size_t last_slash = path.find_last_of('/');
    string dir_path = (last_slash != string::k_npos) ? path.substr(0, last_slash) : "/";
    string filename = (last_slash != string::k_npos) ? path.substr(last_slash + 1) : path;

    vfs_create_directory(vfs, dir_path);
    vfs_node_t* parent_dir = vfs_resolve_path(vfs, dir_path);

    ptr::unique<vfs_node_t> new_node = ptr::make_unique<vfs_node_t>();
    new_node->meta.name = filename;
    new_node->parent = parent_dir;
    new_node->root_mount_point = parent_dir->root_mount_point;
    new_node->root_storage_interface = parent_dir->root_storage_interface;
    new_node->type = vfs_node_type_t::FILE;
    parent_dir->children.insert_back(move(new_node));

    return true;
}

const vfs_node_meta_t* vfs_get_meta(vfs_t* vfs, file_descriptor_t fd, const string& path) {
    auto it = vfs->open_files.get(fd);
    if (it == vfs->open_files.end())
        return nullptr;

    return &vfs_resolve_path(vfs, it->value)->meta;
}