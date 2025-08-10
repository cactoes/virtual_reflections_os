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