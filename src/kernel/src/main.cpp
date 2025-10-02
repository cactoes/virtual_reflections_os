#include "arch/generic.hpp"
#include "arch/gdt.hpp"
#include "arch/interrupt.hpp"
#include "arch/pit.hpp"

#include "drivers/vga.hpp"
#include "drivers/pcie.hpp"
#include "drivers/pit.hpp"
#include "drivers/keyboard.hpp"
#include "drivers/ps2/keyboard.hpp"
#include "drivers/ps2/mouse.hpp"
#include "drivers/ps2/ps2.hpp"
#include "drivers/storage/ide.hpp"
#include "drivers/network/e1000.hpp"
#include "drivers/network/nidm.hpp"
#include "drivers/network/tcp.hpp"
#include "drivers/network/arp.hpp"

#include "interrupt_manager.hpp"

#include "filesystems/iso9660.hpp"
#include "filesystems/vfs.hpp"

#include "memory/vmem.hpp"
#include "memory/heap.hpp"

#include "utils/debug.hpp"
#include "utils/vector.hpp"
#include "utils/event.hpp"

#include "time/clock.hpp"

#include "gui/desktop.hpp"
#include "gui/games/minesweeper.hpp"

#include "multiboot.hpp"
#include "string.hpp"
#include "common.hpp"
#include "crash_handler.hpp"
#include "virtual_thread.hpp"
#include "random.hpp"

#define HEAP_START_SIZE 0x100000 * 32 // 32 mb
#define PIT_TIMER_INTERVAL 1000 // times per second

virtual_file_system* vfs_ptr = nullptr;

enum print_mode_t {
    STD,
    DBG
};

void printf(print_mode_t mode, const char* p_str, ...) {
    uint8_t buffer[256] = { 0 };

    va_list args;
    va_start(args, p_str);
    size_t strlen = sprintf((char*)buffer, (unsigned long int)sizeof(buffer), p_str, args);
    va_end(args);

    static file_descriptor_t dbg_out_stream_fd = FILE_DESCRIPTOR_INVALID;
    if (dbg_out_stream_fd == FILE_DESCRIPTOR_INVALID && vfs_ptr)
        dbg_out_stream_fd = vfs_ptr->open_file("/dev/dbg");

    auto buffer_array = dynamic_array<uint8_t>();
    buffer_array.resize(256);
    for (size_t i = 0; i < strlen; i++)
        buffer_array.insert_back(buffer[i]);

    switch (mode) {
        case DBG:
            // fallback
            if (dbg_out_stream_fd == FILE_DESCRIPTOR_INVALID || !vfs_ptr) {
                debug_puts((char*)buffer);
                break;
            }
            vfs_ptr->write_file(dbg_out_stream_fd, &buffer_array);
            break;
        case STD:
        default:
            vga_tm_puts(&g_vga_tm_buffer, (char*)buffer);
            break;
    }
}

void exec(const char* path, const char* args) {
    printf(DBG, "executing: %s\n", path);
}

int terminal() {
    static bool keep_running = true;

    dynamic_array<char> current_input {};
    printf(STD, "> ");

    while (keep_running) {
        switch (virtual_key_t vk = wait_for_key()) {
            case VK_ENTER: {
                printf(STD, "\n> ");
                current_input.insert_back(0);

                auto parts = str_split(current_input.get_data(), ' ');

                if (*parts.get_at(0) == "exit") {
                    keep_running = false;
                }

                exec(parts.get_at(0)->c_str(), "");
                current_input.clear();
                break;
            }
            case VK_BACKSPACE:
                if (current_input.length() > 0) {
                    printf(STD, "%c", '\b');
                    current_input.delete_at(current_input.length() - 1);
                }
                break;
            default:
                if (char ch = vk_to_ascii(vk, holding_shift(), holding_caps())) {
                    printf(STD, "%c", ch);
                    current_input.insert_back(ch);
                }
                break;
        }
    }

    return 0;
}

void tcp_callback(const uint8_t* data, size_t size) {
    for (size_t i = 0; i < size; i++)
        printf(DBG, "%c", data[i]);

    printf(DBG, "\n");
}

extern "C" void kernel_entry(void* p_multiboot_struct, void* p_kpml4) {
    // validate multiboot
    if (!mb_has_valid_magic((multiboot_t*)p_multiboot_struct))
        kernel_fatal(KERNEL_FATAL_MULTIBOOT_MAGIC_VALIDATE, "multiboot magic was not valid");

    // parse memory size
    // size_t total_memory_size = 0;
    // for (auto mm_entry = mb_get_first_entry((multiboot_t*)p_multiboot_struct); mm_entry; mm_entry = mb_get_next_entry((multiboot_t*)p_multiboot_struct, mm_entry)) {
    //     // reserve physical pages for reserved memory
    //     if (mm_entry->type == (uint32_t)memory_map_type_t::USABLE) {
    //         total_memory_size += mm_entry->len;
    //     }
    // }

    // initialize the gdt / tss
    gdt_init();

    // initialze vga text mode
    vga_tm_init_buffer(&g_vga_tm_buffer, (void*)VGA_TM_BUFFER_ADDR, VGA_TM_NUM_COLS, VGA_TM_NUM_ROWS);
    vga_tm_clear_buffer(&g_vga_tm_buffer);

    // initialze the debug out stream
    debug_init();

    // initialze virtual memory
    if (!vmem_init(p_multiboot_struct, p_kpml4))
        kernel_fatal(KERNEL_FATAL_VMEM_INIT, "vmem failed to initialize");
    
    vmem_identity_map(p_kpml4);
    set_pml4(p_kpml4);

    // initialze the global heap
    heap_t heap {};
    if (!heap_init(&heap, p_kpml4, (void*)VMEM_HEAP_START_ADDR, HEAP_START_SIZE))
        kernel_fatal(KERNEL_FATAL_HEAP_INIT, "kernel heap fail to initialze");
    
    set_global_heap(&heap);

    // initialze the interrupt line(s)
    set_interrupt_callback(interrupt_t::HARDWARE_PIT, pit_handle_interrupt);
    set_interrupt_callback(interrupt_t::HARDWARE_KEYBOARD, ps2_keyboard_handle_interrupt);
    set_interrupt_callback(interrupt_t::HARDWARE_PS2_MOUSE, ps2_mouse_handle_interrupt);
    set_interrupt_callback(interrupt_t::SOFTWARE_SCHEDULER, vthread_handle_interrupt);

    interrupt_set_handler(handle_interrupt);
    ps2_mouse_init();
    pit_init(PIT_TIMER_INTERVAL);
    interrupt_init(gdt_get_kernel_code_selector());

    virtual_file_system vfs {};
    vfs_ptr = &vfs;
    vfs.create_directory("/mnt");
    vfs.create_directory("/dev");

    vfs.mount("/dev/dbg", ptr::make_unique<vfs_dbg_stream>());

    if (ps2_port_test_device(ps2_device_type_t::KEYBOARD)) {
        printf(DBG, "[+] ps2/keyboard\n");
    }

    if (ps2_port_test_device(ps2_device_type_t::MOUSE)) {
        printf(DBG, "[+] ps2/mouse\n");
    }

    if (vthread_start_and_setup_main() == VTHREAD_HANDLE_INVALID)
        kernel_fatal(KERNEL_FATAL_VHREAD_INIT, "virtual threads failed to intialize");

    pit_add_interrupt_function(vthread_handle_interrupt);

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
    
    e1000_t e1000 {};
    const auto e1000_init_result = e1000_init_device(network_controller, &e1000);
    if (e1000_init_result == 0) {
        printf(DBG, "MAC: %uh:%uh:%uh:%uh:%uh:%uh\n", e1000.mac[0], e1000.mac[1], e1000.mac[2], e1000.mac[3], e1000.mac[4], e1000.mac[5]);

        network_interface_device_t e1000_nid {};
        e1000_nid.name = "eth0 (Intel e1000)";
        // TODO @since 27/08/2025 -- 03:49
        // dhcp :)
        e1000_nid.ip4 = TO_IP(192, 168, 178, 50);
        e1000_nid.is_up = true;
        e1000_nid.device_data = &e1000;
        e1000_nid.gateway_ip = TO_IP(192, 168, 178, 1);
        e1000_nid.subnet_mask = TO_IP(255, 255, 255, 0);
        memcpy(e1000_nid.mac, e1000.mac, sizeof(e1000_nid.mac));
        e1000_nid.send_packet = e1000_nidm_send_packet;
        nidm_register_device(e1000_nid);

        // uint8_t mac[6];
        // uint64_t frame = clock_get_time_since_boot() + 1000;

        // while (!arp_lookup(e1000_nid.gateway_ip, mac)) {
        //     arp_discover_request_ipv4(&e1000_nid, e1000_nid.gateway_ip);
        //     while (frame > clock_get_time_since_boot()) {}
        //     frame = clock_get_time_since_boot() + 1000;
        // }

        // tcp_connect(&e1000_nid, TO_IP(84, 107, 174, 113), 80);
        // uint8_t mac[6];
        // uint64_t frame = clock_get_time_since_boot() + 1000;

        // while (!arp_lookup(TO_IP(192, 168, 178, 219), mac)) {
        //     arp_discover_request_ipv4(&e1000_nid, TO_IP(192, 168, 178, 219));
        //     while (frame > clock_get_time_since_boot()) {}
        //     frame = clock_get_time_since_boot() + 1000;
        // }

        // auto connection = tcp_connect(&e1000_nid, TO_IP(192, 168, 178, 219), 8090);

        // while (connection->state != tcp_state_t::ESTABLISHED);
        
        // const char* http_request = 
        //     "GET / HTTP/1.1\r\n"
        //     "Connection: close\r\n"
        //     "User-Agent: virtual reflections e0\r\n"
        //     "\r\n";
        // printf(DBG, "[HTTP] Sending request:\n%s", http_request);
        // tcp_send_packet(&e1000_nid, (uint8_t*)http_request, strlen(http_request), TCP_FLAG_ACK | TCP_FLAG_PSH, connection);
        tcp_listen(&e1000_nid, 1234, tcp_callback);
    }

    auto disk_storage = ptr::make_unique<vfs_disk_storage>(&mounted_iso9660_fs_instance);
    vfs.mount("/mnt/disk0", move(disk_storage));
    vfs.create_file_cache("/mnt/disk0/INetDrivers.sys");
    vfs.create_file_cache("/mnt/disk0/.env");
    vfs.create_file_cache("/mnt/disk0/media/eva-title-0.bmp");
    vfs.create_file_cache("/mnt/disk0/media/eva-title-0.png");

    // dynamic_array<uint8_t> inet_driver_file {};
    // auto inet_driver_file_handle = vfs.open_file("/mnt/disk0/INetDrivers.sys");
    // vfs.read_file(inet_driver_file_handle, &inet_driver_file);

    // auto handle = driver_load("INetDrivers", inet_driver_file.get_data());
    // driver_start(handle);
    // void* check_driver_function = driver_get_function(handle, "check_driver");
    // typedef bool(*check_driver_function_t)(const char*);
    // auto result = ((check_driver_function_t)check_driver_function)("e1000");
    // printf(DBG, "has e1000 driver: %s\n", result ? "true" : "false");

    // kernel finished
    // printf(STD, "> SYSTEM READY\n");
    printf(DBG, "Kernel finished initializing\n");

    // if (vthread_create(desktop_init) == VTHREAD_HANDLE_INVALID)
    //     printf(DBG, "failed to create desktop thread\n");
    // while (!is_desktop_ready());
    // minesweeper_init();

    uint8_t* smbios_start = (uint8_t*)0x000F0000;
    uint8_t* smbios_end = (uint8_t*)0x000FFFFF;
    const char sig[] { '_', 'S', 'M', '_', };
    uint8_t* smbios_struct = nullptr;

    for (uint8_t* i = smbios_start; i < smbios_end; i += 16) {
        if (memeq(i, sig, ARRAY_SIZE(sig))) {
            smbios_struct = i;
            break;
        }
    }

    printf(DBG, "struct: %p\n", smbios_struct);

    vthread_handle_t vth = vthread_create(terminal, p_kpml4);
    if (vth == VTHREAD_HANDLE_INVALID) {
        printf(DBG, "failed to start terminal");
    } else {
        vthread_wait_for_close(vth);
        printf(DBG, "terminal closed\n");
    }

    // we shoudn t reach this point since the kernel should never stop
    // incase we do just hang here so we dont break anything
    while (true);
}