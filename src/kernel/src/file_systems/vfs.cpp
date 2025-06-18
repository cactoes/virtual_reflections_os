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

void vfs_init() {
    vfs_node_t* node = (vfs_node_t*)g_heap_alloc(sizeof(vfs_node_t));
    node->name = "dev";
    node->node_type = vfs_node_type_t::DIRECTORY;
    node->parent = &vfs_root;
    node->children = vector<vfs_node_t*> {};
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
        }
    }
}

void vfs_read(const char* file, void** data, size_t* size) {
    // // "/dev/ata0/test.txt"

    // if (str_start_with(file, "/dev")) {

    //     if (str_start_with(file, "/ata0")) {
    //         // nodeptr->name == "ata0"
    //         if (nodeptr->node_type == vfs_node_type_t::BLOCK_DEVICE &&
    //             nodeptr->fs.type == fs_type_t::ISO9660) {
    //             nodeptr->fs.read(&nodeptr->fs, &nodeptr->drive, &file[9], data, size);
    //         }
    //     }

    // }

    // int next_index = strff(file + 1, '/');
    // for (VECTOR_LOOP(&vfs_root.children, node)) {
    //     if (node->value->node_type == vfs_node_type_t::DIRECTORY) {
    //         if (!str_start_with(file + 1, node->value->name))
    //             continue;

    //         vfs_read(&file[next_index], data, size);
    //     } else if (node->value->node_type == vfs_node_type_t::BLOCK_DEVICE) {
    //         node->value->fs.read(&node->value->fs, &node->value->drive, &file[next_index], data, size);
    //     }
    // }

    vfs_read_recurse(vfs_get_root(), file, data, size);
}