#include "common.hpp"
#include "string.hpp"
#include "memory.hpp"
#include "cpu.hpp"
#include "debug.hpp"

#include "interrupt.hpp"
#include "virtual_thread.hpp"
#include "hardware_compatibility.hpp"

#include "drivers/vga_driver.hpp"
#include "drivers/pci_driver.hpp"
#include "drivers/pit_driver.hpp"
#include "drivers/keyboard_driver.hpp"
#include "drivers/mouse_driver.hpp"
#include "drivers/ahci_driver.hpp"
#include "drivers/ide_driver.hpp"
#include "drivers/e1000_driver.hpp"

#include "file_systems/iso9660.hpp"
#include "file_systems/fat32.hpp"

void draw_logo_vga_tm() {
    constexpr uint32_t x = 50;
    constexpr uint32_t y_base = 1;
    
    vga_tm_set_cursor(x, y_base + 1);  vga_tm_print("         *  ..        \n");
    vga_tm_set_cursor(x, y_base + 2);  vga_tm_print("        @@# @@.       \n");
    vga_tm_set_cursor(x, y_base + 3);  vga_tm_print("       @@*@@ @@.      \n");
    vga_tm_set_cursor(x, y_base + 4);  vga_tm_print("     .@@  #@@ @@:     \n");
    vga_tm_set_cursor(x, y_base + 5);  vga_tm_print("     @@ *@ :@@ @@:    \n");
    vga_tm_set_cursor(x, y_base + 6);  vga_tm_print("   :@@ #@%  .@@ @@=   \n");
    vga_tm_set_cursor(x, y_base + 7);  vga_tm_print("  :@@ %@#    .@@ @@+  \n");
    vga_tm_set_cursor(x, y_base + 8);  vga_tm_print(" +@@ @@*       @@ %@* \n");
    vga_tm_set_cursor(x, y_base + 9);  vga_tm_print(" @@: @@.       @@ -@@ \n");
    vga_tm_set_cursor(x, y_base + 10); vga_tm_print("  @@- @@:     @@ =@@  \n");
    vga_tm_set_cursor(x, y_base + 11); vga_tm_print("   @@* @@:   @@ *@@   \n");
    vga_tm_set_cursor(x, y_base + 12); vga_tm_print("    @@# @@. @@ *@@    \n");
    vga_tm_set_cursor(x, y_base + 13); vga_tm_print("     #@@ . @@ %@#     \n");
    vga_tm_set_cursor(x, y_base + 14); vga_tm_print("      +@@ @@ #@#      \n");
    vga_tm_set_cursor(x, y_base + 15); vga_tm_print("       -@@@ @@+       \n");
    vga_tm_set_cursor(x, y_base + 16); vga_tm_print("        :@  %=        \n");
}

extern "C" NORETURN void critical_fatal_ex(uint64_t code, const char* message, cpu_state_t* cpu_state = nullptr) {
    debug_print("[FATAL]: critical fatal triggerd: 0x%uh, %s\n", code, message);

    vga_tm_color_map_t color {};
    color.foreground = vga_tm_color_t::WHITE,
    color.background = vga_tm_color_t::BLUE,

    vga_tm_set_color(&color);
    vga_tm_clear_screen();
    vga_tm_print("\n\n\n       :(\n\n");
    vga_tm_print("       fatal error occured in kernel\n");
    vga_tm_print("           (0x%uh) %s \n", code, message);

    if (cpu_state) {
        vga_tm_set_cursor(10, 10); vga_tm_print("cf=%ul", (cpu_state->rflags >> 0) & 1);
        vga_tm_set_cursor(10, 11); vga_tm_print("? =%ul", (cpu_state->rflags >> 1) & 1);
        vga_tm_set_cursor(10, 12); vga_tm_print("pf=%ul", (cpu_state->rflags >> 2) & 1);
        vga_tm_set_cursor(10, 13); vga_tm_print("? =%ul", (cpu_state->rflags >> 3) & 1);
        vga_tm_set_cursor(10, 14); vga_tm_print("af=%ul", (cpu_state->rflags >> 4) & 1);
        vga_tm_set_cursor(10, 15); vga_tm_print("? =%ul", (cpu_state->rflags >> 5) & 1);
        vga_tm_set_cursor(10, 16); vga_tm_print("zf=%ul", (cpu_state->rflags >> 6) & 1);
        vga_tm_set_cursor(10, 17); vga_tm_print("sf=%ul", (cpu_state->rflags >> 7) & 1);
        vga_tm_set_cursor(10, 18); vga_tm_print("tf=%ul", (cpu_state->rflags >> 8) & 1);
        vga_tm_set_cursor(10, 19); vga_tm_print("if=%ul", (cpu_state->rflags >> 9) & 1);
        vga_tm_set_cursor(10, 20); vga_tm_print("df=%ul", (cpu_state->rflags >> 10) & 1);
        vga_tm_set_cursor(10, 21); vga_tm_print("of=%ul", (cpu_state->rflags >> 11) & 1);
        
        vga_tm_set_cursor(20, 10); vga_tm_print("r8= 0x%uh", cpu_state->r8);
        vga_tm_set_cursor(20, 11); vga_tm_print("r9= 0x%uh", cpu_state->r9);
        vga_tm_set_cursor(20, 12); vga_tm_print("r10=0x%uh", cpu_state->r10);
        vga_tm_set_cursor(20, 13); vga_tm_print("r11=0x%uh", cpu_state->r11);
        vga_tm_set_cursor(20, 14); vga_tm_print("r12=0x%uh", cpu_state->r12);
        vga_tm_set_cursor(20, 15); vga_tm_print("r13=0x%uh", cpu_state->r13);
        vga_tm_set_cursor(20, 16); vga_tm_print("r14=0x%uh", cpu_state->r14);
        vga_tm_set_cursor(20, 17); vga_tm_print("r15=0x%uh", cpu_state->r15);

        vga_tm_set_cursor(35, 10); vga_tm_print("rax=0x%uh", cpu_state->rax);
        vga_tm_set_cursor(35, 11); vga_tm_print("rcx=0x%uh", cpu_state->rcx);
        vga_tm_set_cursor(35, 12); vga_tm_print("rdx=0x%uh", cpu_state->rdx);
        vga_tm_set_cursor(35, 13); vga_tm_print("rbp=0x%uh", cpu_state->rbp);
        vga_tm_set_cursor(35, 14); vga_tm_print("rsi=0x%uh", cpu_state->rsi);
        vga_tm_set_cursor(35, 15); vga_tm_print("rdi=0x%uh", cpu_state->rdi);
        vga_tm_set_cursor(35, 16); vga_tm_print("rip=0x%uh", cpu_state->rip);
        vga_tm_set_cursor(35, 17); vga_tm_print("rsp=0x%uh", cpu_state->rsp);

        debug_print("[cf:%ul] ", (cpu_state->rflags >> 0) & 1);
        debug_print("[?:%ul] ", (cpu_state->rflags >> 1) & 1);
        debug_print("[pf:%ul] ", (cpu_state->rflags >> 2) & 1);
        debug_print("[?:%ul] ", (cpu_state->rflags >> 3) & 1);
        debug_print("[af:%ul] ", (cpu_state->rflags >> 4) & 1);
        debug_print("[?:%ul] ", (cpu_state->rflags >> 5) & 1);
        debug_print("[zf:%ul] ", (cpu_state->rflags >> 6) & 1);
        debug_print("[sf:%ul] ", (cpu_state->rflags >> 7) & 1);
        debug_print("[tf:%ul] ", (cpu_state->rflags >> 8) & 1);
        debug_print("[if:%ul] ", (cpu_state->rflags >> 9) & 1);
        debug_print("[df:%ul] ", (cpu_state->rflags >> 10) & 1);
        debug_print("[of:%ul]\n", (cpu_state->rflags >> 11) & 1);

        debug_print("r8= 0x%uh\n", cpu_state->r8);
        debug_print("r9= 0x%uh\n", cpu_state->r9);
        debug_print("r10=0x%uh\n", cpu_state->r10);
        debug_print("r11=0x%uh\n", cpu_state->r11);
        debug_print("r12=0x%uh\n", cpu_state->r12);
        debug_print("r13=0x%uh\n", cpu_state->r13);
        debug_print("r14=0x%uh\n", cpu_state->r14);
        debug_print("r15=0x%uh\n\n", cpu_state->r15);

        debug_print("rax=0x%uh\n", cpu_state->rax);
        debug_print("rcx=0x%uh\n", cpu_state->rcx);
        debug_print("rdx=0x%uh\n", cpu_state->rdx);
        debug_print("rbp=0x%uh\n", cpu_state->rbp);
        debug_print("rsi=0x%uh\n", cpu_state->rsi);
        debug_print("rdi=0x%uh\n", cpu_state->rdi);
        debug_print("rip=0x%uh\n", cpu_state->rip);
        debug_print("rsp=0x%uh\n", cpu_state->rsp);
    }
    
    while (true)
        cpu_halt();
}

NAKED NORETURN inline void critical_fatal(uint64_t code, const char* message) {
    asm volatile (
        // Reserve space for a cpu_state_t and align the stack.
        "sub %[state_size], %%rsp\n"
        "and $-16, %%rsp\n"

        // Dump the CPU state. Pass current rsp as the state pointer.
        "mov %%rsp, %%rdi\n"
        "call __get_cpu_state\n"

        // Move the incoming parameters into temporary registers.
        "mov %[code], %%rcx\n"    // Save code in RCX.
        "mov %[msg], %%r8\n"       // Save message in R8.

        // Set up the arguments for critical_fatal_ex:
        // rdi = code, rsi = message, rdx = cpu_state (rsp).
        "mov %%rcx, %%rdi\n"
        "mov %%r8, %%rsi\n"
        "mov %%rsp, %%rdx\n"
        "call critical_fatal_ex\n"

        :
        : [code] "r"(code),
          [msg] "r"(message),
          [state_size] "i"(sizeof(cpu_state_t))
        : "rcx", "r8", "rdi", "rsi", "rdx", "memory"
    );
}

cpu_state_t* handle_critical_interrupt(uint64_t code, cpu_state_t* rsp) {
    switch (code) {
        case 0x0: critical_fatal_ex(code, "FATAL: division by zero", rsp); break;
        case 0x1: critical_fatal_ex(code, "FATAL: single-step interrupt (see trap flag)", rsp); break;
        case 0x2: critical_fatal_ex(code, "FATAL: nmi", rsp); break;
        case 0x3: critical_fatal_ex(code, "FATAL: breakpoint (which benefits from the shorter 0xcc encoding of int 3)", rsp); break;
        case 0x4: critical_fatal_ex(code, "FATAL: overflow", rsp); break;
        case 0x5: critical_fatal_ex(code, "FATAL: bound range exceeded", rsp); break;
        case 0x6: critical_fatal_ex(code, "FATAL: invalid opcode", rsp); break;
        case 0x7: critical_fatal_ex(code, "FATAL: coprocessor not available", rsp); break;
        case 0x8: critical_fatal_ex(code, "FATAL: double fault", rsp); break;
        case 0x9: critical_fatal_ex(code, "FATAL: coprocessor segment overrun (386 or earlier only)", rsp); break;
        case 0xA: critical_fatal_ex(code, "FATAL: invalid task state segment", rsp); break;
        case 0xB: critical_fatal_ex(code, "FATAL: segment not present", rsp); break;
        case 0xC: critical_fatal_ex(code, "FATAL: stack segment fault", rsp); break;
        case 0xD: critical_fatal_ex(code, "FATAL: general protection fault", rsp); break;
        case 0xE: critical_fatal_ex(code, "FATAL: page fault", rsp); break;
        case 0xF: critical_fatal_ex(code, "FATAL: reserved", rsp); break;
        case 0x10: critical_fatal_ex(code, "FATAL: x87 floating point exception", rsp); break;
        case 0x11: critical_fatal_ex(code, "FATAL: alignment check", rsp); break;
        case 0x12: critical_fatal_ex(code, "FATAL: machine check", rsp); break;
        case 0x13: critical_fatal_ex(code, "FATAL: simd floating-point exception", rsp); break;
        case 0x14: critical_fatal_ex(code, "FATAL: virtualization exception", rsp); break;
        case 0x15: critical_fatal_ex(code, "FATAL: control protection exception (only available with cet)", rsp); break;
        default: critical_fatal_ex(code, "FATAL: unkown", rsp); break;
    }
    return rsp;
}

// static inline int64_t syscall(uint64_t number, int64_t a1, int64_t a2, int64_t a3, int64_t a4, int64_t a5, int64_t a6) {
//     int64_t return_value;
//     asm volatile (
//         "mov %[a4], %%r10\n"
//         "mov %[a5], %%r8\n"
//         "mov %[a6], %%r9\n"
//         "syscall"
//         : "=a"(return_value)
//         : "a"(number),
//           "D"(a1), "S"(a2), "d"(a3),
//           [a4]"r"(a4), [a5]"r"(a5), [a6]"r"(a6)
//         : "rcx", "r11", "r8", "r9", "r10", "memory"
//     );
//     return return_value;
// }

// void syscall_handle_interrupt(cpu_state_t* rsp) {
//     uint64_t number = rsp->rax;

//     switch (number) {
//         case 0:
//             *(uint64_t*)rsp->rdi = 123456;
//             rsp->rax = 123;
//             break;
//         default:
//             rsp->rax = 1;
//             break;
//     }
// }

cpu_state_t* handle_other_interrupt(uint64_t code, cpu_state_t* rsp) {
    // if (code == 0x2E) return rsp;
    // if (code == 0x2F) return rsp;

    if (code == 0x2b)
        return e1000_handle_interrupt(rsp);

    if (code == 0x80) {
        // syscall_handle_interrupt(rsp);
        // TODO @since 07/07/2025 -- 20:57
        // we kinda need usermode first ...
        return rsp;
    }
    
    critical_fatal_ex(code, "FATAL: unhandled interrupt", rsp);
    return rsp;
}

void thread_test() {
    debug_print("test\n");
    while (true) {}
}

int mouse_pox_x_max = VGA_BUFFER_WIDTH;
int mouse_pox_y_max = VGA_BUFFER_HEIGHT;

int mouse_pos_x = 0;
int mouse_pos_y = 0;

void mouse_handler(mouse_state_t* state) {
    state->dy = -1 * state->dy;

    if (mouse_pos_y + state->dy >= mouse_pox_y_max)
        mouse_pos_y = mouse_pox_y_max - 1;
    else if (mouse_pos_y + state->dy < 0)
        mouse_pos_y = 0;
    else
        mouse_pos_y += state->dy;

    if (mouse_pos_x + state->dx >= mouse_pox_x_max)
        mouse_pos_x = mouse_pox_x_max - 1;
    else if (mouse_pos_x + state->dx < 0)
        mouse_pos_x = 0;
    else
        mouse_pos_x += state->dx;
}

bool was_key_pressed = false;

void kb_handler(key_state_t*) {
    was_key_pressed = true;
}

static char ascii_table[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-',
    '4', '5', '6', '+', '1', '2', '3', '0', '.'
};

static char shifted_table[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-',
    '4', '5', '6', '+', '1', '2', '3', '0', '.'
};

char to_ascii(uint64_t scancode) {
    auto key_state = keyboard_get_key_state(scancode);

    if (scancode > sizeof(ascii_table)) 
        return 0;

    auto shift_key = keyboard_get_key_state(SC_LSHIFT);
    auto caps_key = keyboard_get_key_state(SC_CAPS_LOCK);

    if (shift_key->is_pressed && !caps_key->is_pressed)
        return shifted_table[scancode];
    else if (shift_key->is_pressed && caps_key->is_pressed)
        return ascii_table[scancode];
    else if (!shift_key->is_pressed && caps_key->is_pressed)
        return shifted_table[scancode];
    else
        return ascii_table[scancode];
}

char getchar() {
    auto scan_code = 0;
    char ascii = 0;
    while (scan_code == 0 || ascii == 0) {
        scan_code = keyboard_get_last_key();

        if (!keyboard_get_key_state(scan_code)->is_pressed)
            continue;

        ascii = to_ascii(scan_code);
    }

    keyboard_clear_last_key();

    return ascii;
}

static vfs_file_t __std_out_file { .file_type = vfs_file_type_t::SYS_STD_OUT };
static vfs_file_t __dbg_file { .file_type = vfs_file_type_t::SYS_DBG };

#define STD_OUT &__std_out_file
#define DBG &__dbg_file

void file_put_char(char c, vfs_file_t* file) {
    vfs_write("/dev/stdout", file, &c, sizeof(c));
}

void file_put_str(const char* c, vfs_file_t* file) {
    vfs_write("/dev/stdout", file, c, sizeof(c) * strlen(c));
}

extern "C" void kernel_entry(multiboot_t* multiboot_struct, void* kpml4) {
    file_put_str("teststr456", DBG);

    debug_init();

    hc::gdt_tss::init();

    vga_tm_clear_screen();
    draw_logo_vga_tm();
    vga_tm_set_cursor(0, 0);
    vga_tm_print("[KERNEL BOOT SEQUENCE]\n");
    vga_tm_print("> SYSTEM EPISODE: 0\n");
    vga_tm_print("> DEVICE DRIVERS: AHCI, IDE, PS2 KB, PS2 M\n");
    vga_tm_print("> SCREEN MODE: VGA\n");
    vga_tm_print("> INITIALIZING MEMORY ...\n");

    if (!mb_has_valid_magic(multiboot_struct))
        critical_fatal(multiboot_struct->magic, "multiboot magic was invalid");

    if (!vmem_init(multiboot_struct, kpml4))
        critical_fatal(0x0, "vmem_init failed");

    heap_t heap {};
    if (!heap_init(&heap, kpml4, (void*)0x40000000, 0x100000 * 32))
        critical_fatal(0x0, "heap_init failed");
    set_global_heap(&heap);

    vga_tm_print("> COMPLETE\n");

    vga_tm_print("> INITIALIZING PERIPHERALS ...\n");

    key_state_t key_states[KEY_STATE_ARRAY_SIZE] = {};
    keyboard_init(key_states);

    int_set_callback(interrupt_type::OTHER, handle_other_interrupt);
    int_set_callback(interrupt_type::CRITICAL, handle_critical_interrupt);
    int_set_callback(interrupt_type::KEYBOARD, keyboard_handle_interrupt);
    int_set_callback(interrupt_type::PIT, pit_handle_interrupt);
    int_set_callback(interrupt_type::MOUSE, mouse_handle_interrupt);
    int_init();

    mouse_state_t mouse_state = {};
    ps2_mouse_init(&mouse_state);

    vector<pit_timer_t> timers {};
    pit_init(&timers);

    // manual (main) thread setup
    vthread_t main_thread {};
    main_thread.vt_state = vthread_state_t::RUNNING;
    main_thread.vtid = 0;
    ((tls_base_t*)main_thread.tls)->vtid = 0;
    vthread_add(&main_thread);

    vthread_t thread{};
    vthread_create(&thread, thread_test);

    vga_tm_print("> COMPLETE\n");

    vfs_init();

    vga_tm_print("> KERNEL BOOT SEQUENCE COMPLETE\n");
    debug_print("> KERNEL BOOT SEQUENCE COMPLETE\n");

    vga_tm_print("\n[HARDWARE]\n");

    vector<pci_device_info_t> pci_devices {};
    pci_enumerate_devices(&pci_devices);

    for (VECTOR_LOOP(&pci_devices, device_node)) {
        const char* cd = pci_get_class_description(&device_node->value);
        debug_print("[PCI] dectected device:\n");
        debug_print("    class description  : %s\n", cd);
        debug_print("    vendor id          : 0x%uh\n", device_node->value.vendor_id);
        debug_print("    device id          : 0x%uh\n", device_node->value.device_id);
        debug_print("    bus                : 0x%uh\n", device_node->value.bus);
        debug_print("    device             : 0x%uh\n", device_node->value.device);
        debug_print("    function           : %u\n", device_node->value.function);
    }

    vga_tm_print("> PCI(E) DEVICES FOUND: %i\n", pci_devices.length());

    vector<pci_device_request_t> pci_devices_requested {};
    // ahci controller
    pci_devices_requested.insert_back(pci_device_request_t { .revision_id = (uint8_t)PCI_UNKNOWN, .prog_if = (uint8_t)1, .sub_class = (uint8_t)6, .class_code = (uint8_t)1 });
    // ide controller
    pci_devices_requested.insert_back(pci_device_request_t { .revision_id = (uint8_t)PCI_UNKNOWN, .prog_if = (uint8_t)PCI_UNKNOWN, .sub_class = (uint8_t)1, .class_code = (uint8_t)1 });
    // network controller
    pci_devices_requested.insert_back(pci_device_request_t { .revision_id = (uint8_t)PCI_UNKNOWN, .prog_if = (uint8_t)PCI_UNKNOWN, .sub_class = (uint8_t)0, .class_code = (uint8_t)2 });

    const bool found = pci_find_devices(&pci_devices, &pci_devices_requested);

    if (found) {
        pci_device_info_t* ahci_device = pci_devices.get_at(pci_devices_requested.get_at(0)->pci_device_index);

        const auto ahci_irq = pci_config_read(ahci_device->bus, ahci_device->device, ahci_device->function, 0x3C) & 0xFF;

        vector<ahci_sata_drive_t> drives {};
        ahci_init(kpml4, ahci_device, &drives);

        size_t device_index_sata = 0;
        for (VECTOR_LOOP(&drives, drive_node)) {
            debug_print("[AHCI] dectected drive:\n");
            debug_print("   model number            : %s\n", drive_node->value.model);
            debug_print("   serial number           : %s\n", drive_node->value.serial);
            debug_print("   firmware revision       : %s\n", drive_node->value.firmware);
            debug_print("   total lba sectors       : %ul\n", drive_node->value.lba);
            debug_print("   drive capacity          : %ul bytes\n", drive_node->value.capacity);
            debug_print("   logical sector size     : %u bytes\n", drive_node->value.logical_sector_size);
            debug_print("   physical sector size    : %u bytes\n", drive_node->value.physical_sector_size);

            char device_name[6] { 0 };
            sprintf(device_name, 6, "sata%i", device_index_sata);
            
            vfs_node_t* file_system_node = vfs_mount_dev(device_name, drive_type_t::SATA, fs_type_t::FAT32);
            file_system_node->drive.device = &drive_node->value;
            if (fat32_drive_init(&file_system_node->drive, &file_system_node->fs) != 0)
                debug_print("failed to mount device: %s\n", device_name);

            // fat32_drive_deinit(&file_system_node->fs);
            device_index_sata++;
        }

        vga_tm_print("> SATA DRIVES FOUND: %i\n", drives.length());

        // uint8_t data[512] {};
        // ahci_read(&drives.first()->value, 0, 1, data);
        // for (int i = 0; i < 512; ++i) {
        //     debug_print("%uh ", data[i]);
        //     if ((i + 1) % 16 == 0) debug_print("\n");
        // }

        pci_device_info_t* ide_device = pci_devices.get_at(pci_devices_requested.get_at(1)->pci_device_index);

        vector<ata_drive_t> ata_drives {};
        ide_init(ide_device, &ata_drives);

        size_t device_index = 0;
        for (VECTOR_LOOP(&ata_drives, drive_node)) {
            static const char* channels[3] { "NONE", "PRIMARY", "SECONDARY" };
            static const char* types[3] { "NONE", "MASTER", "SLAVE" };
            
            debug_print("[IDE] dectected drive:\n");
            debug_print("   type                    : %s\n", drive_node->value.is_atapi ? "ATAPI" : "ATA");
            debug_print("   channel                 : %s\n", channels[(int)drive_node->value.channel_name]);
            debug_print("   master/slave            : %s\n", types[(int)drive_node->value.type]);
            debug_print("   model number            : %s\n", drive_node->value.model);
            debug_print("   serial number           : %s\n", drive_node->value.serial);
            debug_print("   firmware revision       : %s\n", drive_node->value.firmware);
            debug_print("   total lba sectors       : %ul\n", drive_node->value.lba);
            debug_print("   drive capacity          : %ul bytes\n", drive_node->value.capacity);
            debug_print("   logical sector size     : %u bytes\n", drive_node->value.logical_sector_size);
            debug_print("   physical sector size    : %u bytes\n", drive_node->value.physical_sector_size);


            // TODO @since 18/06/2025 -- 02:46
            // check file system
            if (drive_node->value.is_atapi) {
                char device_name[5] { 0 };
                sprintf(device_name, 5, "ata%i", device_index);
                
                vfs_node_t* file_system_node = vfs_mount_dev(device_name, drive_type_t::ATAPI, fs_type_t::ISO9660);
                file_system_node->drive.device = &drive_node->value;
                if (iso9660_drive_init(&file_system_node->drive, &file_system_node->fs) != 0)
                    debug_print("failed to mount device: %s\n", device_name);

                // iso9660_drive_deinit(&file_system_node->fs);
            }

            device_index++;
        }

        vga_tm_print("> ATA DRIVES FOUND: %i\n", ata_drives.length());

        // TODO @since 18/06/2025 -- 13:20
        // might work? figure out if qemu links to real network
        // *update:
        // might be qemu problem with connection to actual internet (windows host)
        pci_device_info_t* network_device = pci_devices.get_at(pci_devices_requested.get_at(2)->pci_device_index);

        e1000_device_t e1000_device {};
        e1000_init(kpml4, network_device, &e1000_device);

        debug_print("[NETWORK] dectected controller:\n");
        debug_print("   mac                     : %uh:%uh:%uh:%uh:%uh:%uh\n", e1000_device.mac[0], e1000_device.mac[1], e1000_device.mac[2], e1000_device.mac[3], e1000_device.mac[4], e1000_device.mac[5], e1000_device.mac[6]);
        vga_tm_print("> NETWORK CARD FOUND: (INTEL) E1000\n");
    }

    vga_tm_print("\n[SYSTEM CONFIG]\n");

    vfs_file_t file {};
    vfs_read("/dev/ata0/.env", &file);
    for (size_t i = 0; i < file.size; i++)
        vga_tm_print(((char*)file.buffer)[i]);
    vfs_close_file(&file);

    debug_print("SATA0->test.txt: \"");
    vfs_file_t file2 {};
    vfs_read("/dev/sata0/test.txt", &file2);
    for (size_t i = 0; i < file2.size; i++)
        debug_print(((char*)file2.buffer)[i]);
    vfs_close_file(&file2);
    debug_print("\"\n");

    keyboard_add_handler(kb_handler);

    // uint64_t data;
    // debug_print("data: 0x%p\n", &data);
    // // asm volatile ("mov %[ptr], %%rdi" :: [ptr] "r"(&data) : "rdi", "memory");
    // // asm volatile ("int $0x80" ::: "memory");
    // auto scresult = syscall(0, (int64_t)&data, 0, 0, 0, 0, 0);
    // debug_print("data: 0x%p\n", &data);
    // debug_print("data: %ul\n", data);
    // debug_print("scresult: %il\n", scresult);

    const char* spinner[] = { "   ", ".  ", ".. ", "..." };
    constexpr size_t chars_size = (sizeof(spinner) / sizeof(char*));
    size_t i = 0;

    while (!was_key_pressed) {
        vga_tm_set_cursor(0, VGA_TM_NUM_ROWS - 1);
        vga_tm_print("> PRESS ANY KEY TO START %s", spinner[i++ % chars_size]);
        vga_tm_set_cursor(VGA_TM_NUM_COLS, VGA_TM_NUM_ROWS);
        pit_sleep(250);
    }

    vga_tm_clear_screen();

    while (true) {
        char ch = getchar();
        vga_tm_print("%c", ch);
    }

    // TODO @since 28/06/2025 -- 20:44
    // shell / interative ui

    // vga_generic_buffer_t buffer{};
    // vga_gm_buffer_create(&buffer);
    // vga_gm_startup(&buffer);
    // vga_gm_render();

    // ps2_mouse_add_handler(mouse_handler);

    // // 60 fps
    // constexpr float TARGET_FRAME_MS = 1000 / 60;
    // uint64_t prev_tick = pit_timer_read();

    // while (true) {
    //     const uint64_t frame_begin = pit_timer_read();

    //     const float delta_sec = static_cast<float>(frame_begin - prev_tick) / 10;
    //     prev_tick = frame_begin;

    //     vga_gm_draw::clear(&buffer, vga_gm_color_index_t::BLACK);
    //     vga_gm_draw::pixel(&buffer, mouse_pos_x * delta_sec, mouse_pos_y * delta_sec, vga_gm_color_index_t::WHITE);
    //     vga_gm_render();

    //     while ((pit_timer_read() - frame_begin) < TARGET_FRAME_MS) { }
    // }
}