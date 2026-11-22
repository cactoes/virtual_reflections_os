#include "memory/generic.h"
#include "processor/amd64_multiboot.h"

const multiboot2_mmap_entry_t* mb2_get_first_entry(multiboot2_info_t* mbi) {
    for (multiboot2_tag_t* tag = (multiboot2_tag_t*)mbi->tags; tag->type != 0 && tag->size != 8; tag = (multiboot2_tag_t*)((u64)tag + align_up(tag->size, 8))) {
        if (tag->type != MBTT_MMAP)
            continue;

        const multiboot2_tag_mmap_t* mmap_tag = (multiboot2_tag_mmap_t*)tag;
        return (const multiboot2_mmap_entry_t*)mmap_tag->entries;
    }

    return nullptr;
}

const multiboot2_mmap_entry_t* mb2_get_next_entry(multiboot2_info_t* mbi, const multiboot2_mmap_entry_t* prev) {
    for (multiboot2_tag_t* tag = (multiboot2_tag_t*)mbi->tags; tag->type != 0 && tag->size != 8; tag = (multiboot2_tag_t*)((u64)tag + align_up(tag->size, 8))) {
        if (tag->type != MBTT_MMAP)
            continue;

        const multiboot2_tag_mmap_t* mmap_tag = (multiboot2_tag_mmap_t*)tag;
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

const multiboot2_tag_framebuffer_t* mb2_get_framebuffer_tag(multiboot2_info_t* mbi) {
    for (multiboot2_tag_t* tag = (multiboot2_tag_t*)mbi->tags; tag->type != 0; tag = (multiboot2_tag_t*)((u64)tag + align_up(tag->size, 8))) {
        if (tag->type != MBTT_FRAMEBUFFER)
            continue;

        return (const multiboot2_tag_framebuffer_t*)tag;
    }

    return nullptr;
}

const multiboot2_tag_cmdline_t* mb2_get_cmdline_tag(multiboot2_info_t* mbi) {
    for (multiboot2_tag_t* tag = (multiboot2_tag_t*)mbi->tags; tag->type != 0; tag = (multiboot2_tag_t*)((u64)tag + align_up(tag->size, 8))) {
        if (tag->type != MBTT_CMDLINE)
            continue;

        return (const multiboot2_tag_cmdline_t*)tag;
    }

    return nullptr;
}

const multiboot2_tag_acpi_t* mb2_get_apci_tag(multiboot2_info_t* mbi) {
    for (multiboot2_tag_t* tag = (multiboot2_tag_t*)mbi->tags; tag->type != 0; tag = (multiboot2_tag_t*)((u64)tag + align_up(tag->size, 8))) {
        if (tag->type != MBTT_ACPI_RSDP1)
            continue;

        return (const multiboot2_tag_acpi_t*)tag;
    }

    return nullptr;
}