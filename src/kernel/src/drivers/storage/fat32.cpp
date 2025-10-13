#include "filesystems/fat32.hpp"

bool fat32_bpb_is_valid(const fat32_bpb_extended_t* bpb) {
    if (bpb->bpb.bytes_per_sector != 512 &&
        bpb->bpb.bytes_per_sector != 1024 &&
        bpb->bpb.bytes_per_sector != 2048 &&
        bpb->bpb.bytes_per_sector != 4096)
        return false;

    if (bpb->bpb.sectors_per_cluster == 0)
        return false;

    if (bpb->bpb.reserved_sectors == 0)
        return false;

    if (bpb->bpb.fat_count == 0)
        return false;

    if (bpb->fat_size_32 == 0)
        return false;

    if (!memeq((const uint8_t*)bpb->fs_type, "FAT32", 5))
        return false;

    if (bpb->boot_parition_signature != 0xAA55 && *(uint16_t*)((uint8_t*)bpb + 510) != 0xAA55)
        return false;

    return true;
}

int fat32_init(storage_driver_interface_t* storage_interface, fat32_data_t* fs_data) {
    const size_t size = FAT32_SECTOR_SIZE;
    uint8_t data[FAT32_SECTOR_SIZE];

    if (!storage_interface->read(128, data, size))
        return 1;

    const fat32_bpb_extended_t* bpb_extended = (fat32_bpb_extended_t*)data;

    if (!fat32_bpb_is_valid(bpb_extended))
        return 2;

    fs_data->bytes_per_sector = bpb_extended->bpb.bytes_per_sector;
    fs_data->sectors_per_cluster = bpb_extended->bpb.sectors_per_cluster;
    fs_data->first_fat_sector = bpb_extended->bpb.reserved_sectors;
    fs_data->sectors_per_fat = bpb_extended->fat_size_32;
    fs_data->first_data_sector = bpb_extended->bpb.reserved_sectors + bpb_extended->bpb.fat_count * bpb_extended->fat_size_32;
    fs_data->root_cluster = bpb_extended->root_cluster;

    return 0;
}

uint32_t cluster_to_lba(const fat32_data_t* fs_data, uint32_t cluster) {
    return 128 + fs_data->first_data_sector + (cluster - 2) * fs_data->sectors_per_cluster;
}

ptr::unique<uint8_t> fat32_read_cluster_from_disk(storage_driver_interface_t* storage_interface, fat32_data_t* fat32_data, size_t cluster, bool* error) {
    ptr::unique<uint8_t> data = ptr::unique<uint8_t>((uint8_t*)heap_alloc(get_global_heap(), fat32_data->bytes_per_sector * fat32_data->sectors_per_cluster));

    *error = false;
    for (uint32_t sector = 0; sector < fat32_data->sectors_per_cluster; ++sector) {
        if (!storage_interface->read(cluster_to_lba(fat32_data, cluster) + sector, data.get() + (sector * fat32_data->bytes_per_sector), fat32_data->bytes_per_sector)) {
            *error = true;
            break;
        }
    }

    return data;
}

uint32_t get_next_cluster(storage_driver_interface_t* storage_interface, const fat32_data_t* fat32_data, uint32_t cluster) {
    uint32_t offset = cluster * 4;
    uint32_t fat_sector = 128 + fat32_data->first_fat_sector + offset / FAT32_SECTOR_SIZE;
    uint32_t entry_offset = offset % FAT32_SECTOR_SIZE;

    uint8_t sector_data[FAT32_SECTOR_SIZE];
    if (storage_interface->read(fat_sector, sector_data, FAT32_SECTOR_SIZE) != 0) 
        return FAT32_EOC32;
    
    return (*(uint32_t*)(sector_data + entry_offset)) & 0x0FFFFFFF;
}

void convert_utf16_to_ascii(char* destination, const uint16_t* source, size_t count) {
    while (count-- && *source && *source != 0xFFFF) {
        *destination++ = (char)(*source++ & 0xFF);
    }
    *destination = 0;
}

char to_uppercase(char c) {
    if (c >= 'a' && c <= 'z')
        return c - ('a' - 'A');
    return c;
}

int compare_ascii_case_insensitive(const char* a, const char* b) {
    while (*a && *b) {
        char char_a = to_uppercase((unsigned char)*a++);
        char char_b = to_uppercase((unsigned char)*b++);
        if (char_a != char_b) 
            return char_a - char_b;
    }
    return *a - *b;
}

bool fat32_find_node(storage_driver_interface_t* storage_interface, fat32_data_t* fat32_data, uint64_t cluster, uint64_t size, const char* path, fat32_node_data_t* node_data) {
    // check for "root" path
    if (path == nullptr || strlen(path) == 0) {
        node_data->first_cluster = cluster;
        node_data->size = size;
        node_data->is_directory = true;
        return true;
    }

    // split string to get the next node to look for
    dynamic_array<string> path_parts = str_split(path, '/');
    if (path_parts.length() == 0)
        return false;

    string& target = *path_parts.get_at(0);

    while (cluster < FAT32_EOC32) {
        // read target disk cluster
        bool error;
        ptr::unique<uint8_t> dir_data = fat32_read_cluster_from_disk(storage_interface, fat32_data, cluster, &error);
        if (error)
            return false;

        char long_filename[260] { 0 };
        for (uint32_t offset = 0; offset < fat32_data->bytes_per_sector * fat32_data->sectors_per_cluster; offset += 32) {
            fat32_dir_entry_t* entry = (fat32_dir_entry_t*)(dir_data.get() + offset);

            if (entry->name[0] == 0)
                return false;

            if (entry->name[0] == 0xE5) {
                long_filename[0] = 0;
                continue;
            }

            if (entry->attributes == FAT32_DIRATTR_LFN) {
                fat32_lfn_entry_t* lfn_entry = (fat32_lfn_entry_t*)entry;
                int index = ((lfn_entry->order & 0x1F) - 1) * 13;
                char ascii_part[14] = {0};

                uint16_t utf16_name[13];
                memcpy(utf16_name + 0, lfn_entry->name1, sizeof(lfn_entry->name1));
                memcpy(utf16_name + 5, lfn_entry->name2, sizeof(lfn_entry->name2));
                memcpy(utf16_name + 11, lfn_entry->name3, sizeof(lfn_entry->name3));

                convert_utf16_to_ascii(ascii_part, utf16_name, 13);
                strncpy(long_filename + index, ascii_part, 13);

                continue;
            }

            char short_filename[13] { 0 };
            memcpy(short_filename, entry->name, 8);
            for (int i = 7; i >= 0 && short_filename[i] == ' '; --i)
                short_filename[i] = '\0';

            if (entry->name[8] != ' ') {
                size_t name_len = strlen(short_filename);
                short_filename[name_len++] = '.';

                for (int j = 0; j < 3 && entry->name[8 + j] != ' '; ++j)
                    short_filename[name_len++] = entry->name[8 + j];

                short_filename[name_len] = '\0';
            }

            const char* current_name = long_filename[0] ? long_filename : short_filename;

            if (streq(current_name, ".") || streq(current_name, "..")) {
                long_filename[0] = 0;
                continue;
            }

            if (compare_ascii_case_insensitive(current_name, target.c_str()) == 0) {
                uint32_t first_cluster = (entry->first_cluster_high << 16) | entry->first_cluster_low;
                bool is_directory = (entry->attributes & FAT32_DIRATTR_DIRECTORY) != 0;

                if (!is_directory && (path_parts.length() == 1)) {
                    node_data->first_cluster = first_cluster;
                    node_data->size = entry->file_size;
                    node_data->is_directory = false;
                    return true;
                }

                if (is_directory) {
                    string remaining_path = "";
                    for (size_t i = 1; i < path_parts.length(); i++) {
                        remaining_path += "/";
                        remaining_path += *path_parts.get_at(i);
                    }

                    return fat32_find_node(storage_interface, fat32_data, first_cluster, entry->file_size, remaining_path.c_str(), node_data);
                }

                return false;
            }

            long_filename[0] = 0;
        }

        cluster = get_next_cluster(storage_interface, fat32_data, cluster);
    }

    return false;
}

fat32_filesystem_interface::fat32_filesystem_interface(ptr::unique<storage_driver_interface_t> storage_interface, const fat32_data_t& data) {
    this->data = data;
    this->storage_interface = move(storage_interface);
}

bool fat32_filesystem_interface::read(const char* path, void** data, size_t* size) {
    fat32_node_data_t node {};
    if (!fat32_find_node(storage_interface.get(), &this->data, this->data.root_cluster, this->data.bytes_per_sector * this->data.sectors_per_cluster, path, &node))
        return false;

    if (node.is_directory)
        return false;

    *size = node.size;
    *data = heap_alloc(get_global_heap(), node.size);
    if (!*data)
        return false;

    uint64_t cluster = node.first_cluster;
    size_t bytes_read_total = 0;
    while (cluster < FAT32_EOC32 && bytes_read_total < *size) {
        bool error;
        ptr::unique<uint8_t> cluster_data = fat32_read_cluster_from_disk(storage_interface.get(), &this->data, cluster, &error);
        if (error) {
            *data = nullptr;
            *size = 0;
            return false;
        }

        size_t bytes_to_copy = MIN(this->data.bytes_per_sector * this->data.sectors_per_cluster, *size - bytes_read_total);
        memcpy(*(uint8_t**)data + bytes_read_total, cluster_data.get(), bytes_to_copy);
        bytes_read_total += bytes_to_copy;

        cluster = get_next_cluster(storage_interface.get(), &this->data, cluster);
    }

    if (bytes_read_total < *size)
        *size = bytes_read_total;

    return true;
}

bool fat32_filesystem_interface::write(const char* path, void* data, size_t* size) {
    // TODO @since 13/10/2025 -- 00:06
    return false;
}

bool fat32_filesystem_interface::enumerate_directory(const char* path, dynamic_array<filesystem_node_t>* out_array) {
    fat32_node_data_t node {};
    if (!fat32_find_node(storage_interface.get(), &this->data, this->data.root_cluster, this->data.bytes_per_sector * this->data.sectors_per_cluster, path, &node))
        return false;

    if (!node.is_directory)
        return false;

    uint64_t cluster = node.first_cluster;
    size_t bytes_read_total = 0;
    while (cluster < FAT32_EOC32) {
        bool error;
        ptr::unique<uint8_t> cluster_data = fat32_read_cluster_from_disk(storage_interface.get(), &this->data, cluster, &error);
        if (error)
            return false;

        char long_filename[260] { 0 };
        for (uint32_t offset = 0; offset < this->data.bytes_per_sector * this->data.sectors_per_cluster; offset += sizeof(fat32_dir_entry_t)) {
            fat32_dir_entry_t* entry = (fat32_dir_entry_t*)(cluster_data.get() + offset);

            // End of directory
            if (entry->name[0] == 0)
                return true;

            // Deleted entry
            if (entry->name[0] == 0xE5) {
                long_filename[0] = 0;
                continue;
            }

            // Long file name entry
            if (entry->attributes == FAT32_DIRATTR_LFN) {
                fat32_lfn_entry_t* lfn_entry = (fat32_lfn_entry_t*)entry;
                int index = ((lfn_entry->order & 0x1F) - 1) * 13;
                char ascii_part[14] = {0};

                uint16_t utf16_name[13];
                memcpy(utf16_name + 0, lfn_entry->name1, sizeof(lfn_entry->name1));
                memcpy(utf16_name + 5, lfn_entry->name2, sizeof(lfn_entry->name2));
                memcpy(utf16_name + 11, lfn_entry->name3, sizeof(lfn_entry->name3));

                convert_utf16_to_ascii(ascii_part, utf16_name, 13);
                strncpy(long_filename + index, ascii_part, 13);
                continue;
            }

            // Short filename fallback
            char short_filename[13] { 0 };
            memcpy(short_filename, entry->name, 8);
            for (int i = 7; i >= 0 && short_filename[i] == ' '; --i)
                short_filename[i] = '\0';

            if (entry->name[8] != ' ') {
                size_t name_len = strlen(short_filename);
                short_filename[name_len++] = '.';
                for (int j = 0; j < 3 && entry->name[8 + j] != ' '; ++j)
                    short_filename[name_len++] = entry->name[8 + j];
                short_filename[name_len] = '\0';
            }

            const char* name = long_filename[0] ? long_filename : short_filename;

            // Skip volume labels and system entries
            if (entry->attributes & FAT32_DIRATTR_VOLUME_ID) {
                long_filename[0] = 0;
                continue;
            }

            if (streq(name, ".") || streq(name, "..")) {
                long_filename[0] = 0;
                continue;
            }

            // Create node entry
            filesystem_node_t fs_node {};
            fs_node.name = name;
            fs_node.filesize = entry->file_size;
            fs_node.is_directory = (entry->attributes & FAT32_DIRATTR_DIRECTORY) != 0;

            out_array->insert_back(fs_node);

            long_filename[0] = 0;
        }

        cluster = get_next_cluster(storage_interface.get(), &this->data, cluster);
    }

    return true;
}
