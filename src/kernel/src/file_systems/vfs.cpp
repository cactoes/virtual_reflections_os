#include "file_systems/vfs.hpp"
#include "file_systems/iso9660.hpp"
#include "drivers/ide_driver.hpp"
#include "string.hpp"

static vfs_node_t vfs_root {
    .name = "/",
    .node_type = vfs_node_type_t::DIRECTORY,
    .parent = nullptr,
    .children = vector<vfs_node_t*>{}
};

int dev_null_read(drive_t*, uint32_t lba, void* buffer, size_t* size) {
    *size = 0;
    return 0;
}

int dev_null_write(drive_t*, uint32_t lba, void* buffer, size_t* size) {
    return 0;
}

void vfs_init() {
    vfs_node_t* node = (vfs_node_t*)g_heap_alloc(sizeof(vfs_node_t));
    node->name = "dev";
    node->node_type = vfs_node_type_t::DIRECTORY;
    node->parent = &vfs_root;
    node->children = vector<vfs_node_t*> {};

    vfs_node_t* null_node = (vfs_node_t*)g_heap_alloc(sizeof(vfs_node_t));
    null_node->name = "dev";
    null_node->node_type = vfs_node_type_t::CHAR_DEVICE;
    null_node->parent = node;
    null_node->children = vector<vfs_node_t*> {};
    null_node->drive.read = &dev_null_read;
    null_node->drive.write = &dev_null_write;
    node->children.insert_back(null_node);

    vfs_root.children.insert_back(node);
}

vfs_node_t* vfs_get_root() {
    return &vfs_root;
}

vfs_node_t* vfs_mount_dev(const char* name, drive_type_t drive_type, fs_type_t fs_type) {
    vfs_node_t* node = (vfs_node_t*)g_heap_alloc(sizeof(vfs_node_t));

    if (!node)
        return nullptr;

    node->name = name;
    node->node_type = vfs_node_type_t::BLOCK_DEVICE;

    switch (drive_type) {
        case drive_type_t::ATAPI:
        node->drive.type = drive_type_t::ATAPI;
            node->drive.read = &atapi_read;
            node->drive.write = &atapi_write;
            break;
        case drive_type_t::NONE:
        default:
            break;
    }

    switch (fs_type) {
        case fs_type_t::ISO9660:
            node->fs.type = fs_type_t::ISO9660;
            node->fs.read = &iso9660_read_file;
            node->fs.write = &iso9660_write_file;
            break;
        case fs_type_t::NONE:
        default:
            break;
    }

    vfs_node_t* dev_node = (*vfs_root.children.get_at(0));
    dev_node->children.insert_back(node);

    return node;
}

void vfs_read_recurse(vfs_node_t* start_node, const char* file, void** data, size_t* size) {
    int next_index = strff(file + 1, '/');
    for (VECTOR_LOOP(&start_node->children, node)) {
        if (node->value->node_type == vfs_node_type_t::DIRECTORY) {
            if (!str_start_with(file + 1, node->value->name))
                continue;

            vfs_read_recurse(node->value, &file[next_index + 1], data, size);
        } else if (node->value->node_type == vfs_node_type_t::BLOCK_DEVICE) {
            node->value->fs.read(&node->value->fs, &node->value->drive, &file[next_index + 1], data, size);
        } else if (node->value->node_type == vfs_node_type_t::CHAR_DEVICE) {
            node->value->drive.read(&node->value->drive, 0, *data, size);
        }
    }
}

void vfs_read(const char* file, vfs_file_t* file_ptr) {
    vfs_read_recurse(vfs_get_root(), file, &file_ptr->buffer, &file_ptr->size);
}

void vfs_close_file(vfs_file_t* file) {
    g_heap_free(file->buffer);
    file->size = 0;
    file->buffer = nullptr;
}