#include "libtype.h"
#include "libdef.h"
#include "processor/amd64_linker.h"
#include "kbootinfo.h"
#include "processor/amd64_multiboot.h"
#include "libstring.h"
#include "processor/amd64_cpu.h"
#include "libmem.h"

// in main.c
__noreturn
void virtual_kernel_entry(kboot_info_t kboot_info);

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
bool parse_cmdline(multiboot2_info_t* multiboot2, kboot_info_t* kboot_info) {
    const multiboot2_tag_cmdline_t* tag = mb2_get_cmdline_tag(multiboot2);
    if (!tag)
        return false;

    const char* cmdline = tag->string;
    u64 len = strlen(cmdline);
    if (len >= CMDLINE_MAX)
        len = CMDLINE_MAX - 1;

    memcpy(kboot_info->cmdline, cmdline, len);
    return true;
}

typedef struct __rsdpv1 {
    char signature[8];
    u8 checksum;
    char oem_id[6];
    u8 revision;
    u32 rsdt_address;
    // ~~~ v2 
    u32 length;
    u64 xsdt_address;
    u8 ext_checksum;
    u8 reserved[3];
} __packed rsdp_t;

static
__section_text
bool parse_rsdp(multiboot2_info_t* multiboot2, kboot_info_t* kboot_info) {
    if (!multiboot2 || !kboot_info )
        return false;

    u64 root_paddr = 0;
    const multiboot2_tag_acpi_t* tag = mb2_get_apci_tag(multiboot2, MBTT_ACPI_RSDP2);

    if (tag && tag->size >= sizeof(*tag) + sizeof(rsdp_t)) {
        const rsdp_t* rsdp = (const rsdp_t*)((const u8*)tag + sizeof(*tag));
        if (rsdp->revision >= 2)
            root_paddr = rsdp->xsdt_address ? rsdp->xsdt_address : rsdp->rsdt_address;
    }

    if (!root_paddr) {
        tag = mb2_get_apci_tag(multiboot2, MBTT_ACPI_RSDP1);
        if (tag && tag->size >= sizeof(*tag) + 20u) {
            const rsdp_t* rsdp = (const rsdp_t*)((const u8*)tag + sizeof(*tag));
            root_paddr = rsdp->rsdt_address;
        }
    }

    if (!root_paddr)
        return false;

    kboot_info->extra_values[kboot_info->extra_values_written++] = (void*)root_paddr;
    return true;
}

__noreturn __section_text
void amd64_entry(multiboot2_info_t* multiboot2) {
    memzero((void*)LNK_BSS_START, LNK_BSS_SIZE);

    ((u64*)KERNEL_PHYS_TO_VIRT(page_table_l4))[0] = 0;
    amd64_reload_cr3();

    kboot_info_t kboot_info;
    memzero(&kboot_info, sizeof(kboot_info_t));

    kboot_info.memory.region_overflowed = !parse_memory(multiboot2, &kboot_info);
    kboot_info.framebuffer.valid = parse_framebuffer(multiboot2, &kboot_info);
    kboot_info.cmdline_valid = parse_cmdline(multiboot2, &kboot_info);
    kboot_info.boot_media_type = MEDIA_TYPE_LIVE; // hardcoded for now
    (void)parse_rsdp(multiboot2, &kboot_info);

    // VMEM_PAGE_SIZE_SMALL
    kboot_info.kernel_page_count = (LNK_KERNEL_END_PHYSICAL + 0x1000 - 1) >> 12;

    virtual_kernel_entry(kboot_info);
}