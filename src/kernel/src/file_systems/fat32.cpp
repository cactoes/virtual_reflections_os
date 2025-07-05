#include "file_systems/fat32.hpp"
#include "memory.hpp"
#include "debug.hpp"
#include "vector.hpp"
#include "string.hpp"
#include "drivers/ahci_driver.hpp"

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

int fat32_drive_init(drive_t* drive, fs_t* fs) {
    uint8_t data[FAT32_SECTOR_SIZE] {};
    size_t size = FAT32_SECTOR_SIZE;
    if (drive->read(drive, 128, data, &size) != 0)
        return 1;

    const fat32_bpb_extended_t* bpb_extended = (fat32_bpb_extended_t*)data;

    if (!fat32_bpb_is_valid(bpb_extended))
        return 2;

    fs->data = g_heap_alloc(sizeof(fat32_fs_data_t));
    if (!fs->data)
        return 3;

    fat32_fs_data_t* fs_data = (fat32_fs_data_t*)fs->data;
    fs_data->bytes_per_sector = bpb_extended->bpb.bytes_per_sector;
    fs_data->sectors_per_cluster = bpb_extended->bpb.sectors_per_cluster;
    fs_data->first_fat_sector = bpb_extended->bpb.reserved_sectors;
    fs_data->sectors_per_fat = bpb_extended->fat_size_32;
    fs_data->first_data_sector = bpb_extended->bpb.reserved_sectors + bpb_extended->bpb.fat_count * bpb_extended->fat_size_32;
    fs_data->root_cluster = bpb_extended->root_cluster;

    return 0;
}

int fat32_drive_deinit(fs_t* fs) {
    if (!fs->data)
        return 1;

    g_heap_free(fs->data);
    return 0;
}

uint32_t cluster_to_lba(const fat32_fs_data_t* fs_data, uint32_t cluster) {
    return 128 + fs_data->first_data_sector + (cluster - 2) * fs_data->sectors_per_cluster;
}

uint32_t get_next_cluster(drive_t* drive, const fat32_fs_data_t* fs_data, uint32_t cluster) {
    uint32_t offset = cluster * 4;
    uint32_t fat_sector = 128 + fs_data->first_fat_sector + offset / FAT32_SECTOR_SIZE;
    uint32_t entry_offset = offset % FAT32_SECTOR_SIZE;

    uint8_t sector_data[FAT32_SECTOR_SIZE];
    size_t sector_size = FAT32_SECTOR_SIZE;
    if (drive->read(drive, fat_sector, sector_data, &sector_size) != 0) 
        return FAT32_EOC32;
    
    return (*(uint32_t*)(sector_data + entry_offset)) & 0x0FFFFFFF;
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

void convert_utf16_to_ascii(char* destination, const uint16_t* source, size_t count) {
    while (count-- && *source && *source != 0xFFFF) {
        *destination++ = (char)(*source++ & 0xFF);
    }
    *destination = 0;
}

bool find_file_in_directory(drive_t* drive, const fat32_fs_data_t* fs_data, uint32_t directory_cluster, const char* target_name, fat32_node_t* output) {
    const uint32_t cluster_bytes = fs_data->bytes_per_sector * fs_data->sectors_per_cluster;
    uint8_t* buffer = (uint8_t*)g_heap_alloc(cluster_bytes);
    if (!buffer) 
        return false;

    char long_filename[260] { 0 };

    while (directory_cluster < FAT32_EOC32) {
        for (uint32_t sector = 0; sector < fs_data->sectors_per_cluster; ++sector) {
            size_t sector_size = FAT32_SECTOR_SIZE;
            if (drive->read(drive, cluster_to_lba(fs_data, directory_cluster) + sector, buffer + sector * FAT32_SECTOR_SIZE, &sector_size) != 0) {
                g_heap_free(buffer);
                return false;
            }
        }

        for (uint32_t offset = 0; offset < cluster_bytes; offset += 32) {
            fat32_dir_entry_t* entry = (fat32_dir_entry_t*)(buffer + offset);
            
            if (entry->name[0] == 0x00) {
                g_heap_free(buffer);
                return false;
            }
            
            if (entry->name[0] == 0xE5) {
                long_filename[0] = 0;
                continue;
            }

            if (entry->attributes == FAT32_DIRATTR_LFN) {
                fat32_lfn_entry_t* lfn_entry = (fat32_lfn_entry_t*)entry;
                int index = ((lfn_entry->order & 0x1F) - 1) * 13;
                char temp_name[14];

                uint16_t name_part1[5];
                memcpy(name_part1, lfn_entry->name1, sizeof(name_part1));
                convert_utf16_to_ascii(temp_name, name_part1, 5);

                uint16_t name_part2[6], name_part3[2];
                memcpy(name_part2, lfn_entry->name2, sizeof(name_part2));
                memcpy(name_part3, lfn_entry->name3, sizeof(name_part3));

                convert_utf16_to_ascii(temp_name, name_part1, 5);
                strncpy(long_filename + index, temp_name, 260);
                convert_utf16_to_ascii(temp_name, name_part2, 6);
                strncpy(long_filename + index + 5, temp_name, 260);
                convert_utf16_to_ascii(temp_name, name_part3, 2);
                strncpy(long_filename + index + 11, temp_name, 260);

                continue;
            }

            char short_filename[13] { 0 };
            memcpy(short_filename, entry->name, 8);
            
            for (int i = 7; i >= 0 && short_filename[i] == ' '; --i) 
                short_filename[i] = 0;
            
            if (entry->name[8] != ' ') {
                size_t name_length = strlen(short_filename);
                short_filename[name_length] = '.';
                memcpy(short_filename + name_length + 1, entry->name + 8, 3);
                short_filename[name_length + 4] = 0;
                
                for (int i = name_length + 3; i >= (int)name_length + 1 && short_filename[i] == ' '; --i)
                    short_filename[i] = 0;
            }

            const char* comparison_name = long_filename[0] ? long_filename : short_filename;
            if (compare_ascii_case_insensitive(comparison_name, target_name) == 0) {
                uint32_t cluster = ((uint32_t)entry->first_cluster_high << 16) | entry->first_cluster_low;
                output->first_cluster = cluster;
                output->size = entry->file_size;
                output->is_directory = (entry->attributes & FAT32_DIRATTR_DIRECTORY) != 0;
                g_heap_free(buffer);
                return true;
            }
            
            long_filename[0] = 0;
        }
        
        directory_cluster = get_next_cluster(drive, fs_data, directory_cluster);
    }
    
    g_heap_free(buffer);
    return false;
}

bool __recurse_lookup_path(drive_t* drive, const fat32_fs_data_t* fs_data, const char* path, fat32_node_t* output) {
    while (*path == '/') 
        ++path;
    
    if (*path == '\0') {
        output->first_cluster = fs_data->root_cluster;
        output->is_directory = true;
        output->size = 0;
        return true;
    }

    char component[260];
    const char* slash = strchr(path, '/');
    size_t length = slash ? (size_t)(slash - path) : strlen(path);
    
    if (length >= sizeof(component)) 
        length = sizeof(component) - 1;
    
    memcpy(component, path, length);
    component[length] = 0;

    fat32_node_t node;
    if (!find_file_in_directory(drive, fs_data, fs_data->root_cluster, component, &node)) 
        return false;
    
    if (!slash) {
        *output = node;
        return true;
    }
    
    if (!node.is_directory)
        return false;

    fat32_fs_data_t temp_fs_data = *fs_data;
    temp_fs_data.root_cluster = node.first_cluster;
    return __recurse_lookup_path(drive, &temp_fs_data, slash + 1, output);
}

size_t read_file_data(drive_t* drive, const fat32_fs_data_t* fs_data, const fat32_node_t* node, void* destination) {
    uint8_t* buffer_ptr = (uint8_t*)destination;
    uint32_t current_cluster = node->first_cluster;
    size_t bytes_remaining = node->size;
    const uint32_t cluster_bytes = fs_data->bytes_per_sector * fs_data->sectors_per_cluster;

    while (current_cluster < FAT32_EOC32 && bytes_remaining) {
        uint32_t lba = cluster_to_lba(fs_data, current_cluster);
        
        for (uint32_t sector = 0; sector < fs_data->sectors_per_cluster && bytes_remaining; ++sector) {
            size_t sector_size = fs_data->bytes_per_sector;
            if (drive->read(drive, lba + sector, buffer_ptr, &sector_size) != 0) 
                return -1;
            
            size_t bytes_to_copy = (bytes_remaining < sector_size) ? bytes_remaining : sector_size;
            buffer_ptr += bytes_to_copy;
            bytes_remaining -= bytes_to_copy;
        }
        
        current_cluster = get_next_cluster(drive, fs_data, current_cluster);
    }
    
    return node->size - bytes_remaining;
}

void* load_file_from_path(drive_t* drive, fat32_fs_data_t* fs_data, const char* path, size_t* output_size) {
    fat32_node_t node;
    if (!__recurse_lookup_path(drive, fs_data, path, &node) || node.is_directory) 
        return nullptr;

    void* memory = g_heap_alloc(node.size);
    if (!memory) 
        return nullptr;
    
    if (read_file_data(drive, fs_data, &node, memory) != (size_t)node.size) {
        g_heap_free(memory);
        return nullptr;
    }
    
    if (output_size) 
        *output_size = node.size;
    
    return memory;
}

int fat32_read_file(fs_t* fs, drive_t* drive, const char* file_path, void** file_data, size_t* size) {
    *file_data = load_file_from_path(drive, (fat32_fs_data_t*)fs->data, file_path, size);
    if (!*file_data)
        return 1;
    
    return 0;
}

int fat32_write_file(fs_t* fs, drive_t* drive, const char* file_path, void* file_data, size_t* size) {
    return 1;
}