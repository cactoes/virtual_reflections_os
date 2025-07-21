#include "filesystems/iso9660.hpp"
#include "memory/heap.hpp"
#include "string.hpp"

int iso9660_drive_init(storage_driver_api_t* p_storage_driver, iso9660_fs_data_t* p_iso_data) {
    const size_t size = 2048;
    uint8_t data[size];

    p_iso_data->read = iso9660_fs_api_read;
    p_iso_data->write = nullptr;

    p_iso_data->p_storage_driver = p_storage_driver;
    if (p_storage_driver->read(p_storage_driver, 16, data, size))
        return 1;

    iso9660_volume_descriptor_t* desc = (iso9660_volume_descriptor_t*)data;
    if (desc->type != iso9660_volume_type_t::PRIMARY_VOLUME_DESCRIPTOR)
        return 2;

    memcpy(&p_iso_data->pvd, desc, sizeof(iso9660_volume_descriptor_t));
    iso9660_volume_primary_volume_descriptor_t* pvd = (iso9660_volume_primary_volume_descriptor_t*)desc;

    p_iso_data->block_size = pvd->logical_block_size.le;
    p_iso_data->volume_size = pvd->volume_space_size.le * pvd->logical_block_size.le;

    iso9660_dir_record_t* root = (iso9660_dir_record_t*)(pvd->directory_entry_root);

    p_iso_data->root.is_directory = true;
    p_iso_data->root.lba = root->extent_lba.le;
    p_iso_data->root.size = root->data_length.le;

    return 0;
}

void fmt_name(const char* p_name, uint8_t len, char* p_buffer) {
    if(len == 1 && (p_name[0] == 0 || p_name[0] == 1)) {
        if (p_name[0] == 0) {
            p_buffer[0] = '.';
            return;
        }

        p_buffer[0] = '.';
        p_buffer[1] = '.';
        return;
    }

    for (int i = 0; i < len; i++) {
        if (p_name[i] == ';')
            break;
        
        p_buffer[i] = p_name[i];
    }
}

void get_name(iso9660_dir_record_t* p_record, char* p_out, size_t out_size) {
    uint8_t* system_use = (uint8_t*)p_record->name + p_record->name_len;
    
    if (p_record->name_len % 2 == 0)
        system_use++;
    
    size_t system_use_len = p_record->length - (system_use - (uint8_t*)p_record);
    if (system_use_len < 0) {
        fmt_name(p_record->name, p_record->name_len, p_out);
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
            
            if (data_len > 0) {
                uint8_t flags = *name_ptr;
                name_ptr++;
                data_len--;
            }
            
            if (data_len >= out_size) {
                data_len = out_size - 1;
            }
            
            memcpy(p_out, name_ptr, data_len);
            p_out[data_len] = '\0';
            return;
        }
        
        if (entry->length == 0)
            break;
        ptr += entry->length;
    }

    fmt_name(p_record->name, p_record->name_len, p_out);
}

bool recurse_loop_dir(iso9660_fs_data_t* p_iso_data, uint64_t lba, uint64_t size, char* p_path, iso9660_node_data_t* p_node_data) {
    char* target = (char*)GALLOC(strlen(p_path) * sizeof(char));
    memzero(target,  strlen(p_path));

    int index = strff(p_path + 1, '/');
    if (index > 0) memcpy(target, p_path + 1, index);
    else           memcpy(target, p_path + 1, strlen(p_path));

    size_t num_sectors = (size + 2048 - 1) / 2048;
    uint8_t* dir_data = (uint8_t*)GALLOC(num_sectors * 2048);

    for (uint32_t i = 0; i < num_sectors; i++) {
        size_t ss = 2048;
        if (p_iso_data->p_storage_driver->read(p_iso_data->p_storage_driver, lba + i, dir_data + i * 2048, ss) != 0) {
            heap_free(get_global_heap(), dir_data);
            heap_free(get_global_heap(), target);
            return false;
        }
    }

    size_t offset = 0;
    while (offset < size) {
        iso9660_dir_record_t* dir_record = reinterpret_cast<iso9660_dir_record_t*>(dir_data + offset);
        if (dir_record->length == 0) {
            offset = ((offset / 2048) + 1) * 2048;
            continue;
        }
        
        if (dir_record->length + offset > size)
            break;

        char name[256] { 0 };
        get_name(dir_record, name, 256);

        bool is_directory = (dir_record->file_flags & 0x02) != 0;
        bool is_target = streq(name, target);

        if (is_target && !is_directory) {
            p_node_data->lba = dir_record->extent_lba.le;
            p_node_data->size = dir_record->data_length.le;
            p_node_data->is_directory = false;
            
            GFREE(dir_data);
            GFREE(target);
            return true;
        }

        if (is_target && is_directory &&
            !(dir_record->name_len == 1 && (dir_record->name[0] == 0 || dir_record->name[0] == 1))) {
            uint32_t child_extent_lba = dir_record->extent_lba.le;
            uint32_t child_extent_size = dir_record->data_length.le;

            const auto result = recurse_loop_dir(p_iso_data, child_extent_lba, child_extent_size, &p_path[strlen(target) + 1], p_node_data);
            GFREE(dir_data);
            GFREE(target);
            return result;
        }

        offset += dir_record->length;
    }

    GFREE(dir_data);
    GFREE(target);
    return false;
}

int iso9660_read_file(iso9660_fs_data_t* p_iso_data, const char* p_path, void** p_data, size_t* p_size) {
    iso9660_node_data_t node_data {};
    if (!recurse_loop_dir(p_iso_data, p_iso_data->root.lba, p_iso_data->root.size, (char*)p_path, &node_data))
        return 1;

    const uint64_t raw_size = align_up(node_data.size, 2048);
    uint8_t* buff = (uint8_t*)GALLOC(raw_size);
    if (!buff)
        return 2;

    p_iso_data->p_storage_driver->read(p_iso_data->p_storage_driver, node_data.lba, buff, raw_size);

    *p_data = (void*)GALLOC(node_data.size);
    *p_size = node_data.size;
    
    if (!*p_data) {
        GFREE(buff);
        return 3;
    }

    memcpy(*p_data, buff, node_data.size);
    GFREE(buff);

    return 0;
}