#include "file_systems/iso9660.hpp"
#include "memory.hpp"
#include "debug.hpp"
#include "vector.hpp"
#include "string.hpp"

void fmt_name(const char* name, uint8_t len, char* buff) {
    if(len == 1 && (name[0] == 0 || name[0] == 1)) {
        // debug_print("%s", name[0] == 0 ? "." : "..");
        if (name[0] == 0) {
            buff[0] = '.';
        } else {
            buff[0] = '.';
            buff[1] = '.';
        }
    } else {
        for (int i = 0; i < len; i++) {
            if (name[i] == ';')
                break;
            buff[i] = name[i];
        }
    }
}

// vfs_node_t ata0_device_node {};
// drive_t atapi_drive {};
// fs_t iso9660_fs {};

// void iso_test(ata_drive_t* drive, iso9660_fs_data_t* fs_data) {
//     uint8_t data[IDE_SECTOR_SIZE] {};
//     ide_atapi_read(drive, 16, data);

//     iso9660_volume_descriptor_t* desc = (iso9660_volume_descriptor_t*)data;
//     memcpy(&fs_data->pvd, desc, sizeof(iso9660_volume_descriptor_t));

//     if (desc->type != iso9660_volume_type_t::PRIMARY_VOLUME_DESCRIPTOR) {
//         debug_print("INVALID VOLUME TYPE");
//         return;
//     }

//     iso9660_volume_primary_volume_descriptor_t* pvd = (iso9660_volume_primary_volume_descriptor_t*)desc;

//     fs_data->block_size = pvd->logical_block_size.le;
//     fs_data->volume_size = pvd->volume_space_size.le * pvd->logical_block_size.le;

//     iso9660_dir_record_t* root = (iso9660_dir_record_t*)(pvd->directory_entry_root);

//     fs_data->root.is_directory = true;
//     fs_data->root.lba = root->extent_lba.le;
//     fs_data->root.size = root->data_length.le;

//     atapi_drive.read = &atapi_read;
//     atapi_drive.write = &atapi_write;
//     atapi_drive.data = fs_data;

//     ata0_device_node.drive = &atapi_drive;
//     ata0_device_node.fs = &iso9660_fs;
//     ata0_device_node.name = "ata0";
//     ata0_device_node.node_type = vfs_node_type_t::BLOCK_DEVICE;

//     void* file_data_out;
//     size_t size;
//     vfs_read("/dev/ata0/test.txt", &file_data_out, &size);

//     debug_print("file_content: ");
//     for (size_t i = 0; i < size; i++)
//         debug_print(((char*)file_data_out)[i]);
// }

int iso9660_drive_init(drive_t* drive, fs_t* fs) {
    uint8_t data[IDE_SECTOR_SIZE] {};
    size_t size = IDE_SECTOR_SIZE;
    if (drive->read(drive, 16, data, &size) != 0)
        return 1;
        
    iso9660_volume_descriptor_t* desc = (iso9660_volume_descriptor_t*)data;
    if (desc->type != iso9660_volume_type_t::PRIMARY_VOLUME_DESCRIPTOR) {
        return 3;
    }

    fs->data = g_heap_alloc(sizeof(iso9660_fs_data_t));
    if (!fs->data)
        return 2;

    iso9660_fs_data_t* fs_data = (iso9660_fs_data_t*)fs->data;
    memcpy(&fs_data->pvd, desc, sizeof(iso9660_volume_descriptor_t));
    iso9660_volume_primary_volume_descriptor_t* pvd = (iso9660_volume_primary_volume_descriptor_t*)desc;

    fs_data->block_size = pvd->logical_block_size.le;
    fs_data->volume_size = pvd->volume_space_size.le * pvd->logical_block_size.le;

    iso9660_dir_record_t* root = (iso9660_dir_record_t*)(pvd->directory_entry_root);

    fs_data->root.is_directory = true;
    fs_data->root.lba = root->extent_lba.le;
    fs_data->root.size = root->data_length.le;

    return 0;
}

int iso9660_drive_deinit(fs_t* fs) {
    if (!fs->data)
        return 1;

    g_heap_free(fs->data);
    return 0;
}

bool __recurse_loop_dir(drive_t* drive, uint64_t lba, uint64_t size, char* file_path, iso9660_node_data_t* data) {
    char* target = (char*)heap_alloc(get_global_heap(), strlen(file_path));
    memzero(target,  strlen(file_path));

    int index = strff(file_path + 1, '/');
    if (index > 0) {
        memcpy(target, file_path + 1, index);
    } else {
        memcpy(target, file_path + 1, strlen(file_path));
    }

    size_t num_sectors = (size + IDE_SECTOR_SIZE - 1) / IDE_SECTOR_SIZE;
    uint8_t* dir_data = (uint8_t*)heap_alloc(get_global_heap(), num_sectors * IDE_SECTOR_SIZE);

    for (uint32_t i = 0; i < num_sectors; i++) {
        size_t ss = IDE_SECTOR_SIZE;

        if (drive->read(drive, lba + i, dir_data + i * IDE_SECTOR_SIZE, &ss) != 0) {
            heap_free(get_global_heap(), dir_data);
            heap_free(get_global_heap(), target);
            return false;
        }
    }

    size_t offset = 0;
    while(offset < size) {
        iso9660_dir_record_t* dir_record = reinterpret_cast<iso9660_dir_record_t*>(dir_data + offset);
        if (dir_record->length == 0) {
            offset = ((offset / IDE_SECTOR_SIZE) + 1) * IDE_SECTOR_SIZE;
            continue;
        }
        
        if (dir_record->length + offset > size) {
            break;
        }

        char name[256] { 0 };
        fmt_name(dir_record->name, dir_record->name_len, name);

        bool is_directory = (dir_record->file_flags & 0x02) != 0;
        bool is_target = streq(name, target);

        if (is_target && !is_directory) {
            data->lba = dir_record->extent_lba.le;
            data->size = dir_record->data_length.le;
            data->is_directory = false;
            return true;
        }

        if (is_directory && !(dir_record->name_len == 1 && (dir_record->name[0] == 0 || dir_record->name[0] == 1)) && is_target) {
            uint32_t child_extent_lba = dir_record->extent_lba.le;
            uint32_t child_extent_size = dir_record->data_length.le;
            
            if (!__recurse_loop_dir(drive, child_extent_lba, child_extent_size, &file_path[strlen(target) + 1], data)) {
                heap_free(get_global_heap(), dir_data);
                heap_free(get_global_heap(), target);
                return true;
            }
        }

        offset += dir_record->length;
    }

    heap_free(get_global_heap(), dir_data);
    heap_free(get_global_heap(), target);

    return false;
}

int iso9660_read_file(fs_t* fs, drive_t* drive, const char* file_path, void** file_data, size_t* size) {
    iso9660_node_data_t node_data {};
    iso9660_fs_data_t* fs_data = (iso9660_fs_data_t*)fs->data;

    if (!__recurse_loop_dir(drive, fs_data->root.lba, fs_data->root.size, (char*)file_path, &node_data))
        return 1;

    uint8_t* buff = (uint8_t*)g_heap_alloc(mem_align_up(node_data.size, IDE_SECTOR_SIZE));
    if (!buff)
        return 2;

    size_t ss = IDE_SECTOR_SIZE;
    drive->read(drive, node_data.lba, buff, &ss);

    *file_data = (void*)g_heap_alloc(node_data.size);
    
    if (!*file_data) {
        g_heap_free(buff);
        return 3;
    }

    *size = node_data.size;

    memcpy(*file_data, buff, *size);

    g_heap_free(buff);

    return 0;
}

int iso9660_write_file(fs_t* fs, drive_t* drive, const char* file_path, void* file_data, size_t* size) {
    return 1;
}