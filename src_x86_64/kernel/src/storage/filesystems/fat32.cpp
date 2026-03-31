#include "storage/filesystems/fat32.hpp"
#include "std/string.hpp"
#include "std/pointer.hpp"

// TODO @since 02/02/2026 -- 17:13
// move these string functions

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

uint32_t cluster_to_lba(const fat32_fsdata_t* fs_data, uint32_t cluster) {
    if (!fs_data)
        return false;

    return fs_data->layout.first_data_sector + (cluster - 2) * fs_data->layout.sectors_per_cluster;
}

uint32_t get_next_cluster(const fat32_fsdata_t* fs_data, uint32_t cluster) {
    if (!fs_data)
        return false;

    uint32_t offset = cluster * 4;
    uint32_t fat_sector = fs_data->layout.first_fat_sector + (offset / fs_data->layout.bytes_per_sector);
    uint32_t entry_offset = offset % fs_data->layout.bytes_per_sector;

    uint8_t* sector_data = (uint8_t*)malloc(fs_data->block_device->block_size);
    if (!block_read(fs_data->block_device.get(), fat_sector, sector_data)) {
        free(sector_data);
        return FAT32_EOC32;
    }
    
    uint32_t value = *(uint32_t*)(sector_data + entry_offset);
    free(sector_data);
    return value & 0x0FFFFFFF;
}

std::unique_ptr<uint8_t> fat32_read_from_disk(fat32_fsdata_t* fs_data, size_t cluster, bool* error) {
    if (!fs_data)
        return std::unique_ptr<uint8_t>(nullptr);

    std::unique_ptr<uint8_t> data = std::unique_ptr<uint8_t>((uint8_t*)malloc(fs_data->layout.bytes_per_sector * fs_data->layout.sectors_per_cluster));

    if (error) *error = false;

    for (uint32_t sector = 0; sector < fs_data->layout.sectors_per_cluster; sector++) {
        if (!block_read_sized(fs_data->block_device.get(), cluster_to_lba(fs_data, cluster) + sector, data.get() + (sector * fs_data->layout.bytes_per_sector), fs_data->layout.bytes_per_sector)) {
            if (error) *error = true;
            break;
        }
    }

    return data;
}

bool fat32_validate(uint8_t* buffer, size_t size) {
    if (!buffer)
        return false;

    if (size > sizeof(fat32_bpb_extended_t))
        return false;

    const fat32_bpb_extended_t* bpb = (fat32_bpb_extended_t*)buffer;

    if (*(uint16_t*)(buffer + 510) != 0xAA55)
        return false;

    if (bpb->bpb.bytes_per_sector != 512 &&
        bpb->bpb.bytes_per_sector != 1024 &&
        bpb->bpb.bytes_per_sector != 2048 &&
        bpb->bpb.bytes_per_sector != 4096)
        return false;

    uint8_t sectors_per_cluster = bpb->bpb.sectors_per_cluster;
    if (sectors_per_cluster == 0 || (sectors_per_cluster & (sectors_per_cluster - 1)))
        return false;

    if (bpb->bpb.reserved_sectors == 0)
        return false;

    if (bpb->bpb.fat_count == 0)
        return false;

    if (bpb->bpb.root_entry_count != 0)
        return false;

    if (bpb->bpb.fat_size_16 != 0)
        return false;

    if (bpb->fat_size_32 == 0)
        return false;
    
    return true;
}

bool fat32_init(std::unique_ptr<block_device_t> device, fat32_fsdata_t* fs_data) {
    if (!device || !fs_data)
        return false;

    uint8_t* buffer = (uint8_t*)malloc(device->block_size);
    if (!block_read(device.get(), 0, buffer))
        return false;

    const fat32_bpb_extended_t* bpb_extended = (fat32_bpb_extended_t*)buffer;

    fs_data->block_device = move(device);
    fs_data->layout.bytes_per_sector = bpb_extended->bpb.bytes_per_sector;
    fs_data->layout.sectors_per_cluster = bpb_extended->bpb.sectors_per_cluster;
    fs_data->layout.first_fat_sector = bpb_extended->bpb.reserved_sectors;
    fs_data->layout.sectors_per_fat = bpb_extended->fat_size_32;
    fs_data->layout.first_data_sector = bpb_extended->bpb.reserved_sectors + bpb_extended->bpb.fat_count * bpb_extended->fat_size_32;
    fs_data->layout.root_cluster = bpb_extended->root_cluster;

    free(buffer);

    return true;
}

bool fat32_find_node(fat32_fsdata_t* fs_data, const char* path, size_t size, uint32_t cluster, fat32_node_t* out_node) {
    if (!fs_data || !out_node)
        return false;

    if (path == nullptr || strlen(path) == 0 || streq(path, "/")) {
        out_node->first_cluster = cluster;
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

    while (cluster < FAT32_EOC32) {
        // read target disk cluster
        bool error;
        std::unique_ptr<uint8_t> dir_data = fat32_read_from_disk(fs_data, cluster, &error);
        if (error)
            return false;

        char long_filename[260] { 0 };
        for (uint32_t offset = 0; offset < fs_data->layout.bytes_per_sector * fs_data->layout.sectors_per_cluster; offset += 32) {
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

                for (int j = 0; j < 3 && entry->name[8 + j] != ' '; j++)
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
                    out_node->first_cluster = first_cluster;
                    out_node->size = entry->file_size;
                    out_node->is_directory = false;
                    memcpy(out_node->name, current_name, strlen(current_name));
                    return true;
                }

                if (is_directory) {
                    std::string remaining_path = "";
                    for (size_t i = 1; i < path_parts.length(); i++) {
                        remaining_path += "/";
                        remaining_path += *path_parts.get_at(i);
                    }

                    return fat32_find_node(fs_data, remaining_path.c_str(), entry->file_size, first_cluster, out_node);
                }

                return false;
            }

            long_filename[0] = 0;
        }

        cluster = get_next_cluster(fs_data, cluster);
    }

    return false;
}

bool fat32_directory_exists(fat32_fsdata_t* fs_data, const char* path) {
    fat32_node_t node {};
    if (!fat32_find_node(fs_data, path, fs_data->layout.bytes_per_sector * fs_data->layout.sectors_per_cluster, fs_data->layout.root_cluster, &node))
        return false;

    return node.is_directory;
}

bool fat32_file_exists(fat32_fsdata_t* fs_data, const char* path) {
    if (!fs_data)
        return false;

    fat32_node_t node {};
    if (!fat32_find_node(fs_data, path, fs_data->layout.bytes_per_sector * fs_data->layout.sectors_per_cluster, fs_data->layout.root_cluster, &node))
        return false;

    return !node.is_directory;
}

bool fat32_read(fat32_fsdata_t* fs_data, const char* path, uint8_t** out_data, size_t* out_size) {
    if (!fs_data || !out_data || !out_size)
        return false;

    fat32_node_t node {};
    if (!fat32_find_node(fs_data, path, fs_data->layout.bytes_per_sector * fs_data->layout.sectors_per_cluster, fs_data->layout.root_cluster, &node))
        return false;

    if (node.is_directory)
        return false;

    *out_size = node.size;
    *out_data = (uint8_t*)malloc(node.size);
    if (!*out_data)
        return false;

    uint64_t cluster = node.first_cluster;
    size_t bytes_read_total = 0;
    while (cluster < FAT32_EOC32 && bytes_read_total < *out_size) {
        bool error;
        std::unique_ptr<uint8_t> cluster_data = fat32_read_from_disk(fs_data, cluster, &error);
        if (error) {
            *out_data = nullptr;
            *out_size = 0;
            return false;
        }

        size_t bytes_to_copy = MIN(fs_data->layout.bytes_per_sector * fs_data->layout.sectors_per_cluster, *out_size - bytes_read_total);
        memcpy(*(uint8_t**)out_data + bytes_read_total, cluster_data.get(), bytes_to_copy);
        bytes_read_total += bytes_to_copy;

        cluster = get_next_cluster(fs_data, cluster);
    }

    if (bytes_read_total < *out_size)
        *out_size = bytes_read_total;

    return true;
}

bool fat32_list_directory(fat32_fsdata_t* fs_data, const char* path, std::dynamic_array<fat32_node_t>* out_nodes) {
    if (!fs_data || !path || !out_nodes)
        return false;

    fat32_node_t node {};
    if (!fat32_find_node(fs_data, path, fs_data->layout.bytes_per_sector * fs_data->layout.sectors_per_cluster, fs_data->layout.root_cluster, &node))
        return false;

    if (!node.is_directory)
        return false;

    uint64_t cluster = node.first_cluster;
    while (cluster < FAT32_EOC32) {
        // read target disk cluster
        bool error;
        std::unique_ptr<uint8_t> dir_data = fat32_read_from_disk(fs_data, cluster, &error);
        if (error)
            return false;

        char long_filename[260] { 0 };
        for (uint32_t offset = 0; offset < fs_data->layout.bytes_per_sector * fs_data->layout.sectors_per_cluster; offset += 32) {
            fat32_dir_entry_t* entry = (fat32_dir_entry_t*)(dir_data.get() + offset);

            if (entry->name[0] == 0)
                return true;

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

                for (int j = 0; j < 3 && entry->name[8 + j] != ' '; j++)
                    short_filename[name_len++] = entry->name[8 + j];

                short_filename[name_len] = '\0';
            }

            const char* current_name = long_filename[0] ? long_filename : short_filename;

            if (streq(current_name, ".") || streq(current_name, "..")) {
                long_filename[0] = 0;
                continue;
            }
            
            fat32_node_t out_node {};
            out_node.first_cluster = (entry->first_cluster_high << 16) | entry->first_cluster_low;
            out_node.size = entry->file_size;
            out_node.is_directory = (entry->attributes & FAT32_DIRATTR_DIRECTORY) != 0;

            memcpy(out_node.name, current_name, MIN(strlen(current_name), sizeof(fat32_node_t::name) - 1));
            out_nodes->insert_back(out_node);

            long_filename[0] = 0;
        }

        cluster = get_next_cluster(fs_data, cluster);
    }

    return true;
}