#include "storage/controller.hpp"
#include "arch/generic.hpp"
#include "std/string.hpp"
#include "std/pointer.hpp"
#include "storage/drivers/ide.hpp"

struct iso9660_lbs_msb_32 {
    uint32_t le;
    uint32_t be;
} PACKED;

struct iso9660_lbs_msb_16 {
    uint16_t le;
    uint16_t be;
} PACKED;

enum class iso9660_volume_type_t : uint8_t {
    BOOT_RECORD = 0,
    PRIMARY_VOLUME_DESCRIPTOR,
    SUPPLEMENTARY_VOLUME_DESCRIPTOR,
    VOLUME_PARTITION_DESCRIPTOR,
    // Reserved 4-254
    VOLUME_DESCRIPTOR_SET_TERMINATOR = 255
};

struct iso9660_volume_descriptor_t {
    iso9660_volume_type_t type;
    char identifier_raw[5];
    uint8_t version;
    
    uint8_t data[2041];
} PACKED;

struct iso9660_volume_boot_record_t {
    iso9660_volume_type_t type;
    char identifier_raw[5];
    uint8_t version;
    
    char boot_system_identifier_raw[32];
    char boot_identifier_raw[32];
    uint8_t boot_system_use[1977];
} PACKED;

struct iso9660_dir_record_t {
    uint8_t length;
    uint8_t ext_attr_length;
    iso9660_lbs_msb_32 extent_lba;
    iso9660_lbs_msb_32 data_length;
    uint8_t recording_date[7];
    uint8_t file_flags;
    uint8_t file_unit_size;
    uint8_t interleave_gap_size;
    iso9660_lbs_msb_16 volume_sequence_number;
    uint8_t name_len;
    char name[];
} PACKED;

struct iso9660_volume_primary_volume_descriptor_t {
    iso9660_volume_type_t type;
    char identifier_raw[5];
    uint8_t version;
    
    uint8_t unused0; // should be 0
    char system_identifier_raw[32];
    char volume_identifier_raw[32];

    uint8_t unused1[8]; // should be 0
    iso9660_lbs_msb_32 volume_space_size;

    uint8_t unused2[32]; // should be 0
    iso9660_lbs_msb_16 volume_set_size;
    iso9660_lbs_msb_16 volume_sequence_number;
    iso9660_lbs_msb_16 logical_block_size;
    iso9660_lbs_msb_32 path_table_size;
    
    uint32_t location_path_table_lba_le; // LBA location of the path table. The path table pointed to contains only little-endian values.
    uint32_t location_path_table_optional_lba_le; // LBA location of the optional path table. The path table pointed to contains only little-endian values. Zero means that no optional path table exists. 

    uint32_t location_path_table_lba_be;
    uint32_t location_path_table_optional_lba_be;

    uint8_t directory_entry_root[34];

    char volume_set_identifier_raw[128];
    char publisher_identifier_raw[128];
    char data_preparer_identifier_raw[128];
    char application_identifier_raw[128];

    char copyright_file_identifier_raw[37];
    char abstract_file_identifier_raw[37];
    char bibliographic_file_identifier_raw[37];
    uint8_t creation_date[17];
    uint8_t modification_date[17];
    uint8_t expiration_date[17];
    uint8_t effective_date[17];
    uint8_t file_structure_version;
    
    uint8_t unused3; // should be 0
    char application_used[512];
    uint8_t reserved[653];
} PACKED;

struct iso9660_volume_descriptor_set_terminator_t {
    iso9660_volume_type_t type;
    char identifier_raw[5];
    uint8_t version;
} PACKED;

struct susp_entry_t {
    char signature[2];
    uint8_t length;
    uint8_t version;
} PACKED;

struct iso9660_node_t {
    uint64_t lba;
    uint64_t size;
    bool is_directory;
};

struct iso9660_fsdata_t {
    block_device_t* block_device;
    uint64_t volume_size;
    iso9660_node_t root_node;
    iso9660_volume_primary_volume_descriptor_t pvd;
};

bool block_read(block_device_t* device, uint64_t lba, uint8_t* buffer) {
    if (!device || !buffer)
        return false;

    switch (device->type) {
        case sc_device_type_t::IDE:
            return ide_read((ide_device_t*)device->disk_device, device->start_lba + lba, buffer, device->block_size);
        default:
            return false;
    }

    return false;
}

void fmt_name(const char* name, uint8_t len, char* buffer) {
    if (!name || !buffer)
        return;

    if (len == 1 && (name[0] == 0 || name[0] == 1)) {
        if (name[0] == 0) {
            buffer[0] = '.';
            return;
        }

        buffer[0] = '.';
        buffer[1] = '.';
        return;
    }

    for (int i = 0; i < len; i++) {
        if (name[i] == ';')
            break;
        
        buffer[i] = name[i];
    }
}

void get_name(iso9660_dir_record_t* record, char* out, size_t out_size) {
    uint8_t* system_use = (uint8_t*)record->name + record->name_len;

    if (record->name_len % 2 == 0)
        system_use++;

    size_t system_use_len = record->length - (system_use - (uint8_t*)record);
    if (system_use_len < 0) {
        fmt_name(record->name, record->name_len, out);
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

            if (data_len >= out_size)
                data_len = out_size - 1;

            memcpy(out, name_ptr, data_len);
            out[data_len] = '\0';
            return;
        }

        if (entry->length == 0)
            break;

        ptr += entry->length;
    }

    fmt_name(record->name, record->name_len, out);
}

bool iso9600_init(block_device_t* device, iso9660_fsdata_t* fs_data) {
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

    return true;
}

bool iso9660_find_node(iso9660_fsdata_t* fs_data, const char* path, uint64_t size, uint64_t lba, iso9660_node_t* out_node) {
    auto iso9660_read_from_disk = [](block_device_t* device, size_t size, uint64_t lba, bool* error = nullptr) -> std::unique_ptr<uint8_t> {
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
    };

    if (!fs_data || !out_node)
        return false;

    // check for "root" path
    if (path == nullptr || strlen(path) == 0) {
        out_node->lba = lba;
        out_node->size = size;
        out_node->is_directory = true;
        return true;
    }

    // read target disk section
    bool error;
    std::unique_ptr<uint8_t> disk_data = iso9660_read_from_disk(fs_data->block_device, size, lba, &error);
    if (error)
        return false;
    
    // split string to get the next node to look for
    std::dynamic_array<std::string> path_parts = str_split(path, '/');
    if (path_parts.length() == 0)
        return false;

    std::string& target = *path_parts.get_at(0);

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
        get_name(record, name, 256);
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

bool iso9660_read(iso9660_fsdata_t* fs_data, const char* path, uint8_t* out_data, size_t* out_size) {
    iso9660_node_t node {};
    if (!iso9660_find_node(fs_data, path, fs_data->root_node.size, fs_data->root_node.lba, &node))
        return false;

    if (node.is_directory)
        return false;

    const uint64_t formatted_size = align_up(node.size, fs_data->block_device->block_size);
    const std::unique_ptr<uint8_t> file_buffer = malloc();
}

bool testidk(const pci_device_t* ide_device) {
    std::dynamic_array<ide_device_t> devices {};
    devices.resize(4);

    if (!ide_init(ide_device, &devices))
        return false;

    ide_device_t* device = devices.get_at(0);

    uint8_t* buffer = (uint8_t*)malloc(device->logical_sector_size);
    if (!ide_read(device, 16, buffer, device->logical_sector_size))
        return false;

    // ide check
    if (!memeq(&buffer[1], "CD001", 5))
        return false;

    block_device_t block_device {};
    block_device.disk_device = device;
    block_device.type = sc_device_type_t::IDE;
    block_device.start_lba = 0;
    block_device.end_lba = device->lba_count - 1;
    block_device.block_size = device->logical_sector_size;

    memzero(buffer, device->logical_sector_size);

    if (!block_read(&block_device, 16, buffer))
        return false;

    iso9660_fsdata_t fs_data {};
    if (!iso9600_init(&block_device, &fs_data))
        return false;

    return true;
}