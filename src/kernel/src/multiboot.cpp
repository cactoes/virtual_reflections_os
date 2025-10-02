#include "multiboot.hpp"

bool mb_has_valid_magic(multiboot_t* p_multiboot_struct) {
    return p_multiboot_struct->magic == MULTIBOOT_MAGIC;
}

memory_map_entry_t* mb_get_first_entry(multiboot_t* p_multiboot_struct) {
    const auto& mbi = p_multiboot_struct->info;

    if (!(mbi->flags & (uint32_t)multiboot_flags_t::MMAP))
        return nullptr;

    const auto p_memory_map_entry = (memory_map_entry_t*)(uint64_t)mbi->mmap_addr;

    if ((uint64_t)p_memory_map_entry < (uint64_t)(mbi->mmap_addr + mbi->mmap_length))
        return p_memory_map_entry;

    return nullptr;
}

memory_map_entry_t* mb_get_next_entry(multiboot_t* p_multiboot_struct, memory_map_entry_t* p_prev) {
    const auto& mbi = p_multiboot_struct->info;

    const auto p_memory_map_entry = (memory_map_entry_t*)((uint64_t)p_prev + p_prev->size + sizeof(p_prev->size));

    if ((uint64_t)p_memory_map_entry < (uint64_t)((uint64_t)mbi->mmap_addr + mbi->mmap_length))
        return p_memory_map_entry;

    return nullptr;
}