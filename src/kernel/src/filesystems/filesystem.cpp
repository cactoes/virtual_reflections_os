#include "filesystems/filesystem.hpp"
#include "std/pointer.hpp"
#include "memory/heap.hpp"

filesystem_type_t filesystem_identify(storage_driver_interface_t* storage_interface) {
    std::unique_ptr<uint8_t> buffer = std::unique_ptr<uint8_t>((uint8_t*)heap_alloc(get_global_heap(), storage_interface->get_block_size()));
    
    // first check for iso9660
    storage_interface->read(16, buffer.get(), storage_interface->get_block_size());
    if (memeq(&buffer[1], "CD001", 5)) {
        storage_interface->set_root_lba(16);
        return filesystem_type_t::ISO9660;
    }
    
    if (!storage_interface->read(0, buffer.get(), storage_interface->get_block_size()))
        return filesystem_type_t::UNKNOWN;

    // then check mbr
    if (buffer[510] == 0x55 && buffer[511] == 0xAA) {
        // TODO @since 13/10/2025 -- 23:11
        // partitions!
        // mbr has 4 partitions mbr_partition_entry is actually an array of 4 entries / paritions
        const mbr_partition_entry_t* mbr_partition_entry = (mbr_partition_entry_t*)&buffer[MBR_PARTITION_OFFSET];
        
        // store before we override the buffer
        uint64_t lba_start = mbr_partition_entry->lba_start;
        
        // read at target
        if (!storage_interface->read(lba_start, buffer.get(), storage_interface->get_block_size()))
            return filesystem_type_t::UNKNOWN;

        // its fat32
        if (memeq(&buffer[0x52], "FAT32", 5)) {
            storage_interface->set_root_lba(lba_start);
            return filesystem_type_t::FAT32;
        }

        return filesystem_type_t::UNKNOWN;
    }

    if (memeq(&buffer[0x52], "FAT32", 5)) {
        storage_interface->set_root_lba(0);
        return filesystem_type_t::FAT32;
    }

    return filesystem_type_t::UNKNOWN;
}