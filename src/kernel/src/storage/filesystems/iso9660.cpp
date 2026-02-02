#include "storage/filesystems/iso9660.hpp"
#include "std/string.hpp"
#include "std/pointer.hpp"

bool fmt_name(const char* name, uint8_t len, char* buffer) {
    if (!name || !buffer)
        return false;

    if (len == 1 && (name[0] == 0 || name[0] == 1)) {
        if (name[0] == 0) {
            buffer[0] = '.';
            return true;
        }

        buffer[0] = '.';
        buffer[1] = '.';
        return true;
    }

    for (int i = 0; i < len; i++) {
        if (name[i] == ';')
            break;
        
        buffer[i] = name[i];
    }

    return true;
}

bool get_name(iso9660_dir_record_t* record, char* out, size_t out_size) {
    if (!record || !out)
        return false;

    uint8_t* system_use = (uint8_t*)record->name + record->name_len;

    if (record->name_len % 2 == 0)
        system_use++;

    size_t system_use_len = record->length - (system_use - (uint8_t*)record);
    if (system_use_len < 0)
        return fmt_name(record->name, record->name_len, out);

    const uint8_t* ptr = system_use;
    const uint8_t* end = system_use + system_use_len;

    while (ptr + 4 <= end) {
        const iso9660_susp_entry_t* entry = (const iso9660_susp_entry_t*)ptr;

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

            if (data_len >= out_size)
                data_len = out_size - 1;

            memcpy(out, name_ptr, data_len);
            out[data_len] = '\0';
            return true;
        }

        if (entry->length == 0)
            break;

        ptr += entry->length;
    }

    return fmt_name(record->name, record->name_len, out);
}

bool iso9660_init(block_device_t* device, iso9660_fsdata_t* fs_data) {
    if (!device || !fs_data)
        return false;

    uint8_t* buffer = (uint8_t*)malloc(device->block_size);
    if (!buffer)
        return false;

    if (!block_read(device, 16, buffer))
        return false;

    iso9660_volume_primary_volume_descriptor_t* pvd = (iso9660_volume_primary_volume_descriptor_t*)buffer;

    memcpy(&fs_data->pvd, pvd, sizeof(iso9660_volume_primary_volume_descriptor_t));
    
    iso9660_dir_record_t* root_record = (iso9660_dir_record_t*)pvd->directory_entry_root;
    
    fs_data->block_device = device;
    fs_data->volume_size = pvd->volume_space_size.le * device->block_size;
    fs_data->root_node.is_directory = true;
    fs_data->root_node.lba = root_record->extent_lba.le;
    fs_data->root_node.size = root_record->data_length.le;

    free(buffer);

    return true;
}

std::unique_ptr<uint8_t> iso9660_read_from_disk(block_device_t* device, size_t size, uint64_t lba, bool* error = nullptr) {
    if (!device)
        return std::unique_ptr<uint8_t>(nullptr);

    const uint64_t lba_count = (size + device->block_size - 1) / device->block_size;
    std::unique_ptr<uint8_t> data = std::unique_ptr<uint8_t>((uint8_t*)malloc(lba_count * device->block_size));

    if (error) *error = false;

    for (size_t i = 0; i < lba_count; i++) {
        if (!block_read(device, lba + i, data.get() + (i * device->block_size))) {
            if (error) *error = true;
            break;
        }
    }

    return data;
}

bool iso9660_find_node(iso9660_fsdata_t* fs_data, const char* path, uint64_t size, uint64_t lba, iso9660_node_t* out_node) {
    if (!fs_data || !out_node)
        return false;

    // check for "root" path
    if (path == nullptr || strlen(path) == 0 || streq(path, "/")) {
        out_node->lba = lba;
        out_node->size = size;
        out_node->is_directory = true;
        out_node->name[0] = '/';
        out_node->name[1] = '\0';
        return true;
    }

    // split string to get the next node to look for
    std::dynamic_array<std::string> path_parts = str_split(path, '/');
    if (path_parts.length() == 0)
        return false;

    std::string& target = *path_parts.get_at(0);

    // read target disk section
    bool error;
    std::unique_ptr<uint8_t> disk_data = iso9660_read_from_disk(fs_data->block_device, size, lba, &error);
    if (error)
        return false;

    size_t offset = 0;
    while (offset < size) {
        iso9660_dir_record_t* record = (iso9660_dir_record_t*)(disk_data.get() + offset);

        // avoid padding
        if (record->length == 0) {
            offset = ((offset / fs_data->block_device->block_size) + 1) * fs_data->block_device->block_size;
            continue;
        }

        if (record->length + offset > size)
            break;

        char name[256] = { 0 };
        (void)get_name(record, name, 256);
        if ((record->name_len == 1 && (record->name[0] == 0 || record->name[0] == 1))) {
            offset += record->length;
            continue;
        }

        if (target == name) {
            bool is_directory = (record->file_flags & 0x02) != 0;

            // found target node
            if (!is_directory && (path_parts.length() == 1)) {
                out_node->lba = record->extent_lba.le;
                out_node->size = record->data_length.le;
                out_node->is_directory = false;
                memcpy(out_node->name, name, 256);
                return true;
            }

            // recurse, if the directory was the target the function will catch it at the beginning
            if (is_directory) {
                std::string remaining_path = "";
                for (size_t i = 1; i < path_parts.length(); i++) {
                    remaining_path += "/";
                    remaining_path += *path_parts.get_at(i);
                }

                return iso9660_find_node(fs_data, remaining_path.c_str(), record->data_length.le, record->extent_lba.le, out_node);
            }

            return false;
        }

        offset += record->length;
    }

    return false;
}

bool iso9660_directory_exists(iso9660_fsdata_t* fs_data, const char* path) {
    iso9660_node_t node {};
    if (!iso9660_find_node(fs_data, path, fs_data->root_node.size, fs_data->root_node.lba, &node))
        return false;

    return node.is_directory;
}

bool iso9660_file_exists(iso9660_fsdata_t* fs_data, const char* path) {
    iso9660_node_t node {};
    if (!iso9660_find_node(fs_data, path, fs_data->root_node.size, fs_data->root_node.lba, &node))
        return false;

    return !node.is_directory;
}

bool iso9660_read(iso9660_fsdata_t* fs_data, const char* path, uint8_t** out_data, size_t* out_size) {
    iso9660_node_t node {};
    if (!iso9660_find_node(fs_data, path, fs_data->root_node.size, fs_data->root_node.lba, &node))
        return false;

    if (node.is_directory)
        return false;

    const uint64_t aligned_size = align_up(node.size, fs_data->block_device->block_size);
    std::unique_ptr<uint8_t> file_buffer = std::unique_ptr<uint8_t>((uint8_t*)malloc(aligned_size));
    if (!file_buffer.get())
        return false;

    if (!block_read_sized(fs_data->block_device, node.lba, file_buffer.get(), aligned_size))
        return false;

    *out_size = 0;
    *out_data = (uint8_t*)malloc(node.size);
    if (!*out_data)
        return false;

    *out_size = node.size;
    memcpy(*out_data, file_buffer.get(), node.size);
    return true;
}

bool iso9660_list_directory(iso9660_fsdata_t* fs_data, const char* path, std::dynamic_array<iso9660_node_t>* out_nodes) {
    if (!fs_data || !path || !out_nodes)
        return false;

    iso9660_node_t node {};
    if (!iso9660_find_node(fs_data, path, fs_data->root_node.size, fs_data->root_node.lba, &node))
        return false;

    if (!node.is_directory)
        return false;

    bool error;
    std::unique_ptr<uint8_t> disk_data = iso9660_read_from_disk(fs_data->block_device, node.size, node.lba, &error);
    if (error)
        return false;

    size_t offset = 0;
    while (offset < node.size) {
        iso9660_dir_record_t* record = (iso9660_dir_record_t*)(disk_data.get() + offset);

        // avoid padding
        if (record->length == 0) {
            offset = ((offset / fs_data->block_device->block_size) + 1) * fs_data->block_device->block_size;
            continue;
        }

        if (record->length + offset > node.size)
            break;

        char name[256] = { 0 };
        (void)get_name(record, name, 256);
        if ((record->name_len == 1 && (record->name[0] == 0 || record->name[0] == 1))) {
            offset += record->length;
            continue;
        }

        iso9660_node_t out_node {};
        out_node.lba = record->extent_lba.le;
        out_node.size = record->data_length.le;
        out_node.is_directory = (record->file_flags & 0x02) != 0;

        memcpy(out_node.name, name, 255);

        out_nodes->insert_back(out_node);

        offset += record->length;
    }

    return true;
}