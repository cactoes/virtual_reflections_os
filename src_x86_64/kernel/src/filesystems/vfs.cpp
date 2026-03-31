// #include "filesystems/vfs.hpp"

// // #define STORAGE_INTERFACE_INVALID(x) (x) == nullptr || (uint64_t)(x) == 0xffffffff || (uint64_t)(x) == 0xffffffffffffffff || (uint64_t)(x) == 0xffffffff00000000

// static vfs_t* global_vfs_instance = nullptr;

// vfs_memory_storage_interface::vfs_memory_storage_interface() {
//     mutex_init(&mutex);
// }

// bool vfs_memory_storage_interface::read_file(const std::string& path, std::dynamic_array<uint8_t>* content) {
//     if (auto it = storage.get(path); it != storage.end()) {
//         for (auto& v : *it->value)
//             content->insert_back(v);

//         return true;
//     }

//     return false;
// }

// bool vfs_memory_storage_interface::write_file(const std::string& path, std::dynamic_array<uint8_t>* content) {
//     return write_file(path, content->get_data(), content->length());
// }

// bool vfs_memory_storage_interface::write_file(const std::string& path, uint8_t* content, size_t size) {
//     mutex_lock_guard guard(&mutex);

//     std::unique_ptr<std::dynamic_array<uint8_t>> target_storage_file;

//     if (auto it = storage.get(path); it != storage.end())
//         target_storage_file = move(it->value);
//     else
//         target_storage_file = std::make_unique<std::dynamic_array<uint8_t>>();

//     if (target_storage_file.get() == nullptr)
//         return false;

//     target_storage_file->clear();

//     target_storage_file->resize(size);
//     for (size_t i = 0; i < size; i++)
//         target_storage_file->insert_back(content[i]);

//     storage[path] = move(target_storage_file);

//     return true;
// }

// bool vfs_memory_storage_interface::create_directory(const std::string& path) {
//     return true;
// }

// vfs_disk_storage_interface::vfs_disk_storage_interface(std::unique_ptr<filesystem_interface_t> api) {
//     mutex_init(&mutex);
//     this->api = move(api);
// }

// bool vfs_disk_storage_interface::read_file(const std::string& path, std::dynamic_array<uint8_t>* content) {
//     void* data;
//     size_t size;
//     if (!api->read(path.c_str(), &data, &size))
//         return false;
//     content->assign((uint8_t*)data, size / sizeof(uint8_t));
//     return true;
// }

// bool vfs_disk_storage_interface::write_file(const std::string& path, std::dynamic_array<uint8_t>* content) {
//     return write_file(path, content->get_data(), content->length());
// }

// bool vfs_disk_storage_interface::write_file(const std::string& path, uint8_t* content, size_t size) {
//     mutex_lock_guard guard(&mutex);
//     return api->write(path.c_str(), content, &size) == 0;
// }

// bool vfs_disk_storage_interface::create_directory(const std::string& path) {
//     return true;
// }

// bool vfs_disk_storage_interface::enumerate_directory(const std::string& path, std::dynamic_array<std::unique_ptr<vfs_node_t>>* out_array) {
//     std::dynamic_array<filesystem_node_t> nodes {};
//     if (!api->enumerate_directory(path.c_str(), &nodes))
//         return false;

//     out_array->resize(nodes.length());

//     for (const auto& node : nodes) {
//         auto vfs_node = std::make_unique<vfs_node_t>();
//         vfs_node->type = node.is_directory ? vfs_node_type_t::DIRECTORY : vfs_node_type_t::FILE;
//         vfs_node->meta.name = node.name;
//         out_array->insert_back(move(vfs_node));
//     }

//     return true;
// }

// vfs_out_stream_interface::vfs_out_stream_interface(void(*writer)(const char*)) {
//     mutex_init(&mutex);
//     writer_fn = writer;
// }

// bool vfs_out_stream_interface::write_file(const std::string& path, std::dynamic_array<uint8_t>* content) {
//     return write_file(path, content->get_data(), content->length());
// }

// bool vfs_out_stream_interface::write_file(const std::string& path, uint8_t* content, size_t size) {
//     mutex_lock_guard guard(&mutex);
//     if (size == 0)
//         return false;
    
//     // make sure content is 'safe'
//     content[size] = '\0';

//     // then send to writer
//     writer_fn((const char*)content);

//     return true;
// }

// bool vfs_out_stream_interface::enumerate_directory(const std::string& path, std::dynamic_array<std::unique_ptr<vfs_node_t>>* out_array) {
//     auto node = std::make_unique<vfs_node_t>();
//     node->meta.name = "stream";
//     node->type = vfs_node_type_t::FILE;
//     out_array->insert_back(move(node));
//     return true;
// }

// void set_global_vfs(vfs_t* vfs) {
//     global_vfs_instance = vfs;
// }

// vfs_t* get_global_vfs() {
//     return global_vfs_instance;
// }

// bool vfs_init(vfs_t* vfs) {
//     vfs->root_storage_interface = std::make_unique<vfs_memory_storage_interface>();

//     vfs->root_node = std::make_unique<vfs_node_t>();
//     vfs->root_node->type = vfs_node_type_t::DIRECTORY;
//     vfs->root_node->storage_interface = vfs->root_storage_interface.get();
//     vfs->root_node->root_mount_point = nullptr;
//     vfs->root_node->mount_point = nullptr;
//     vfs->root_node->root_storage_interface = vfs->root_node.get();
    
//     vfs->root_node->meta.name = "/";
//     vfs->root_node->meta.flags.is_interface_root = true;
//     vfs->root_node->meta.flags.is_mount_point = false;
//     vfs->root_node->meta.permissions.read = true;
//     vfs->root_node->meta.permissions.write = true;

//     mutex_init(&vfs->fd_mutex);
//     mutex_init(&vfs->si_mutex);
//     mutex_init(&vfs->nt_mutex);

//     return true;
// }

// vfs_node_t* vfs_node_get_child(vfs_node_t* parent, const std::string& name) {
//     for (auto& child : parent->children)
//         if (child->meta.name == name)
//             return child.get();

//     return nullptr;
// }

// vfs_node_t* vfs_resolve_path(vfs_t* vfs, const std::string& path) {
//     if (path == "/" || path.length() == 0)
//         return vfs->root_node.get();

//     std::dynamic_array<std::string> path_parts = str_split(path, '/');
//     vfs_node_t* current_node = vfs->root_node.get();

//     for (const auto& part : path_parts) {
//         if (current_node->type != vfs_node_type_t::DIRECTORY)
//             return nullptr;

//         current_node = vfs_node_get_child(current_node, part);
//         if (!current_node)
//             return nullptr;
//     }

//     return current_node;
// }

// vfs_storage_interface_t* vfs_get_storage_interface(vfs_t* vfs, const std::string& path) {
//     mutex_lock_guard guard(&vfs->si_mutex);

//     vfs_node_t* node = vfs_resolve_path(vfs, path);
//     if (!node)
//         return nullptr;

//     if (node->storage_interface)
//         return node->storage_interface;

//     return node->root_storage_interface->storage_interface;
// }

// vfs_node_t* vfs_create_cache_directories(vfs_t* vfs, const std::string& path) {
//     if (path == "/" || path.length() == 0)
//         return vfs->root_node.get();

//     std::dynamic_array<std::string> path_parts = str_split(path, '/');
//     vfs_node_t* current_node = vfs->root_node.get();

//     for (const auto& part : path_parts) {
//         vfs_node_t* child_node = vfs_node_get_child(current_node, part);
//         if (!child_node) {
//             std::unique_ptr<vfs_node_t> new_node = std::make_unique<vfs_node_t>();
//             new_node->meta.name = part;
//             new_node->parent = current_node;
//             new_node->root_mount_point = current_node->root_mount_point;
//             new_node->root_storage_interface = current_node->root_storage_interface;
//             new_node->type = vfs_node_type_t::DIRECTORY;

//             vfs_node_t* new_node_ptr = new_node.get();
//             current_node->children.insert_back(move(new_node));
//             current_node = new_node_ptr;

//             continue;
//         }

//         if (child_node->type == vfs_node_type_t::DIRECTORY) {
//             current_node = child_node;
//             continue;
//         }

//         return nullptr;
//     }

//     return current_node;
// }

// std::string vfs_node_to_path(vfs_node_t* node) {
//     if (!node->parent)
//         return "/";

//     std::string path = "";
//     vfs_node_t* current = node;

//     while (current->parent) {
//         path = std::string("/") + current->meta.name + path;
//         current = current->parent;
//     }

//     return path;
// }

// std::string vfs_translate_to_backend_path(vfs_t* vfs, const std::string& path) {
//     vfs_node_t* node = vfs_resolve_path(vfs, path);
    
//     if (node) {
//         vfs_node_t* storage_node = node->root_storage_interface;
//         std::string relative_path = path.substr(vfs_node_to_path(storage_node).length());
//         if (relative_path.find("/") != 0)
//             relative_path = std::string("/") + relative_path;
//         return relative_path;
//     }

//     return path;
// }

// file_descriptor_t vfs_get_next_descriptor(vfs_t* vfs) {
//     return vfs->fd_counter++;
// }

// bool vfs_create_directory(vfs_t* vfs, const std::string& path) {
//     mutex_lock_guard guard(&vfs->nt_mutex);

//     if (vfs_resolve_path(vfs, path))
//         return true;

//     vfs_node_t* created_path_in_cache = vfs_create_cache_directories(vfs, path);
//     if (!created_path_in_cache)
//         return false;

//     vfs_storage_interface_t* storage_interface = vfs_get_storage_interface(vfs, path);
//     if (!storage_interface)
//         return false;

//     std::string relative_path = vfs_translate_to_backend_path(vfs, path);
//     return storage_interface->create_directory(relative_path);
// }

// bool vfs_create_file(vfs_t* vfs, const std::string& path) {
//     size_t last_slash = path.find_last_of('/');
//     std::string dir_path = (last_slash != std::string::npos) ? path.substr(0, last_slash) : "/";
//     std::string filename = (last_slash != std::string::npos) ? path.substr(last_slash + 1) : path;

//     vfs_create_directory(vfs, dir_path);
    
//     // after since else we deadlock
//     mutex_lock_guard guard(&vfs->nt_mutex);
    
//     vfs_node_t* parent_dir = vfs_resolve_path(vfs, dir_path);

//     std::unique_ptr<vfs_node_t> new_node = std::make_unique<vfs_node_t>();
//     new_node->meta.name = filename;
//     new_node->parent = parent_dir;

//     new_node->root_mount_point = parent_dir->root_mount_point;
//     new_node->root_storage_interface = parent_dir->root_storage_interface;
//     new_node->type = vfs_node_type_t::FILE;

//     parent_dir->children.insert_back(move(new_node));

//     vfs_storage_interface_t* storage_interface = vfs_get_storage_interface(vfs, path);
//     std::string relative_path = vfs_translate_to_backend_path(vfs, path);
    
//     uint8_t zero_data[1] { 0 };
//     return storage_interface->write_file(relative_path, zero_data, 1);
// }

// file_descriptor_t vfs_open_file(vfs_t* vfs, const std::string& path) {
//     mutex_lock_guard guard(&vfs->fd_mutex);

//     if (auto node = vfs_resolve_path(vfs, path); !node || node->type != vfs_node_type_t::FILE)
//         return FILE_DESCRIPTOR_INVALID;

//     file_descriptor_t fd = vfs_get_next_descriptor(vfs);
//     vfs->open_files[fd] = path;

//     return fd;
// }

// bool vfs_close_file(vfs_t* vfs, file_descriptor_t fd) {
//     mutex_lock_guard guard(&vfs->fd_mutex);
//     return vfs->open_files.remove(fd);
// }

// bool vfs_read_file(vfs_t* vfs, file_descriptor_t fd, std::dynamic_array<uint8_t>* content) {
//     mutex_lock_guard guard(&vfs->fd_mutex);

//     auto it = vfs->open_files.get(fd);
//     if (it == vfs->open_files.end())
//         return false;

//     vfs_storage_interface_t* storage_interface = vfs_get_storage_interface(vfs, it->value);
//     std::string relative_path = vfs_translate_to_backend_path(vfs, it->value);
//     return storage_interface->read_file(relative_path, content);
// }

// bool vfs_write_file(vfs_t* vfs, file_descriptor_t fd, std::dynamic_array<uint8_t>* content) {
//     mutex_lock_guard guard(&vfs->fd_mutex);
    
//     auto it = vfs->open_files.get(fd);
//     if (it == vfs->open_files.end())
//         return false;

//     vfs_storage_interface_t* storage_interface = vfs_get_storage_interface(vfs, it->value);
//     std::string relative_path = vfs_translate_to_backend_path(vfs, it->value);
//     return storage_interface->write_file(relative_path, content);
// }

// bool vfs_write_file(vfs_t* vfs, file_descriptor_t fd, uint8_t* content, size_t size) {
//     mutex_lock_guard guard(&vfs->fd_mutex);

//     auto it = vfs->open_files.get(fd);
//     if (it == vfs->open_files.end())
//         return false;

//     vfs_storage_interface_t* storage_interface = vfs_get_storage_interface(vfs, it->value);
//     std::string relative_path = vfs_translate_to_backend_path(vfs, it->value);
//     return storage_interface->write_file(relative_path, content, size);
// }

// void vfs_enumerate_and_append_child_nodes(vfs_storage_interface_t* storage_interface, const std::string& path, vfs_node_t* parent_node) {
//     std::dynamic_array<std::unique_ptr<vfs_node_t>> directories {};
//     storage_interface->enumerate_directory(path, &directories);

//     for (auto& node : directories) {
//         node->parent = parent_node;
//         node->root_mount_point = parent_node->root_mount_point;
//         node->root_storage_interface = parent_node->root_mount_point;

//         if (node->type == vfs_node_type_t::DIRECTORY) {
//             vfs_enumerate_and_append_child_nodes(storage_interface, std::string(path) + "/" + node->meta.name, node.get());
//         }

//         // its OK to move it out of the array since the destructor checks if the ptr is valid
//         // it is sketchy tho ...
//         parent_node->children.insert_back(move(node));
//     }
// }

// bool vfs_mount(vfs_t* vfs, const std::string& path, std::unique_ptr<vfs_storage_interface_t> storage_interface) {
//     mutex_lock_guard guard(&vfs->nt_mutex);

//     for (const auto& mount : vfs->mount_points)
//         if (mount->mount_point_path == path)
//             return false;

//     vfs_node_t* new_node = vfs_create_cache_directories(vfs, path);
//     if (!new_node) // || new_node->root_storage_interface
//         return false;

//     vfs_storage_interface_t* storage_interface_pointer = storage_interface.get();

//     std::unique_ptr<vfs_mount_point_t> mp = std::make_unique<vfs_mount_point_t>();
//     mp->mount_point_path = path;
//     mp->interface = move(storage_interface);
//     vfs_mount_point_t* mp_pointer = mp.get();
//     vfs->mount_points.insert_back(move(mp));

//     new_node->storage_interface = storage_interface_pointer;
//     new_node->root_storage_interface = new_node;
//     new_node->root_mount_point = new_node;
//     new_node->mount_point = mp_pointer;
//     new_node->meta.flags.is_mount_point = true;
//     new_node->meta.flags.is_interface_root = true;

//     vfs_enumerate_and_append_child_nodes(storage_interface_pointer, "", new_node);

//     return true;
// }

// bool vfs_add_file_cache(vfs_t* vfs, const std::string& path) {
//     size_t last_slash = path.find_last_of('/');
//     std::string dir_path = (last_slash != std::string::npos) ? path.substr(0, last_slash) : "/";
//     std::string filename = (last_slash != std::string::npos) ? path.substr(last_slash + 1) : path;

//     vfs_create_directory(vfs, dir_path);

//     // after to prevent deadlock
//     mutex_lock_guard guard(&vfs->nt_mutex);

//     vfs_node_t* parent_dir = vfs_resolve_path(vfs, dir_path);

//     std::unique_ptr<vfs_node_t> new_node = std::make_unique<vfs_node_t>();
//     new_node->meta.name = filename;
//     new_node->parent = parent_dir;
//     new_node->root_mount_point = parent_dir->root_mount_point;
//     new_node->root_storage_interface = parent_dir->root_storage_interface;
//     new_node->type = vfs_node_type_t::FILE;
//     parent_dir->children.insert_back(move(new_node));

//     return true;
// }

// const vfs_node_meta_t* vfs_get_meta(vfs_t* vfs, file_descriptor_t fd) {
//     mutex_lock_guard guard(&vfs->fd_mutex);

//     auto it = vfs->open_files.get(fd);
//     if (it == vfs->open_files.end())
//         return nullptr;

//     return &vfs_resolve_path(vfs, it->value)->meta;
// }

// bool vfs_list_directory(vfs_t* vfs, const std::string& path, std::dynamic_array<vfs_node_t*>* out_array) {
//     mutex_lock_guard guard(&vfs->nt_mutex);

//     vfs_node_t* root_node = vfs_resolve_path(vfs, path);
//     if (!root_node)
//         return false;

//     out_array->resize(root_node->children.length());
//     for (auto& child : root_node->children)
//         out_array->insert_back(child.get());

//     return true;
// }

// bool vfs_get_disk_info(vfs_t* vfs, const std::string& path, vfs_storage_info_t* disk_info) {
//     mutex_lock_guard guard(&vfs->nt_mutex);

//     vfs_node_t* node = vfs_resolve_path(vfs, path);
//     if (!node || !node->meta.flags.is_mount_point)
//         return false;

//     *disk_info = move(node->storage_interface->get_storage_info());
//     return true;
// }