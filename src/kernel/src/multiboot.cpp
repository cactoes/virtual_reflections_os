#include "multiboot.hpp"

// int mb_has_valid_magic(multiboot2_info_t* p_multiboot_struct) {
//     if (p_multiboot_struct->magic == MULTIBOOT_MAGIC)
//         return MULTIBOOT_VER1;

//     if (p_multiboot_struct->magic == MULTIBOOT2_MAGIC)
//         return MULTIBOOT_VER2;

//     return MULTIBOOT_VER_UNKOWN;
// }

multiboot1_mmap_entry_t* mb1_get_first_entry(multiboot2_info_t* p_multiboot_struct) {
    const auto& mbi = (multiboot1_info_t*)p_multiboot_struct;

    if (!(mbi->flags & (u32)multiboot_flags_t::MMAP))
        return nullptr;

    const auto p_memory_map_entry = (multiboot1_mmap_entry_t*)(u64)mbi->mmap_addr;

    if ((u64)p_memory_map_entry < (u64)(mbi->mmap_addr + mbi->mmap_length))
        return p_memory_map_entry;

    return nullptr;
}

multiboot1_mmap_entry_t* mb1_get_next_entry(multiboot2_info_t* p_multiboot_struct, multiboot1_mmap_entry_t* p_prev) {
    const auto& mbi = (multiboot1_info_t*)p_multiboot_struct;

    const auto p_memory_map_entry = (multiboot1_mmap_entry_t*)((u64)p_prev + p_prev->size + sizeof(p_prev->size));

    if ((u64)p_memory_map_entry < (u64)((u64)mbi->mmap_addr + mbi->mmap_length))
        return p_memory_map_entry;

    return nullptr;
}

const multiboot2_mmap_entry_t* mb2_get_first_entry(multiboot2_info_t* multiboot_struct) {
    const multiboot2_info_t* mbi = (multiboot2_info_t*)multiboot_struct;
    
    for (multiboot2_tag_t* tag = (multiboot2_tag_t*)mbi->tags; tag->type != 0 && tag->size != 8; tag = (multiboot2_tag_t*)((u64)tag + align_up(tag->size, 8))) {
        if (tag->type != (u32)multiboot_tag_type_t::MMAP)
            continue;

        const auto mmap_tag = (multiboot2_tag_mmap_t*)tag;
        return (const multiboot2_mmap_entry_t*)mmap_tag->entries;
    }

    return nullptr;
}

const multiboot2_mmap_entry_t* mb2_get_next_entry(multiboot2_info_t* multiboot_struct, const multiboot2_mmap_entry_t* prev) {
    const multiboot2_info_t* mbi = (multiboot2_info_t*)multiboot_struct;
    
    for (multiboot2_tag_t* tag = (multiboot2_tag_t*)mbi->tags; tag->type != 0 && tag->size != 8; tag = (multiboot2_tag_t*)((u64)tag + align_up(tag->size, 8))) {
        if (tag->type != (u32)multiboot_tag_type_t::MMAP)
            continue;

        const auto mmap_tag = (multiboot2_tag_mmap_t*)tag;
        const multiboot2_mmap_entry_t* mmap_entry = (multiboot2_mmap_entry_t*)mmap_tag->entries;

        bool is_next = false;

        while ((u64)mmap_entry < (u64)mmap_tag + mmap_tag->size) {           
            if (is_next)
                return mmap_entry;
            
            if (mmap_entry == prev)
                is_next = true;

            mmap_entry = (multiboot2_mmap_entry_t*)((u8*)mmap_entry + mmap_tag->entry_size);
        }

        return nullptr;
    }

    return nullptr;
}

multiboot2_tag_framebuffer_t* mb2_get_framebuffer(multiboot2_info_t* multiboot_struct) {
    const multiboot2_info_t* mbi = (multiboot2_info_t*)multiboot_struct;

    for (multiboot2_tag_t* tag = (multiboot2_tag_t*)mbi->tags; tag->type != 0; tag = (multiboot2_tag_t*)((u64)tag + align_up(tag->size, 8))) {
        if (tag->type != (u32)multiboot_tag_type_t::FRAMEBUFFER)
            continue;

        return (multiboot2_tag_framebuffer_t*)tag;
    }

    return nullptr;
}