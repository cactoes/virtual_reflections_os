#include "file_systems/iso9660.hpp"
#include "memory.hpp"
#include "debug.hpp"
#include "vector.hpp"
#include "string.hpp"
#include "drivers/ide_driver.hpp"

void fmt_name(const char* name, uint8_t len, char* buff) {
    if(len == 1 && (name[0] == 0 || name[0] == 1)) {
        if (name[0] == 0) {
            buff[0] = '.';
            return;
        }

        buff[0] = '.';
        buff[1] = '.';
        return;
    }

    for (int i = 0; i < len; i++) {
        if (name[i] == ';')
            break;
        
        buff[i] = name[i];
    }
}

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

void get_name(iso9660_dir_record_t* record, char* out_name, size_t out_size) {
    uint8_t* system_use = (uint8_t*)record->name + record->name_len;
    
    if (record->name_len % 2 == 0)
        system_use++;
    
    size_t system_use_len = record->length - (system_use - (uint8_t*)record);
    if (system_use_len < 0) {
        fmt_name(record->name, record->name_len, out_name);
        return;
    }
    
    const uint8_t* ptr = system_use;
    const uint8_t* end = system_use + system_use_len;
    
    while (ptr + 4 <= end) {
        const susp_entry_t* entry = (const susp_entry_t*)ptr;
        
        if (entry->length < 4 || ptr + entry->length > end)
            break;
        
        if (memeq(entry->signature, "NM", 2)) {
            size_t data_len = entry->length - 4;
            const uint8_t* name_ptr = ptr + 4;
            
            // Skip flags byte if present
            if (data_len > 0) {
                uint8_t flags = *name_ptr;
                name_ptr++;
                data_len--;
            }
            
            if (data_len >= out_size) {
                data_len = out_size - 1;
            }
            
            memcpy(out_name, name_ptr, data_len);
            out_name[data_len] = '\0';
            return;
        }
        
        if (entry->length == 0)
            break;
        ptr += entry->length;
    }

    fmt_name(record->name, record->name_len, out_name);
}

bool __recurse_loop_dir(drive_t* drive, uint64_t lba, uint64_t size, char* file_path, iso9660_node_data_t* data) {
    char* target = (char*)heap_alloc(get_global_heap(), strlen(file_path));
    memzero(target,  strlen(file_path));

    int index = strff(file_path + 1, '/');
    if (index > 0) memcpy(target, file_path + 1, index);
    else           memcpy(target, file_path + 1, strlen(file_path));

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
    while (offset < size) {
        iso9660_dir_record_t* dir_record = reinterpret_cast<iso9660_dir_record_t*>(dir_data + offset);
        if (dir_record->length == 0) {
            offset = ((offset / IDE_SECTOR_SIZE) + 1) * IDE_SECTOR_SIZE;
            continue;
        }
        
        if (dir_record->length + offset > size)
            break;

        char name[256] { 0 };
        get_name(dir_record, name, 256);

        bool is_directory = (dir_record->file_flags & 0x02) != 0;
        bool is_target = streq(name, target);

        if (is_target && !is_directory) {
            data->lba = dir_record->extent_lba.le;
            data->size = dir_record->data_length.le;
            data->is_directory = false;
            
            heap_free(get_global_heap(), dir_data);
            heap_free(get_global_heap(), target);
            return true;
        }

        if (is_target &&
            is_directory &&
            !(dir_record->name_len == 1 && (dir_record->name[0] == 0 || dir_record->name[0] == 1))) {
            uint32_t child_extent_lba = dir_record->extent_lba.le;
            uint32_t child_extent_size = dir_record->data_length.le;

            const auto r = __recurse_loop_dir(drive, child_extent_lba, child_extent_size, &file_path[strlen(target) + 1], data);
            heap_free(get_global_heap(), dir_data);
            heap_free(get_global_heap(), target);
            return r;
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
    *size = node_data.size;
    
    if (!*file_data) {
        g_heap_free(buff);
        return 3;
    }

    memcpy(*file_data, buff, *size);

    g_heap_free(buff);

    return 0;
}

int iso9660_write_file(fs_t* fs, drive_t* drive, const char* file_path, void* file_data, size_t* size) {
    return 1;
}