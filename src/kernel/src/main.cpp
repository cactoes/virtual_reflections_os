#include "arch/generic.hpp"
#include "arch/gdt.hpp"
#include "arch/interrupt.hpp"
#include "arch/pit.hpp"

#include "drivers/vga.hpp"
#include "drivers/pcie.hpp"
#include "drivers/ps2/keyboard.hpp"
#include "drivers/ps2/mouse.hpp"
#include "drivers/ps2/ps2.hpp"
#include "drivers/storage/ide.hpp"

#include "filesystems/iso9660.hpp"
#include "filesystems/vfs.hpp"

#include "memory/vmem.hpp"
#include "memory/heap.hpp"

#include "hardware/vhd.hpp"

#include "utils/debug.hpp"
#include "utils/vector.hpp"
#include "utils/event.hpp"

#include "multiboot.hpp"
#include "string.hpp"
#include "common.hpp"
#include "crash_handler.hpp"

#define HEAP_START_SIZE 0x100000 * 32 // 32 mb
#define PIT_TIMER_INTERVAL 1000 // times per second

enum print_mode_t {
    STD,
    DBG
};

void printf(print_mode_t mode, const char* p_str, ...) {
    char buffer[256] = { 0 };

    va_list args;
    va_start(args, p_str);
    size_t strlen = sprintf(buffer, (unsigned long int)sizeof(buffer), p_str, args);
    va_end(args);

    switch (mode) {
        case DBG:
            debug_puts(buffer);
            break;
        case STD:
        default:
            vga_tm_puts(&g_vga_tm_buffer, buffer);
            break;
    }
}

void pit_handle_interrupt() {
    // TODO @since 14/07/2025 -- 18:59
}

enum class interrupt_type_t {
    UNKOWN = -1,

    // wont change
    EXCEPTION_DIVISION_BY_ZERO = 0,
    EXCEPTION_SINGLE_STEP_INTERRUPT = 1,
    EXCEPTION_NMI = 2,
    EXCEPTION_BREAKPOINT = 3,
    EXCEPTION_OVERFLOW = 4,
    EXCEPTION_BOUND_RANGE_EXCEEDED = 5,
    EXCEPTION_INVALID_OPCODE = 6,
    EXCEPTION_COPROCESSOR_NOT_AVAILABLE = 7,
    EXCEPTION_DOUBLE_FAULT = 8,
    EXCEPTION_COPROCESSOR_SEGMENT_OVERRUN = 9,
    EXCEPTION_INVALID_TSS = 10,
    EXCEPTION_SEGMENT_NOT_PRESENT = 11,
    EXCEPTION_STACK_SEGMENT_FAULT = 12,
    EXCEPTION_GENERAL_PROTECTION_FAULT = 13,
    EXCEPTION_PAGE_FAULT = 14,
    EXCEPTION_RESERVED = 15,
    EXCEPTION_X87_FLOATING_POINT_EXCEPTION = 16,
    EXCEPTION_ALIGNMENT_CHECK = 17,
    EXCEPTION_MACHINE_CHECK = 18,
    EXCEPTION_SIMD_FP_EXCEPTION = 19,
    EXCEPTION_VIRTUALIZATION_EXCEPTION = 20,
    EXCEPTION_CONTROL_PROTECTION_EXCEPTION = 21,
    
    // wont change
    HARDWARE_PIT = 22,
    HARDWARE_KEYBOARD = 23,
    HARDWARE_CASCADE = 24,
    HARDWARE_COM2 = 25,
    HARDWARE_COM1 = 26,
    HARDWARE_LPT2 = 27,
    HARDWARE_FLOPPY_DISK = 28,
    HARDWARE_LPT1 = 29,
    HARDWARE_CMOS_RTC = 30,
    HARDWARE_FFP_L_SCSI_NIC = 31,
    HARDWARE_FFP_SSCI_NIC1 = 32,
    HARDWARE_FFP_SSCI_NIC2 = 33,
    HARDWARE_PS2_MOUSE = 34,
    HARDWARE_COPROCESSOR = 35,
    HARDWARE_PRIMARY_ATA_HD = 36,
    HARDWARE_SECONDARY_ATA_HD = 37,
    
    SOFTWARE_SYSTEMCALL,
};

bool is_interrupt_exception(interrupt_type_t type) {
    return ((int64_t)type >= 0 && (int64_t)type <= 21);
}

interrupt_type_t convert_interrupt_code(uint64_t code) {
    // exceptions
    if (is_interrupt_exception((interrupt_type_t)code))
        return (interrupt_type_t)code;

    // hardware
    if (code >= 32 && code <= 47)
        return (interrupt_type_t)(code - 10);

    // software
    if (code == 128)
        return interrupt_type_t::SOFTWARE_SYSTEMCALL;

    return interrupt_type_t::UNKOWN;
}

void* interrupt_handler(uint64_t code, cpu_state_t* p_rsp) {
    const auto interrupt_type = convert_interrupt_code(code);

    if (is_interrupt_exception(interrupt_type))
        __kernel_fatal(code, "critical interrupt triggerd", p_rsp);

    switch (interrupt_type) {
        case interrupt_type_t::HARDWARE_PIT:
            pit_handle_interrupt();
            interrupt_send_eoi(X86_64_INT_IRQ_PIT);
            return p_rsp;
        case interrupt_type_t::HARDWARE_KEYBOARD:
            ps2_keyboard_handle_interrupt();
            interrupt_send_eoi(X86_64_INT_IRQ_PS2_KEYBOARD);
            return p_rsp;
        case interrupt_type_t::HARDWARE_PS2_MOUSE:
            ps2_mouse_handle_interrupt();
            interrupt_send_eoi(X86_64_INT_IRQ_PS2_MOUSE);
            return p_rsp;
    }

    // for uncaught irq s
    if (code >= 0x20 && code < 0x2F)
        interrupt_send_eoi(code - 0x20);

    printf(DBG, "unkown interrupt triggerd: 0x%uh\n", code);
    return p_rsp;
}

// void trigger_pf() {
//     volatile int* ptr = (int*)0x1234564478;
//     int val = *ptr;
// }

// void trigger_gpf() {
//     asm volatile (
//         "mov $0xFFFF, %%ax\n"
//         "ltr %%ax\n"
//         :
//         :
//         : "rax"
//     );
// }

void on_key_down(const ps2_key_state_t* p_state) {
    static char s_ascii_table[128] = {
        0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
        '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
        0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-',
        '4', '5', '6', '+', '1', '2', '3', '0', '.'
    };

    static char s_ascii_table_upper[128] = {
        0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
        '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
        0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
        '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*',
        0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-',
        '4', '5', '6', '+', '1', '2', '3', '0', '.'
    };

    if (p_state->scan_code > 128 || !p_state->is_pressed)
        return;

    auto shift_key = ps2_keyboard_get_key_state(PS2_KEYBOARD_SC_LSHIFT);
    auto caps_key = ps2_keyboard_get_key_state(PS2_KEYBOARD_SC_CAPS_LOCK);

    char ch;

    if (shift_key->is_pressed && !caps_key->is_pressed)
        ch = s_ascii_table_upper[p_state->scan_code];
    else if (shift_key->is_pressed && caps_key->is_pressed)
        ch = s_ascii_table[p_state->scan_code];
    else if (!shift_key->is_pressed && caps_key->is_pressed)
        ch = s_ascii_table_upper[p_state->scan_code];
    else
        ch = s_ascii_table[p_state->scan_code];

    printf(DBG, "%c", ch);
}

void on_mouse(const ps2_mouse_state_t* p_state) {
    printf(DBG, "L: %i, M: %i, R: %i, S: %i\n", p_state->buttons.left, p_state->buttons.middle, p_state->buttons.right, p_state->ds);
}

void elf_driver_test(void* p_data, size_t file_size);

extern "C" void kernel_entry(void* p_multiboot_struct, void* p_kpml4) {
    // validate multiboot
    UNUSED(mb_has_valid_magic((multiboot_t*)p_multiboot_struct));

    // initialize the gdt / tss
    gdt_init();

    // initialze vga text mode
    vga_tm_init_buffer(&g_vga_tm_buffer, (void*)VGA_TM_BUFFER_ADDR, VGA_TM_NUM_COLS, VGA_TM_NUM_ROWS);
    vga_tm_clear_buffer(&g_vga_tm_buffer);

    // initialze the debug out stream
    debug_init();

    // initialze virtual memory
    UNUSED(vmem_init(p_multiboot_struct, p_kpml4));
    vmem_identity_map(p_kpml4);
    set_pml4(p_kpml4);

    // initialze the global heap
    heap_t heap {};
    UNUSED(heap_init(&heap, p_kpml4, (void*)VMEM_HEAP_START_ADDR, HEAP_START_SIZE));
    set_global_heap(&heap);

    // initialze the interrupt line(s)
    interrupt_set_handler(interrupt_handler);
    ps2_mouse_init();
    pit_init(PIT_TIMER_INTERVAL);
    interrupt_init(gdt_get_kernel_code_selector());

    if (ps2_port_test_device(ps2_device_type_t::KEYBOARD)) {
        printf(DBG, "[+] ps2/keyboard\n");
        mount_device("ps2/keyboard", nullptr);
    }

    if (ps2_port_test_device(ps2_device_type_t::MOUSE)) {
        printf(DBG, "[+] ps2/mouse\n");
        mount_device("ps2/mouse", nullptr);
    }

    // TODO @since 14/07/2025 -- 18:58
    // threads / processes

    linked_list<pci_device_t> pci_devices {};
    pci_enumerate_devices(&pci_devices);

    for (auto& device : pci_devices) {
        const char* cd = pci_get_class_description(&device);
        printf(DBG, "[+] %s\n", cd);
    }

    // ide device
    pci_class_info_t ide_device_class_info {
        .revision_id = (uint8_t)PCI_UNKNOWN,
        .prog_if = (uint8_t)PCI_UNKNOWN,
        .sub_class = (uint8_t)1,
        .class_code = (uint8_t)1
    };
    const pci_device_t* ide_controller = pci_find_device(&pci_devices, &ide_device_class_info);

    linked_list<ide_device_t> ide_devices {};
    ide_init(ide_controller, &ide_devices);
    
    size_t ide_device_index = 0;
    for (auto& drive : ide_devices) {
        char buffer[20];
        sprintf(buffer, 20, "ide/disk%i", ide_device_index++);
        printf(DBG, "[+] %s\n", buffer);
        mount_device(buffer, nullptr);
    }

    // TODO @since 05/08/2025 -- 01:18
    // detect file system
    iso9660_fs_data_t mounted_iso9660_fs_instance {};
    iso9660_drive_init(&ide_devices[0], &mounted_iso9660_fs_instance);

    // ahci device
    pci_class_info_t ahci_device_class_info {
        .revision_id = (uint8_t)PCI_UNKNOWN,
        .prog_if = (uint8_t)1,
        .sub_class = (uint8_t)6,
        .class_code = (uint8_t)1
    };
    const pci_device_t* ahci_controller = pci_find_device(&pci_devices, &ahci_device_class_info);
    // TODO @since 14/07/2025 -- 21:52
    
    // network device
    pci_class_info_t network_device_class_info {
        .revision_id = (uint8_t)PCI_UNKNOWN,
        .prog_if = (uint8_t)PCI_UNKNOWN,
        .sub_class = (uint8_t)0,
        .class_code = (uint8_t)2
    };
    const pci_device_t* network_controller = pci_find_device(&pci_devices, &network_device_class_info);
    // TODO @since 14/07/2025 -- 21:52

    virtual_file_system vfs {};
    vfs.create_directory("/mnt");

    auto disk_storage = ptr::make_unique<vfs_disk_storage>(&mounted_iso9660_fs_instance);
    vfs.mount("/mnt/disk0", move(disk_storage));

    // dynamic_array<uint8_t> driver_file {};
    // vfs.create_file_cache("/mnt/disk0/TestDriver.sys");
    // auto file = vfs.open_file("/mnt/disk0/TestDriver.sys");
    // vfs.read_file(file, &driver_file);

    // uint8_t* driver_data = driver_file.get_data();
    // size_t driver_data_size = driver_file.length();

    // elf_driver_test(driver_data, driver_data_size);

    // printf(DBG, "vfs file debug test:\n");
    // for (const auto& ch : file_content)
    //     printf(DBG, "%c", ch);
    // printf(DBG, "END\n");

    ps2_keyboard_event_subscribe(on_key_down);
    ps2_mouse_event_subscribe(on_mouse);

    // kernel finished
    printf(STD, "> SYSTEM READY\n");
    printf(DBG, "Kernel finished initializing\n");

    // we shoudn t reach this point since the kernel should never stop
    // incase we do just hang here so we dont break anything
    while (true);
}

// VVVV ELF SHIT VVVV

typedef uint16_t Elf32_Half;	// Unsigned half int
typedef uint32_t Elf32_Off;	    // Unsigned offset
typedef uint32_t Elf32_Addr;	// Unsigned address
typedef uint32_t Elf32_Word;	// Unsigned int
typedef int32_t  Elf32_Sword;	// Signed int

# define ELF_NIDENT	16

typedef struct {
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;      // section header table offset
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;  // section header size
    uint16_t e_shnum;      // number of section headers
    uint16_t e_shstrndx;   // index of section header string table
} Elf64_Ehdr;

enum Elf_Ident {
	EI_MAG0		= 0, // 0x7F
	EI_MAG1		= 1, // 'E'
	EI_MAG2		= 2, // 'L'
	EI_MAG3		= 3, // 'F'
	EI_CLASS	= 4, // Architecture (32/64)
	EI_DATA		= 5, // Byte Order
	EI_VERSION	= 6, // ELF Version
	EI_OSABI	= 7, // OS Specific
	EI_ABIVERSION	= 8, // OS Specific
	EI_PAD		= 9  // Padding
};

# define ELFMAG0	0x7F // e_ident[EI_MAG0]
# define ELFMAG1	'E'  // e_ident[EI_MAG1]
# define ELFMAG2	'L'  // e_ident[EI_MAG2]
# define ELFMAG3	'F'  // e_ident[EI_MAG3]

enum Elf_Type {
	ET_NONE		= 0, // Unkown Type
	ET_REL		= 1, // Relocatable File
	ET_EXEC		= 2  // Executable File
};

typedef struct {
    uint32_t sh_name;      // offset to section name in shstrtab
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;      // load address of section
    uint64_t sh_offset;    // offset in file/image
    uint64_t sh_size;      // size of section
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} Elf64_Shdr;

typedef struct {
    uint32_t st_name;      // offset into string table
    uint8_t  st_info;
    uint8_t  st_other;
    uint16_t st_shndx;
    uint64_t st_value;     // symbol value (address)
    uint64_t st_size;
} Elf64_Sym;

static inline Elf64_Shdr *elf_sheader(Elf64_Ehdr *hdr) {
	return (Elf64_Shdr *)((uint64_t)hdr + hdr->e_shoff);
}

static inline Elf64_Shdr *elf_section(Elf64_Ehdr *hdr, int idx) {
	return &elf_sheader(hdr)[idx];
}

Elf64_Shdr* find_section_by_name(const Elf64_Ehdr* ehdr, const uint8_t* elf_data, const char* name) {
    Elf64_Shdr* shdrs = (Elf64_Shdr*)(elf_data + ehdr->e_shoff);
    Elf64_Shdr* shstrtab = &shdrs[ehdr->e_shstrndx];
    const char* shstrtab_p = (const char*)(elf_data + shstrtab->sh_offset);

    for (int i = 0; i < ehdr->e_shnum; i++) {
        const char* sec_name = shstrtab_p + shdrs[i].sh_name;
        if (string(sec_name) == name) {
            return &shdrs[i];
        }
    }
    return 0;
}

uint64_t find_symbol_address(const uint8_t* elf_data, const Elf64_Shdr* symtab, const Elf64_Shdr* strtab, const char* sym_name) {
    int num_symbols = symtab->sh_size / symtab->sh_entsize;
    Elf64_Sym* symbols = (Elf64_Sym*)(elf_data + symtab->sh_offset);
    const char* strtab_p = (const char*)(elf_data + strtab->sh_offset);

    for (int i = 0; i < num_symbols; i++) {
        const char* curr_name = strtab_p + symbols[i].st_name;
        if (string(curr_name) == sym_name) {
            return symbols[i].st_value;
        }
    }
    return 0; // not found
}

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64_Phdr;

#define PT_LOAD 1

#include "kernel_api.hpp"

void elf_driver_test(void* p_data, size_t file_size) {
    auto header_ptr = (Elf64_Ehdr*)p_data;
    printf(DBG, "MAGIC: 0x%uh ", header_ptr->e_ident[EI_MAG0]);
    printf(DBG, "%c", header_ptr->e_ident[EI_MAG1]);
    printf(DBG, "%c", header_ptr->e_ident[EI_MAG2]);
    printf(DBG, "%c\n", header_ptr->e_ident[EI_MAG3]);

    printf(DBG, "CLASS: 0x%uh\n", header_ptr->e_ident[EI_CLASS]);
    
    printf(DBG, "DATA: 0x%uh\n", header_ptr->e_ident[EI_DATA]);
    
    printf(DBG, "MACHINE: 0x%uh\n", header_ptr->e_machine);
    
    printf(DBG, "EI_VERSION: 0x%uh\n", header_ptr->e_ident[EI_VERSION]);
    
    printf(DBG, "TYPE: 0x%uh\n", header_ptr->e_type);
    
    printf(DBG, "ENTRY: 0x%uh\n", header_ptr->e_entry);

    Elf64_Shdr* shdr = elf_sheader(header_ptr);

    printf(DBG, "SHNUM: 0x%uh\n", header_ptr->e_shnum);

    Elf64_Shdr* symtab = find_section_by_name(header_ptr, (const uint8_t*)p_data, ".symtab");
    Elf64_Shdr* strtab = find_section_by_name(header_ptr, (const uint8_t*)p_data, ".strtab");

    printf(DBG, "symtab: 0x%uh\n", symtab);
    printf(DBG, "strtab: 0x%uh\n", strtab);
    
    uint64_t symptr = find_symbol_address((const uint8_t*)p_data, symtab, strtab, "driver_init");
    // uint64_t kernel_test_function_sym = find_symbol_address((const uint8_t*)p_data, symtab, strtab, "kernel_test_function");
    printf(DBG, "symptr: 0x%uh\n", symptr);

    Elf64_Sym* symbols = (Elf64_Sym*)((const uint8_t*)p_data + symtab->sh_offset);
    int num_symbols = symtab->sh_size / symtab->sh_entsize;
    const char* strtab_p = (const char*)((const uint8_t*)p_data + strtab->sh_offset);

    for (int i = 0; i < num_symbols; i++) {
        const char* name = strtab_p + symbols[i].st_name;

        if (streq(name, "kernel_test_function")) {
            printf(DBG, "patching symbol: %s\n", name);
            symbols[i].st_value = (uint64_t)&kernel_test_function;
            break;
        }
    }

    uint8_t* base_load_address = (uint8_t*)GALLOC(0x2000);
    Elf64_Phdr* phdr = (Elf64_Phdr*)((uint8_t*)p_data + header_ptr->e_phoff);

    for (int i = 0; i < header_ptr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            memcpy(base_load_address + phdr[i].p_vaddr,
                ((uint8_t*)p_data) + phdr[i].p_offset,
                phdr[i].p_filesz);

            // Zero out .bss section if needed
            if (phdr[i].p_memsz > phdr[i].p_filesz) {
                memset(base_load_address + phdr[i].p_vaddr + phdr[i].p_filesz,
                    0,
                    phdr[i].p_memsz - phdr[i].p_filesz);
            }
        }
    }

    int (*driver_init)() = (int(*)())(base_load_address + symptr);
    int result = driver_init();
    printf(DBG, "result: %il\n", result);
}