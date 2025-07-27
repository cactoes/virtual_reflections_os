#include "filesystems/vfs.hpp"

void vfs_file::set_content(const vector<uint8_t>& data) {
    content = data;
    meta->size = data.length();
    position = 0;
}

vfs_node::vfs_node(const string& name, vfs_node_type_t type) {
    meta = ptr::make_unique<vfs_file_meta_t>(move(name), type);
}

vfs::vfs() {
    root = ptr::make_unique<vfs_directory>("");
    default_storage_backend = ptr::make_unique<vfs_memory_storage_backend>();
}

vector<string> vfs::split_path(const string& path) {
    vector<string> parts {};

    const char* str = path.c_str();
    const char* start = str;

    while (*start) {
        while (*start == '/')
            start++;

        if (!*start)
            break;

        const char* end = start;
        while (*end && *end != '/')
            end++;

        size_t len = end - start;
        if (len > 0) {
            char buffer[256] { 0 };
            if (len >= sizeof(buffer))
                len = sizeof(buffer) - 1;

            strncpy(buffer, start, len);
            buffer[len] = '\0';

            parts.insert_back(string(buffer));
        }

        start = end;
    }

    return parts;
}

vfs_node* vfs_directory::get_child(const string& name) {
    for (const auto& child : children)
        if (child.key == name)
            return child.value.get();

    return nullptr;
}

void vfs_directory::add_child(ptr::unique<vfs_node> node) {
    node->parent = this;
    
    key_value_t<ptr::unique<vfs_node>> kv {
        .key = node->meta->name,
        .value = move(node)
    };

    children.insert_back(move(kv));
}

vfs_directory* vfs::create_directories_recursive(const string& path) {
    if (path == "/" || path.length() == 0)
        return root.get();

    auto parts = split_path(path);
    vfs_directory* current = root.get();

    for (const auto& part : parts) {
        vfs_node* child = current->get_child(part);
        
        if (!child) {
            auto new_dir = ptr::make_unique<vfs_directory>(part);
            vfs_directory* new_dir_ptr = new_dir.get();
            current->add_child(move(new_dir));
            current = new_dir_ptr;
        } else if (auto* dir = dynamic_cast<vfs_directory*>(child)) {
            current = dir;
        } else {
            return nullptr;
        }
    }

    return current;
}

vfs_node* vfs::resolve_path(const string& path) {
    if (path == "/" || path.length() == 0)
        return root.get();

    auto parts = split_path(path);
    vfs_node* current = root.get();

    for (const auto& part : parts) {
        if (auto* dir = dynamic_cast<vfs_directory*>(current)) {
            current = dir->get_child(part);
            if (!current) {
                return nullptr;
            }
        } else {
            return nullptr;
        }
    }

    return current;
}

bool vfs::mount(const string& vfs_path, ptr::unique<vfs_storage_backend_interface> backend, const string& device, const string& type) {
    mutex_lock_guard guard(&mutex);

    if (!create_directories_recursive(vfs_path))
        return false;

    mount_points.insert_back(
        ptr::make_unique<vfs_mount_point_t>(vfs_path, move(backend), device, type));

    return true;
}

bool vfs::create_file(const string& path, vector<uint8_t>& content) {
    mutex_lock_guard guard(&mutex);

    size_t last_slash = path.find_last_of('/');
    string dir_path = (last_slash != string::k_npos) ? path.substr(0, last_slash) : "/";
    string filename = (last_slash != string::k_npos) ? path.substr(last_slash + 1) : path;

    vfs_directory* parent_dir = create_directories_recursive(dir_path);
    if (!parent_dir || parent_dir->get_child(filename))
        return false;

    auto new_file = ptr::make_unique<vfs_file>(filename);
    new_file->set_content(content);
    parent_dir->add_child(move(new_file));

    vfs_storage_backend_interface* backend = get_backend_for_path(path);
    string backend_path = get_backend_relative_path(path);
    backend->write_file(backend_path, content);
}

vfs_storage_backend_interface* vfs::get_backend_for_path(const string& path) {
    string best_match = "";
    vfs_storage_backend_interface* best_backend = default_storage_backend.get();

    for (const auto& mount : mount_points) {
        if (path.find(mount->path) == 0 && mount->path.length() > best_match.length()) {
            best_match = mount->path;
            best_backend = mount->storage.get();
        }
    }

    return best_backend;
}

string vfs::get_backend_relative_path(const string& vfs_path) {
    for (const auto& mount : mount_points) {
        if (vfs_path.find(mount->path) == 0) {
            string relative = vfs_path.substr(mount->path.length());
            return relative.length() == 0 ? "/" : relative;
        }
    }
    return vfs_path;
}

/*
#pragma warning(disable : 4996)

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <filesystem>
#include <mutex>

// Forward declarations
class VFSNode;
class VFSFile;
class VFSDirectory;
class StorageBackend;
class VirtualFileSystem;

enum class NodeType {
    FILE,
    DIRECTORY
};

struct FileMetadata {
    std::string name;
    NodeType node_type;
    size_t size;
    time_t created_time;
    time_t modified_time;
    std::string owner;
    std::string group;

    FileMetadata(const std::string& n, NodeType type)
        : name(n), node_type(type), size(0), owner("user"), group("user") {
        time_t now = time(nullptr);
        created_time = now;
        modified_time = now;
    }
};

class VFSNode {
public:
    std::unique_ptr<FileMetadata> metadata;
    VFSDirectory* parent;

    VFSNode(const std::string& name, NodeType type)
        : metadata(std::make_unique<FileMetadata>(name, type)), parent(nullptr) {}

    virtual ~VFSNode() = default;

    std::string get_path() const;
};

class VFSFile : public VFSNode {
private:
    std::vector<uint8_t> content;
    size_t position;

public:
    VFSFile(const std::string& name)
        : VFSNode(name, NodeType::FILE), position(0) {}

    size_t read(uint8_t* buffer, size_t size) {
        size_t available = content.size() - position;
        size_t to_read = std::min(size, available);

        if (to_read > 0) {
            std::copy(content.begin() + position,
                content.begin() + position + to_read,
                buffer);
            position += to_read;
        }

        return to_read;
    }

    size_t write(const uint8_t* data, size_t size) {
        // Resize if necessary
        if (position + size > content.size()) {
            content.resize(position + size);
        }

        std::copy(data, data + size, content.begin() + position);
        position += size;
        metadata->size = content.size();
        metadata->modified_time = time(nullptr);

        return size;
    }

    void seek(size_t pos) {
        position = std::min(pos, content.size());
    }

    void truncate(size_t size = 0) {
        content.resize(size);
        metadata->size = size;
        position = std::min(position, size);
        metadata->modified_time = time(nullptr);
    }

    const std::vector<uint8_t>& get_content() const {
        return content;
    }

    void set_content(const std::vector<uint8_t>& data) {
        content = data;
        metadata->size = data.size();
        position = 0;
        metadata->modified_time = time(nullptr);
    }
};

class VFSDirectory : public VFSNode {
private:
    std::map<std::string, std::unique_ptr<VFSNode>> children;

public:
    VFSDirectory(const std::string& name)
        : VFSNode(name, NodeType::DIRECTORY) {}

    void add_child(std::unique_ptr<VFSNode> node) {
        node->parent = this;
        std::string name = node->metadata->name;
        children[name] = std::move(node);
        metadata->modified_time = time(nullptr);
    }

    std::unique_ptr<VFSNode> remove_child(const std::string& name) {
        auto it = children.find(name);
        if (it != children.end()) {
            auto node = std::move(it->second);
            children.erase(it);
            node->parent = nullptr;
            metadata->modified_time = time(nullptr);
            return node;
        }
        return nullptr;
    }

    VFSNode* get_child(const std::string& name) const {
        auto it = children.find(name);
        return (it != children.end()) ? it->second.get() : nullptr;
    }

    std::vector<std::string> list_children() const {
        std::vector<std::string> names;
        for (const auto& pair : children) {
            names.push_back(pair.first);
        }
        return names;
    }

    size_t child_count() const {
        return children.size();
    }
};

// Abstract storage backend
class StorageBackend {
public:
    virtual ~StorageBackend() = default;
    virtual bool read_file(const std::string& path, std::vector<uint8_t>& content) = 0;
    virtual bool write_file(const std::string& path, const std::vector<uint8_t>& content) = 0;
    virtual bool delete_file(const std::string& path) = 0;
    virtual std::vector<std::string> list_directory(const std::string& path) = 0;
    virtual bool create_directory(const std::string& path) = 0;
    virtual bool delete_directory(const std::string& path) = 0;
    virtual bool exists(const std::string& path) = 0;
};

// Memory storage backend
class MemoryStorageBackend : public StorageBackend {
private:
    std::map<std::string, std::vector<uint8_t>> storage;
    std::mutex storage_mutex;

public:
    bool read_file(const std::string& path, std::vector<uint8_t>& content) override {
        std::lock_guard<std::mutex> lock(storage_mutex);
        auto it = storage.find(path);
        if (it != storage.end()) {
            content = it->second;
            return true;
        }
        return false;
    }

    bool write_file(const std::string& path, const std::vector<uint8_t>& content) override {
        std::lock_guard<std::mutex> lock(storage_mutex);
        storage[path] = content;
        return true;
    }

    bool delete_file(const std::string& path) override {
        std::lock_guard<std::mutex> lock(storage_mutex);
        return storage.erase(path) > 0;
    }

    std::vector<std::string> list_directory(const std::string& path) override {
        std::lock_guard<std::mutex> lock(storage_mutex);
        std::vector<std::string> files;
        std::string prefix = (path == "/") ? "/" : path + "/";

        for (const auto& pair : storage) {
            if (pair.first.find(prefix) == 0) {
                std::string relative = pair.first.substr(prefix.length());
                if (relative.find('/') == std::string::npos && !relative.empty()) {
                    files.push_back(relative);
                }
            }
        }
        return files;
    }

    bool create_directory(const std::string& path) override {
        // Memory backend doesn't need explicit directory creation
        return true;
    }

    bool delete_directory(const std::string& path) override {
        // Memory backend doesn't need explicit directory deletion
        return true;
    }

    bool exists(const std::string& path) override {
        std::lock_guard<std::mutex> lock(storage_mutex);
        return storage.find(path) != storage.end();
    }
};

// Disk storage backend
class DiskStorageBackend : public StorageBackend {
private:
    std::string root_path;
    std::mutex disk_mutex;

    std::string get_full_path(const std::string& vfs_path) const {
        return root_path + vfs_path;
    }

public:
    DiskStorageBackend(const std::string& root) : root_path(root) {
        // Ensure root directory exists
        std::filesystem::create_directories(root_path);
    }

    bool read_file(const std::string& path, std::vector<uint8_t>& content) override {
        std::lock_guard<std::mutex> lock(disk_mutex);
        std::string full_path = get_full_path(path);

        std::ifstream file(full_path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        content.resize(size);
        file.read(reinterpret_cast<char*>(content.data()), size);

        return file.good();
    }

    bool write_file(const std::string& path, const std::vector<uint8_t>& content) override {
        std::lock_guard<std::mutex> lock(disk_mutex);
        std::string full_path = get_full_path(path);

        // Create parent directories
        std::filesystem::create_directories(std::filesystem::path(full_path).parent_path());

        std::ofstream file(full_path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        file.write(reinterpret_cast<const char*>(content.data()), content.size());
        return file.good();
    }

    bool delete_file(const std::string& path) override {
        std::lock_guard<std::mutex> lock(disk_mutex);
        std::string full_path = get_full_path(path);
        return std::filesystem::remove(full_path);
    }

    std::vector<std::string> list_directory(const std::string& path) override {
        std::lock_guard<std::mutex> lock(disk_mutex);
        std::vector<std::string> files;
        std::string full_path = get_full_path(path);

        try {
            for (const auto& entry : std::filesystem::directory_iterator(full_path)) {
                files.push_back(entry.path().filename().string());
            }
        }
        catch (const std::filesystem::filesystem_error&) {
            // Directory doesn't exist or can't be read
        }

        return files;
    }

    bool create_directory(const std::string& path) override {
        std::lock_guard<std::mutex> lock(disk_mutex);
        std::string full_path = get_full_path(path);
        return std::filesystem::create_directories(full_path);
    }

    bool delete_directory(const std::string& path) override {
        std::lock_guard<std::mutex> lock(disk_mutex);
        std::string full_path = get_full_path(path);
        return std::filesystem::remove_all(full_path) > 0;
    }

    bool exists(const std::string& path) override {
        std::lock_guard<std::mutex> lock(disk_mutex);
        std::string full_path = get_full_path(path);
        return std::filesystem::exists(full_path);
    }
};

// Mount point structure
struct MountPoint {
    std::string vfs_path;
    std::unique_ptr<StorageBackend> backend;
    std::string device_name;
    std::string filesystem_type;

    MountPoint(const std::string& path, std::unique_ptr<StorageBackend> b,
        const std::string& device, const std::string& fs_type)
        : vfs_path(path), backend(std::move(b)), device_name(device), filesystem_type(fs_type) {}
};

class VirtualFileSystem {
private:
    std::unique_ptr<VFSDirectory> root;
    std::vector<std::unique_ptr<MountPoint>> mount_points;
    std::unique_ptr<StorageBackend> default_backend;
    std::mutex vfs_mutex;

    std::vector<std::string> split_path(const std::string& path) const {
        std::vector<std::string> parts;
        std::stringstream ss(path);
        std::string part;

        while (std::getline(ss, part, '/')) {
            if (!part.empty()) {
                parts.push_back(part);
            }
        }
        return parts;
    }

    VFSNode* resolve_path(const std::string& path) const {
        if (path == "/" || path.empty()) {
            return root.get();
        }

        auto parts = split_path(path);
        VFSNode* current = root.get();

        for (const auto& part : parts) {
            if (auto* dir = dynamic_cast<VFSDirectory*>(current)) {
                current = dir->get_child(part);
                if (!current) {
                    return nullptr;
                }
            }
            else {
                return nullptr;
            }
        }

        return current;
    }

    VFSDirectory* create_directories_recursive(const std::string& path) {
        if (path == "/" || path.empty()) {
            return root.get();
        }

        auto parts = split_path(path);
        VFSDirectory* current = root.get();

        for (const auto& part : parts) {
            VFSNode* child = current->get_child(part);
            if (!child) {
                auto new_dir = std::make_unique<VFSDirectory>(part);
                VFSDirectory* new_dir_ptr = new_dir.get();
                current->add_child(std::move(new_dir));
                current = new_dir_ptr;
            }
            else if (auto* dir = dynamic_cast<VFSDirectory*>(child)) {
                current = dir;
            }
            else {
                return nullptr; // Path component is a file, not a directory
            }
        }

        return current;
    }

    StorageBackend* get_backend_for_path(const std::string& path) const {
        // Find the longest matching mount point
        std::string best_match = "";
        StorageBackend* best_backend = default_backend.get();

        for (const auto& mount : mount_points) {
            if (path.find(mount->vfs_path) == 0 && mount->vfs_path.length() > best_match.length()) {
                best_match = mount->vfs_path;
                best_backend = mount->backend.get();
            }
        }

        return best_backend;
    }

    std::string get_backend_relative_path(const std::string& vfs_path) const {
        // Find the mount point and return relative path
        for (const auto& mount : mount_points) {
            if (vfs_path.find(mount->vfs_path) == 0) {
                std::string relative = vfs_path.substr(mount->vfs_path.length());
                return relative.empty() ? "/" : relative;
            }
        }
        return vfs_path;
    }

public:
    VirtualFileSystem() {
        root = std::make_unique<VFSDirectory>("");
        default_backend = std::make_unique<MemoryStorageBackend>();
    }

    // Mount a storage backend at a specific path
    bool mount(const std::string& vfs_path, std::unique_ptr<StorageBackend> backend,
        const std::string& device_name = "", const std::string& fs_type = "unknown") {
        std::lock_guard<std::mutex> lock(vfs_mutex);

        // Create mount point directory if it doesn't exist
        if (!create_directories_recursive(vfs_path)) {
            return false;
        }

        // Check if already mounted
        for (const auto& mount : mount_points) {
            if (mount->vfs_path == vfs_path) {
                return false; // Already mounted
            }
        }

        mount_points.push_back(std::make_unique<MountPoint>(
            vfs_path, std::move(backend), device_name, fs_type));

        return true;
    }

    // Unmount a storage backend
    bool unmount(const std::string& vfs_path) {
        std::lock_guard<std::mutex> lock(vfs_mutex);

        auto it = std::find_if(mount_points.begin(), mount_points.end(),
            [&vfs_path](const std::unique_ptr<MountPoint>& mount) {
                return mount->vfs_path == vfs_path;
            });

        if (it != mount_points.end()) {
            mount_points.erase(it);
            return true;
        }

        return false;
    }

    // List all mount points
    std::vector<std::string> list_mounts() {
        std::lock_guard<std::mutex> lock(vfs_mutex);
        std::vector<std::string> mounts;

        for (const auto& mount : mount_points) {
            std::string info = mount->vfs_path + " (" + mount->device_name +
                ", " + mount->filesystem_type + ")";
            mounts.push_back(info);
        }

        return mounts;
    }

    bool create_file(const std::string& path, const std::vector<uint8_t>& content = {}) {
        std::lock_guard<std::mutex> lock(vfs_mutex);

        size_t last_slash = path.find_last_of('/');
        std::string dir_path = (last_slash != std::string::npos) ? path.substr(0, last_slash) : "/";
        std::string filename = (last_slash != std::string::npos) ? path.substr(last_slash + 1) : path;

        VFSDirectory* parent_dir = create_directories_recursive(dir_path);
        if (!parent_dir || parent_dir->get_child(filename)) {
            return false;
        }

        auto new_file = std::make_unique<VFSFile>(filename);
        new_file->set_content(content);
        parent_dir->add_child(std::move(new_file));

        // Persist to backend
        StorageBackend* backend = get_backend_for_path(path);
        std::string backend_path = get_backend_relative_path(path);
        backend->write_file(backend_path, content);

        return true;
    }

    bool create_directory(const std::string& path) {
        std::lock_guard<std::mutex> lock(vfs_mutex);

        if (resolve_path(path)) {
            return false; // Already exists
        }

        VFSDirectory* created = create_directories_recursive(path);

        // Persist to backend
        StorageBackend* backend = get_backend_for_path(path);
        std::string backend_path = get_backend_relative_path(path);
        backend->create_directory(backend_path);

        return created != nullptr;
    }

    bool read_file(const std::string& path, std::vector<uint8_t>& content) {
        std::lock_guard<std::mutex> lock(vfs_mutex);

        VFSNode* node = resolve_path(path);
        if (auto* file = dynamic_cast<VFSFile*>(node)) {
            content = file->get_content();
            return true;
        }

        return false;
    }

    bool write_file(const std::string& path, const std::vector<uint8_t>& content) {
        std::lock_guard<std::mutex> lock(vfs_mutex);

        VFSNode* node = resolve_path(path);
        if (auto* file = dynamic_cast<VFSFile*>(node)) {
            file->set_content(content);

            // Persist to backend
            StorageBackend* backend = get_backend_for_path(path);
            std::string backend_path = get_backend_relative_path(path);
            backend->write_file(backend_path, content);

            return true;
        }

        return false;
    }

    bool delete_path(const std::string& path) {
        std::lock_guard<std::mutex> lock(vfs_mutex);

        VFSNode* node = resolve_path(path);
        if (!node || !node->parent) {
            return false;
        }

        std::string name = node->metadata->name;
        node->parent->remove_child(name);

        // Remove from backend
        StorageBackend* backend = get_backend_for_path(path);
        std::string backend_path = get_backend_relative_path(path);

        if (dynamic_cast<VFSFile*>(node)) {
            backend->delete_file(backend_path);
        }
        else {
            backend->delete_directory(backend_path);
        }

        return true;
    }

    std::vector<std::string> list_directory(const std::string& path = "/") {
        std::lock_guard<std::mutex> lock(vfs_mutex);

        VFSNode* node = resolve_path(path);
        if (auto* dir = dynamic_cast<VFSDirectory*>(node)) {
            return dir->list_children();
        }

        return {};
    }

    bool exists(const std::string& path) {
        std::lock_guard<std::mutex> lock(vfs_mutex);
        return resolve_path(path) != nullptr;
    }

    bool is_file(const std::string& path) {
        std::lock_guard<std::mutex> lock(vfs_mutex);
        VFSNode* node = resolve_path(path);
        return dynamic_cast<VFSFile*>(node) != nullptr;
    }

    bool is_directory(const std::string& path) {
        std::lock_guard<std::mutex> lock(vfs_mutex);
        VFSNode* node = resolve_path(path);
        return dynamic_cast<VFSDirectory*>(node) != nullptr;
    }

    FileMetadata* get_metadata(const std::string& path) {
        std::lock_guard<std::mutex> lock(vfs_mutex);
        VFSNode* node = resolve_path(path);
        return node ? node->metadata.get() : nullptr;
    }
};

// Utility functions for demonstration
std::string vector_to_string(const std::vector<uint8_t>& data) {
    return std::string(data.begin(), data.end());
}

std::vector<uint8_t> string_to_vector(const std::string& str) {
    return std::vector<uint8_t>(str.begin(), str.end());
}

// Main demonstration
int main() {
    std::cout << "=== C++ Virtual File System with Disk Mounting Demo ===\n\n";

    // Create VFS
    VirtualFileSystem vfs;

    // Create some basic structure
    std::cout << "Creating basic directory structure...\n";
    vfs.create_directory("/home");
    vfs.create_directory("/home/user");
    vfs.create_directory("/var");
    vfs.create_directory("/mnt");

    // Create some files in memory backend
    std::cout << "Creating files in memory backend...\n";
    vfs.create_file("/home/user/readme.txt", string_to_vector("Welcome to C++ VFS!"));
    vfs.create_file("/var/system.log", string_to_vector("System startup\nSystem ready"));

    // Mount a disk backend at /mnt/disk
    std::cout << "\nMounting disk backend at /mnt/disk...\n";
    auto disk_backend = std::make_unique<DiskStorageBackend>("./vfs_disk_mount");
    if (vfs.mount("/mnt/disk", std::move(disk_backend), "/dev/sda1", "ext4")) {
        std::cout << "Disk mounted successfully!\n";
    }
    else {
        std::cout << "Failed to mount disk!\n";
    }

    // Create files on the mounted disk
    std::cout << "\nCreating files on mounted disk...\n";
    vfs.create_directory("/mnt/disk/documents");
    vfs.create_file("/mnt/disk/documents/important.txt",
        string_to_vector("This file is stored on disk!"));
    vfs.create_file("/mnt/disk/config.cfg",
        string_to_vector("disk_cache=true\ncompression=enabled"));

    // Mount another memory backend at /mnt/ram
    std::cout << "\nMounting another memory backend at /mnt/ram...\n";
    auto ram_backend = std::make_unique<MemoryStorageBackend>();
    if (vfs.mount("/mnt/ram", std::move(ram_backend), "tmpfs", "tmpfs")) {
        std::cout << "RAM disk mounted successfully!\n";
    }

    // Create files on RAM disk
    vfs.create_file("/mnt/ram/temp.tmp", string_to_vector("Temporary data"));

    // List mount points
    std::cout << "\nActive mount points:\n";
    auto mounts = vfs.list_mounts();
    for (const auto& mount : mounts) {
        std::cout << "  " << mount << "\n";
    }

    // Demonstrate file operations across different backends
    std::cout << "\n=== File Operations Across Backends ===\n";

    // Read from memory backend
    std::vector<uint8_t> content;
    if (vfs.read_file("/home/user/readme.txt", content)) {
        std::cout << "Memory file content: " << vector_to_string(content) << "\n";
    }

    // Read from disk backend
    if (vfs.read_file("/mnt/disk/documents/important.txt", content)) {
        std::cout << "Disk file content: " << vector_to_string(content) << "\n";
    }

    // Read from RAM disk
    if (vfs.read_file("/mnt/ram/temp.tmp", content)) {
        std::cout << "RAM disk content: " << vector_to_string(content) << "\n";
    }

    // List directories
    std::cout << "\nRoot directory contents:\n";
    auto root_files = vfs.list_directory("/");
    for (const auto& file : root_files) {
        std::cout << "  " << file << "\n";
    }

    std::cout << "\nDisk mount contents:\n";
    auto disk_files = vfs.list_directory("/mnt/disk");
    for (const auto& file : disk_files) {
        std::cout << "  " << file << "\n";
    }

    // Demonstrate metadata
    std::cout << "\n=== File Metadata ===\n";
    auto* metadata = vfs.get_metadata("/mnt/disk/documents/important.txt");
    if (metadata) {
        std::cout << "File: " << metadata->name << "\n";
        std::cout << "Size: " << metadata->size << " bytes\n";
        std::cout << "Type: " << (metadata->node_type == NodeType::FILE ? "File" : "Directory") << "\n";
        std::cout << "Modified: " << ctime(&metadata->modified_time);
    }

    // Test file operations
    std::cout << "\n=== File System Operations ===\n";
    std::cout << "/home/user exists: " << (vfs.exists("/home/user") ? "yes" : "no") << "\n";
    std::cout << "/home/user is directory: " << (vfs.is_directory("/home/user") ? "yes" : "no") << "\n";
    std::cout << "/mnt/disk/config.cfg is file: " << (vfs.is_file("/mnt/disk/config.cfg") ? "yes" : "no") << "\n";

    // Modify file on disk
    std::cout << "\nModifying file on disk backend...\n";
    vfs.write_file("/mnt/disk/config.cfg",
        string_to_vector("disk_cache=false\ncompression=disabled\nupdated=true"));

    if (vfs.read_file("/mnt/disk/config.cfg", content)) {
        std::cout << "Updated disk file: " << vector_to_string(content) << "\n";
    }

    // Demonstrate unmounting
    std::cout << "\nUnmounting RAM disk...\n";
    if (vfs.unmount("/mnt/ram")) {
        std::cout << "RAM disk unmounted successfully!\n";
        std::cout << "Trying to access unmounted path: " <<
            (vfs.exists("/mnt/ram/temp.tmp") ? "still exists" : "no longer accessible") << "\n";
    }

    std::cout << "\nFinal mount points:\n";
    mounts = vfs.list_mounts();
    for (const auto& mount : mounts) {
        std::cout << "  " << mount << "\n";
    }

    std::cout << "\nVFS Demo completed successfully!\n";

    return 0;
}

std::string VFSNode::get_path() const {
    if (parent == nullptr) {
        return metadata->name.empty() ? "/" : "/" + metadata->name;
    }

    std::string parent_path = parent->get_path();
    if (parent_path == "/") {
        return "/" + metadata->name;
    }
    return parent_path + "/" + metadata->name;
}
*/