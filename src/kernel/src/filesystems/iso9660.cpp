#include "filesystems/iso9660.hpp"
#include "memory/heap.hpp"
#include "string.hpp"
#include "utils/vector.hpp"
#include "utils/pointer.hpp"

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

ptr::unique<uint8_t> iso9660_read_from_disk(storage_driver_interface_t* storage_interface, size_t sector_count, uint64_t lba, bool* error) {
    ptr::unique<uint8_t> data = ptr::unique<uint8_t>((uint8_t*)heap_alloc(get_global_heap(), sector_count * SECTOR_SIZE));

    *error = false;
    for (size_t i = 0; i < sector_count; i++) {
        const int read_result = storage_interface->read(lba + i, data.get() + (i * SECTOR_SIZE), SECTOR_SIZE);
        if (!read_result) {
            *error = true;
            break;
        }
    }

    return data;
}

bool iso9660_find_node(storage_driver_interface_t* storage_interface, iso9660_data_t* iso_data, uint64_t lba, uint64_t size, const char* path, iso9660_node_data_t* node_data) {
    // check for "root" path
    if (path == nullptr || strlen(path) == 0) {
        node_data->lba = lba;
        node_data->size = size;
        node_data->is_directory = true;
        return true;
    }

    // read target disk section
    const size_t num_sectors = (size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    bool error;
    ptr::unique<uint8_t> dir_data = iso9660_read_from_disk(storage_interface, num_sectors, lba, &error);
    if (error)
        return false;

    // split string to get the next node to look for
    dynamic_array<string> path_parts = str_split(path, '/');
    if (path_parts.length() == 0)
        return false;

    string& target = *path_parts.get_at(0);

    // find target section
    size_t offset = 0;
    while (offset < size) {
        iso9660_dir_record_t* record = (iso9660_dir_record_t*)(dir_data.get() + offset);

        // avoid padding
        if (record->length == 0) {
            offset = ((offset / SECTOR_SIZE) + 1) * SECTOR_SIZE;
            continue;
        }

        if (record->length + offset > size)
            break;

        char name[256] = { 0 };
        get_name(record, name, 256);
        if ((record->name_len == 1 && (record->name[0] == 0 || record->name[0] == 1))) {
            offset += record->length;
            continue;
        }

        if (target == name) {
            bool is_directory = (record->file_flags & 0x02) != 0;

            // found target node
            if (!is_directory && (path_parts.length() == 1)) {
                node_data->lba = record->extent_lba.le;
                node_data->size = record->data_length.le;
                node_data->is_directory = false;
                return true;
            }

            // recurse, if the directory was the target the function will catch it at the beginning
            if (is_directory) {
                string remaining_path = "";
                for (size_t i = 1; i < path_parts.length(); i++) {
                    remaining_path += "/";
                    remaining_path += *path_parts.get_at(i);
                }

                return iso9660_find_node(storage_interface, iso_data, record->extent_lba.le, record->data_length.le, remaining_path.c_str(), node_data);
            }

            return false;
        }

        offset += record->length;
    }

    return false;
}

iso9660_filesystem_interface_t::iso9660_filesystem_interface_t(ptr::unique<storage_driver_interface_t> storage_interface, const iso9660_data_t& data) {
    this->data = data;
    this->storage_interface = move(storage_interface);
}

bool iso9660_filesystem_interface_t::read(const char* path, void** data, size_t* size) {
    iso9660_node_data_t node_data {};
    if (!iso9660_find_node(storage_interface.get(), &this->data, this->data.root.lba, this->data.root.size, (char*)path, &node_data))
        return false;

    if (node_data.is_directory)
        return false;

    const uint64_t raw_size = align_up(node_data.size, SECTOR_SIZE);
    const ptr::unique<uint8_t> buff = ptr::unique<uint8_t>((uint8_t*)heap_alloc(get_global_heap(), raw_size));
    if (!buff.get())
        return false;

    storage_interface->read(node_data.lba, buff.get(), raw_size);

    *data = (void*)GALLOC(node_data.size);
    if (!*data)
        return false;

    *size = node_data.size;

    memcpy(*data, buff.get(), node_data.size);

    return true;
}

bool iso9660_filesystem_interface_t::write(const char* path, void* data, size_t* size) {
    return false;
}

bool iso9660_filesystem_interface_t::enumerate_directory(const char* path, dynamic_array<filesystem_node_t>* out_array) {
    iso9660_node_data_t node_data {};
    if (!iso9660_find_node(storage_interface.get(), &this->data, this->data.root.lba, this->data.root.size, (char*)path, &node_data))
        return false;

    if (!node_data.is_directory)
        return false;

    const size_t num_sectors = (node_data.size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    bool error;
    ptr::unique<uint8_t> dir_data = iso9660_read_from_disk(storage_interface.get(), num_sectors, node_data.lba, &error);
    if (error)
        return false;

    size_t offset = 0;
    while (offset < node_data.size) {
        iso9660_dir_record_t* record = (iso9660_dir_record_t*)(dir_data.get() + offset);

        // avoid padding
        if (record->length == 0) {
            offset = ((offset / SECTOR_SIZE) + 1) * SECTOR_SIZE;
            continue;
        }

        if (record->length + offset > node_data.size)
            break;

        char name[256] = { 0 };
        get_name(record, name, 256);
        if ((record->name_len == 1 && (record->name[0] == 0 || record->name[0] == 1))) {
            offset += record->length;
            continue;
        }

        bool is_directory = (record->file_flags & 0x02) != 0;

        filesystem_node_t node {
            .name = name,
            .is_directory = is_directory,
            .filesize = is_directory ? 0 : record->data_length.le,
        };

        out_array->insert_back(node);

        offset += record->length;
    }

    return true;
}

const storage_driver_interface_t* iso9660_filesystem_interface_t::get_storage_interface() const {
    return storage_interface.get();
}

int iso9660_init(storage_driver_interface_t* storage_interface, iso9660_data_t* fs_data) {
    const size_t size = SECTOR_SIZE;
    uint8_t data[size];

    if (!storage_interface->read(storage_interface->get_root_lba(), data, size))
        return 1;

    iso9660_volume_descriptor_t* desc = (iso9660_volume_descriptor_t*)data;
    if (desc->type != iso9660_volume_type_t::PRIMARY_VOLUME_DESCRIPTOR)
        return 2;

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