
// =================================================
// TODO: move
typedef void (*constructor)();
extern "C" constructor __lnk_start_ctors;
extern "C" constructor __lnk_end_ctors;

extern "C" void call_constructors() {
    for (constructor* i = &__lnk_start_ctors; i != &__lnk_end_ctors; i++)
        (*i)();
}

typedef unsigned long long size_t;
typedef unsigned long long uint64_t;
typedef          long long int64_t;

typedef unsigned int uint32_t;
typedef          int int32_t;

typedef unsigned short uint16_t;
typedef          short int16_t;

typedef unsigned char uint8_t;
typedef   signed char int8_t;

void* memset(void* dest, uint8_t val, size_t size) {
    if (size == 0)
        return dest;

    uint8_t* p_dest = (uint8_t*)dest;

    for (size_t i = 0; i < size; i++)
        p_dest[i] = val;

    return dest;
}

void* memzero(void* dest, size_t size) {
    return memset(dest, 0, size);
}

void* memcpy(void* dest, const void* src, size_t size) {
    if (size == 0)
        return dest;

    uint8_t* p_dest = (uint8_t*)dest;
    const uint8_t* p_src = (uint8_t*)src;

    for (size_t i = 0; i < size; i++)
        p_dest[i] = p_src[i];

    return dest;
}

bool check_alignment(uint64_t addr, uint64_t align) {
    return (addr & (align - 1)) == 0;
}

uint64_t align_addr_up(uint64_t addr, uint64_t align) {
    return (addr + (align - 1)) & ~(align - 1);
}

uint64_t align_addr_down(uint64_t addr, uint64_t align) {
    return (addr) & ~(align - 1);
}

#define PAGE_SIZE           0x1000
#define PAGE_SIZE_LARGE     0x200000
#define PAGE_SIZE_HUGE      0x40000000

// helper functions for page tables
#define KPAGING_GET_PE(virtual_addr, offset)    ((((uint64_t)(virtual_addr)) >> (offset)) & 0x1FF)
#define KPAGING_GET_ENTRY(table, entry)         ((uint64_t*)(((uint64_t*)(table))[((uint64_t)(entry))] & ~0xFFF))
#define KPAGING_CHECK_ENTRY(table, entry)       ((((uint64_t*)(table))[((uint64_t)(entry))]) & 1)

// page flags
#define PF_PRESENT              (1 << 0)
#define PF_READ_WRITE           (1 << 1)
#define PF_USER_SUPERVISOR      (1 << 2)
#define PF_WRITE_THROUGH        (1 << 3)
#define PF_CACHE_DISABLE        (1 << 4)
#define PF_ACCESSED             (1 << 5)
#define PF_DIRTY                (1 << 6)
#define PF_PAGE_SIZE            (1 << 7)
#define PF_GLOBAL               (1 << 8)

bool bitmap_get(uint64_t* bitmap, size_t size, size_t index) {
    if (index >= size * (sizeof(uint64_t) * 8))
        return false;

    const size_t item_index = index / 64;
    const size_t bit_index = index % 64;
    return (bitmap[item_index] >> bit_index) & 1;
}

template <size_t size>
bool bitmap_get(uint64_t (&bitmap)[size], size_t index) {
    return bitmap_get(bitmap, size, index);
}

void bitmap_set(uint64_t* bitmap, size_t size, size_t index, bool state) {
    if (index >= size * (sizeof(uint64_t) * 8))
        return;

    const size_t item_index = index / 64;
    const size_t bit_index = index % 64;

    state ? bitmap[item_index] |= (1ULL << bit_index)
          : bitmap[item_index] &= ~(1ULL << bit_index);
}

template <size_t size>
void bitmap_set(uint64_t (&bitmap)[size], size_t index, bool state) {
    return bitmap_set(bitmap, size, index, state);
}

constexpr uint64_t hash_fnv1a_64(const char* str, uint64_t hash = 14695981039346656037ULL) {
    return (*str == '\0') ? hash :
        hash_fnv1a_64(str + 1, (hash ^ static_cast<uint64_t>(*str)) * 1099511628211ULL);
}

constexpr uint64_t hash_string_64(const char* str, uint64_t hash = 0ULL) {
    return (*str == '\0') ? hash :
        hash_string_64(str + 1, (hash << 1) + static_cast<uint64_t>(*str));
}

// =================================================

struct memory_map_entry_t {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed)) ;

struct multiboot_info_t {
    uint32_t flags;

    // Memory info
    uint32_t mem_lower;
    uint32_t mem_upper;

    // Boot device
    uint32_t boot_device;

    // Command line
    uint32_t cmdline;

    // Modules
    uint32_t mods_count;
    uint32_t mods_addr;

    // ELF section headers
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;

    // Memory map
    uint32_t mmap_length;
    uint32_t mmap_addr;

    // Drive info
    uint32_t drives_length;
    uint32_t drives_addr;

    // Configuration table
    uint32_t config_table;
    uint32_t boot_loader_name;

    // APM table
    uint32_t apm_table;

    // Video information
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint32_t vbe_mode;
    uint32_t vbe_interface_seg;
    uint32_t vbe_interface_off;
    uint32_t vbe_interface_len;
} __attribute__((packed));

struct multiboot_t {
    uint64_t magic;
    multiboot_info_t* info;
};

enum class memory_map_type_t : uint32_t {
    UNKOWN           = 0,
    USABLE           = 1,
    RESERVED         = 2,
    ACPI_RECLAIMABLE = 3,
    ACPI_NVS         = 4
};

enum class multiboot_flags_t : uint32_t {
    MEM          = (1 << 0),
    BOOT_DEVICE  = (1 << 1),
    CMDLINE      = (1 << 2),
    MODS         = (1 << 3),
    AOUT_SYMS    = (1 << 4),
    ELF_SYMS     = (1 << 5),
    MMAP         = (1 << 6),
    DRIVES       = (1 << 7),
    CONFIG_TABLE = (1 << 8),
    BOOT_LOADER  = (1 << 9),
    APM_TABLE    = (1 << 10),
    VBE          = (1 << 11)
};

bool mb_has_valid_magic(multiboot_t* multiboot_struct) {
    return multiboot_struct->magic == 0x2BADB002;
}

memory_map_entry_t* mb_get_first_entry(multiboot_t* multiboot_struct) {
    const auto& mbi = multiboot_struct->info;

    if (!(mbi->flags & (uint32_t)multiboot_flags_t::MMAP))
        return nullptr;

    auto memory_map_entry = (memory_map_entry_t*)(uint64_t)mbi->mmap_addr;

    if ((uint64_t)memory_map_entry < (uint64_t)(mbi->mmap_addr + mbi->mmap_length))
        return memory_map_entry;

    return nullptr;
}

memory_map_entry_t* mb_get_next_entry(multiboot_t* multiboot_struct, memory_map_entry_t* prev) {
    const auto& mbi = multiboot_struct->info;

    auto memory_map_entry = (memory_map_entry_t*)((uint64_t)prev + prev->size + sizeof(prev->size));

    if ((uint64_t)memory_map_entry < (uint64_t)(mbi->mmap_addr + mbi->mmap_length))
        return memory_map_entry;
    
    return nullptr;
}

// variable placed at the end of the kernel
extern "C" uint64_t __lnk_end_kernel;

#define KERNEL_PAGE_CRITICAL_BITMAP_SIZE 0x20000
#define KERNEL_PAGE_BITMAP_SIZE 0x40000

uint64_t kernel_page_critical_bitmap[KERNEL_PAGE_CRITICAL_BITMAP_SIZE] {};
uint64_t kernel_page_bitmap[KERNEL_PAGE_BITMAP_SIZE] {};

void __flush_tlb(uint64_t* virtual_address) { asm volatile("invlpg (%0)" : : "r"(virtual_address) : "memory"); }
void __set_pml4(void* ptr) { asm volatile("mov %0, %%cr3" : : "r"(ptr) : "memory"); }

void* vmem_get_page_critical() {
    // slow ahh
    for (size_t i = 1; i < KERNEL_PAGE_CRITICAL_BITMAP_SIZE; i++) {
        if (!bitmap_get(kernel_page_critical_bitmap, i)) {
            bitmap_set(kernel_page_critical_bitmap, i, true);
            return (void*)(i * PAGE_SIZE);
        }
    }

    return nullptr;
}

void* vmem_get_page() {
    // slow ahh
    for (size_t i = 1; i < KERNEL_PAGE_BITMAP_SIZE; i++) {
        if (!bitmap_get(kernel_page_bitmap, i)) {
            bitmap_set(kernel_page_bitmap, i, true);
            return (void*)((i + KERNEL_PAGE_CRITICAL_BITMAP_SIZE) * PAGE_SIZE);
        }
    }

    return nullptr;
}

bool vmem_paging_reserve_at_adress(uint64_t address, size_t count = 1) {
    if (!check_alignment(address, PAGE_SIZE))
        return false;

    constexpr uint64_t end_critical_memory_space = KERNEL_PAGE_CRITICAL_BITMAP_SIZE * PAGE_SIZE;
    constexpr uint64_t end_memory_space = end_critical_memory_space + KERNEL_PAGE_BITMAP_SIZE * PAGE_SIZE;

    // uint64_t* bitmap = nullptr;
    // size_t size = 0;

    if (address + (PAGE_SIZE * count) < end_critical_memory_space) {
        size_t page_start_index = address / PAGE_SIZE;
        size_t current_count = 0;
        for (size_t i = page_start_index; i < KERNEL_PAGE_CRITICAL_BITMAP_SIZE; i++) {
            bitmap_get(kernel_page_critical_bitmap, i)
                ? current_count++
                : current_count = 0;

            // found section so we reserve it
            if (current_count == count) {
                for (; current_count > 0; current_count--)
                    bitmap_set(kernel_page_critical_bitmap, i - current_count, true);
                return true;
            }
        }

        return false;
    }

    if (address + (PAGE_SIZE * count) < end_memory_space) {
        size_t page_start_index = (address - end_critical_memory_space) / PAGE_SIZE;
        if (page_start_index < 0)
            return false;

        size_t current_count = 0;
        for (size_t i = page_start_index; i < KERNEL_PAGE_BITMAP_SIZE; i++) {
            bitmap_get(kernel_page_bitmap, i)
                ? current_count++
                : current_count = 0;

            // found section so we reserve it
            if (current_count == count) {
                for (; current_count > 0; current_count--)
                    bitmap_set(kernel_page_bitmap, i - current_count, true);
                return true;
            }
        }

        return false;
    }

    return false;
}

void vmem_identity_map(uint64_t* pml4) {
    if (!KPAGING_CHECK_ENTRY(pml4, 0)) {
        auto page = vmem_get_page_critical();
        pml4[0] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    auto pdpt = KPAGING_GET_ENTRY(pml4, 0);
    if (!KPAGING_CHECK_ENTRY(pdpt, 0)) {
        auto page = vmem_get_page_critical();
        pdpt[0] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    // identity map the first 1 GB
    auto pdt = KPAGING_GET_ENTRY(pdpt, 0);
    for (uint64_t pde = 0; pde < 512; pde++) {
        uint64_t physical_addr = (uint64_t)pde * PAGE_SIZE_LARGE;
        pdt[pde] = (physical_addr & ~0x1FFFFF) | PF_PRESENT | PF_READ_WRITE | PF_PAGE_SIZE;
        __flush_tlb((uint64_t*)physical_addr);
    }
}

void vmem_init(multiboot_t* multiboot_struct, void* pml4) {
    // TODO: check result vmem reserve

    // zero page
    memzero(0, PAGE_SIZE);

    const uint64_t aligned_kernel_end_addr = align_addr_up((uint64_t)&__lnk_end_kernel, PAGE_SIZE_LARGE);
    const uint64_t kernel_page_count = aligned_kernel_end_addr / PAGE_SIZE;
    vmem_paging_reserve_at_adress(0, kernel_page_count);

    for (auto mm_entry = mb_get_first_entry(multiboot_struct); mm_entry; mm_entry = mb_get_next_entry(multiboot_struct, mm_entry)) {
        // reserve physical pages for reserved memory
        if (mm_entry->type != (uint32_t)memory_map_type_t::USABLE) {
            if (mm_entry->addr + mm_entry->len > (KERNEL_PAGE_BITMAP_SIZE + KERNEL_PAGE_CRITICAL_BITMAP_SIZE) * PAGE_SIZE) {
                continue;
            }

            // reserve as much as possible
            for (size_t i = 0; i < mm_entry->len; i += PAGE_SIZE) {
                vmem_paging_reserve_at_adress(mm_entry->addr + i);
            }
        }
    }

    // first 1G is identity mapped
    vmem_identity_map((uint64_t*)pml4);
    __set_pml4(pml4);
}

void heap_init() {}

extern "C" void kernel_entry(multiboot_t* multiboot_struct, void* kpml4) {
    vmem_init(multiboot_struct, kpml4);

    while (true) {}
}