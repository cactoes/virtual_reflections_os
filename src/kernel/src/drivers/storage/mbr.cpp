#include "drivers/storage/mbr.hpp"

bool mbr_is_entry_valid(const mbr_entry_t* entry) {
    if (!entry)
        return false;

    if (entry->attributes != 0x00 && entry->attributes != 0x80)
        return false;
    
    if (entry->partition_type == 0)
        return false;

    if (entry->sector_count == 0)
        return false;

    if (entry->lba_start == 0)
        return false;

    return true;
}

bool is_mbr(const u8* data, size_t size) {
    if (!data)
        return false;

    if (size < 512)
        return false;

    return *(const u16*)(data + 510) == 0xAA55;
}