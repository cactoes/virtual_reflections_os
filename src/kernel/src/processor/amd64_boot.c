#include "libtype.h"
#include "libdef.h"
#include "processor/amd64_linker.h"
#include "kbootinfo.h"
#include "processor/amd64_multiboot.h"
#include "libstring.h"

// in main.c
void virtual_kernel_entry(kboot_info_t kboot_info);

static
__section_text
void __memzero(void* dst, u64 size) {
    __asm__ volatile (
        "rep stosb"
        : "+D"(dst), "+c"(size)
        : "a"(0)
        : "memory"
    );
}

static
__section_text
void __reload_page_table() {
    u64 cr3;
    __asm__ volatile (
        "mov %%cr3, %0\n\t"
        "mov %0, %%cr3"
        : "=&r"(cr3)
        :
        : "memory"
    );
}

static
__section_text
bool parse_memory(multiboot2_info_t* multiboot2, kboot_info_t* kboot_info) {
    u64 memory_region_ptr = 0;

    for (const multiboot2_mmap_entry_t* mm_entry = mb2_get_first_entry(multiboot2); mm_entry; mm_entry = mb2_get_next_entry(multiboot2, mm_entry)) {
        if (memory_region_ptr >= MAX_MEMORY_REGIONS)
            return false;

        memory_type_t type;
        switch (mm_entry->type) {
            case MMT_USABLE: type = MEMORY_USABLE; break;
            case MMT_ACPI_RECLAIMABLE: type = MEMORY_RECLAIMABLE; break;
            default: type = MEMORY_RESERVED; break;
        }

        memory_region_t* region = &kboot_info->memory.regions[memory_region_ptr++];
        region->addr = (mm_entry->addr & ~MEMORY_TYPE_MASK) | type;
        region->size = mm_entry->len;
        kboot_info->memory.region_count++;

        if (type == MEMORY_USABLE)
            kboot_info->memory.size += mm_entry->len;
    }

    return true;
}

static
__section_text
framebuffer_format_t get_framebuffer_format(const multiboot2_tag_framebuffer_t* tag) {
    // we only support 32bpp & rgb

    if (tag->framebuffer_type != MULTIBOOT2_FRAMEBUFFER_TYPE_RGB)
        return FB_FORMAT_UNKNOWN;

    if (tag->framebuffer_bpp != MULTIBOOT2_FRAMEBUFFER_BPP_32BIT)
        return FB_FORMAT_UNKNOWN;

    if (tag->rgb.framebuffer_red_mask_size != 8 ||
        tag->rgb.framebuffer_green_mask_size != 8 ||
        tag->rgb.framebuffer_blue_mask_size != 8)
        return FB_FORMAT_UNKNOWN;

    if (tag->rgb.framebuffer_red_field_position == 16 &&
        tag->rgb.framebuffer_green_field_position == 8 &&
        tag->rgb.framebuffer_blue_field_position == 0)
        return FB_FORMAT_ARGB8888;

    if (tag->rgb.framebuffer_red_field_position == 24 &&
        tag->rgb.framebuffer_green_field_position == 16 &&
        tag->rgb.framebuffer_blue_field_position == 8)
        return FB_FORMAT_RGBA8888;

    if (tag->rgb.framebuffer_red_field_position == 8 &&
        tag->rgb.framebuffer_green_field_position == 16 &&
        tag->rgb.framebuffer_blue_field_position == 24)
        return FB_FORMAT_BGRA8888;

    if (tag->rgb.framebuffer_red_field_position == 0 &&
        tag->rgb.framebuffer_green_field_position == 8 &&
        tag->rgb.framebuffer_blue_field_position == 16)
        return FB_FORMAT_ABGR8888;

    return FB_FORMAT_UNKNOWN;
}

static
__section_text
bool parse_framebuffer(multiboot2_info_t* multiboot2, kboot_info_t* kboot_info) {
    const multiboot2_tag_framebuffer_t* tag = mb2_get_framebuffer_tag(multiboot2);
    if (!tag)
        return false;

    framebuffer_format_t format = get_framebuffer_format(tag);
    if (format == FB_FORMAT_UNKNOWN)
        return false;

    kboot_info->framebuffer.physical_address = tag->framebuffer_addr;
    kboot_info->framebuffer.width = tag->framebuffer_width;
    kboot_info->framebuffer.height = tag->framebuffer_height;
    kboot_info->framebuffer.pitch = tag->framebuffer_pitch;
    kboot_info->framebuffer.format = format;

    return true;
}

static
__section_text
bool parse_boot_uuid(multiboot2_info_t* multiboot2, kboot_info_t* kboot_info) {
    static const char root_text[] = "root=";
    const u64 root_len = sizeof(root_text) - 1;

    const multiboot2_tag_cmdline_t* tag = mb2_get_cmdline_tag(multiboot2);
    if (!tag)
        return false;

    const char* cmdline = tag->string;
    u64 len = strlen(cmdline);

    for (u64 i = 0; i + root_len < len; i++) {
        if (i != 0 && cmdline[i - 1] != ' ')
            continue;

        u64 k = 0;
        while (k < root_len && cmdline[i + k] == root_text[k])
            k++;
        if (k != root_len)
            continue;

        const char* value = &cmdline[i + root_len];

        u64 n = 0;
        while (n < BOOT_UUID_MAX && value[n] != '\0' && value[n] != ' ') {
            kboot_info->boot_uuid[n] = value[n];
            n++;
        }

        kboot_info->boot_uuid[n] = '\0';
        return n > 0;
    }

    return true;
}

__noreturn __section_text
void amd64_entry(multiboot2_info_t* multiboot2) {
    __memzero((void*)LNK_BSS_START, LNK_BSS_SIZE);

    ((u64*)KERNEL_PHYS_TO_VIRT(page_table_l4))[0] = 0;
    __reload_page_table();

    kboot_info_t kboot_info;
    kboot_info.memory.region_overflowed = !parse_memory(multiboot2, &kboot_info);
    kboot_info.framebuffer.valid = parse_framebuffer(multiboot2, &kboot_info);
    kboot_info.boot_uuid_valid = parse_boot_uuid(multiboot2, &kboot_info);
    kboot_info.boot_media_type = MEDIA_TYPE_LIVE; // hardcoded for now

    // VMEM_PAGE_SIZE_SMALL
    kboot_info.kernel_page_count = (LNK_KERNEL_END_PHYSICAL + 0x1000 - 1) >> 12;

    virtual_kernel_entry(kboot_info);
    while (true);
}