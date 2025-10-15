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
#include "drivers/driver.hpp"
#include "drivers/storage/ide.hpp"
#include "drivers/storage/ahci.hpp"
#include "drivers/network/e1000.hpp"
#include "drivers/network/nidm.hpp"
#include "drivers/network/tcp.hpp"
#include "drivers/network/arp.hpp"
#include "drivers/network/udp.hpp"

#include "interrupt_manager.hpp"

#include "filesystems/iso9660.hpp"
#include "filesystems/fat32.hpp"
#include "filesystems/vfs.hpp"

#include "memory/vmem.hpp"
#include "memory/heap.hpp"

#include "utils/debug.hpp"
#include "std/array.hpp"
#include "utils/event.hpp"

#include "time/clock.hpp"

#include "gui/desktop.hpp"
#include "gui/games/minesweeper.hpp"

#include "std/random.hpp"

#include "multiboot.hpp"
#include "string.hpp"
#include "common.hpp"
#include "crash_handler.hpp"
#include "virtual_thread.hpp"
#include "system_info.hpp"
#include "smbios.hpp"

#define HEAP_START_SIZE 0x100000 * 32 // 32 mb
#define PIT_TIMER_INTERVAL 1000 // times per second

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

    switch (mode) {
        case DBG:
            debug_puts((char*)buffer);
            break;
        case STD:
        default:
            vga_tm_puts(&g_vga_tm_buffer, (char*)buffer);
            break;
    }
}

void exec(const string& path, const std::dynamic_array<string>& args) {
    switch (hash_fnv1a_64(path.c_str())) {
        case hash_fnv1a_64("memstat"): {
            auto heap = get_global_heap();
            size_t used_mem = 0;
            for (size_t i = 0; i < heap->heap_block_array_size; i++) {
                if (!heap->heap_block_array[i].free)
                    used_mem += heap->heap_block_array[i].size;
            }

            // TODO @since 13/10/2025 -- 12:35
            // add dma stuff & reserved memory for the kernel etc

            printf(STD, "Memory allocated:   %s/%s (%i%)\n", str_format_size(used_mem).c_str(), str_format_size(heap->size).c_str(), (int)(((double)used_mem / (double)heap->size) * 100));
            printf(STD, "Total available:    %s\n", str_format_size(get_global_system_info_manager()->memory_size).c_str());
            printf(STD, "Total comitted:     %f%\n", (double)(((double)heap->size / (double)get_global_system_info_manager()->memory_size) * 100));
            break;
        }
        case hash_fnv1a_64("netstat"): {
            for (auto& device : get_global_nidm()->devices) {
                printf(STD, "%s:\n", device.name.c_str());
                printf(STD, "    MAC:            %uh:%uh:%uh:%uh:%uh:%uh\n", device.mac[0], device.mac[1], device.mac[2], device.mac[3], device.mac[4], device.mac[5]);
                printf(STD, "    IPv4:           %u.%u.%u.%u\n", device.ipv4_3, device.ipv4_2, device.ipv4_1, device.ipv4_0);
                printf(STD, "    Gateway:        %u.%u.%u.%u\n", device.gateway_ip_3, device.gateway_ip_2, device.gateway_ip_1, device.gateway_ip_0);
                printf(STD, "    Subnet mask:    %u.%u.%u.%u\n", device.subnet_mask_3, device.subnet_mask_2, device.subnet_mask_1, device.subnet_mask_0);
            }
            break;
        }
        case hash_fnv1a_64("pcistat"): {
            for (auto& device : get_global_pcie_device_manager()->devices) {
                const char* cd = pci_get_class_description(&device);
                printf(STD, "[%u:%u.%u] %s:\n", device.bus, device.device, device.function, cd);
                printf(STD, "    Vendor ID: 0x%uh, Device ID: 0x%uh\n", device.vendor_device_id.vendor_id, device.vendor_device_id.device_id);
            }
            break;
        }
        case hash_fnv1a_64("ls"): {
            // TODO @since 11/10/2025 -- 01:09
            // check if is file
            std::dynamic_array<vfs_node_t*> entries {};
            string arg_path = "";
            if (args.length() >= 1)
                arg_path = *args.get_at(0);

            bool result = vfs_list_directory(get_global_vfs(), arg_path, &entries);
            if (!result) {
                printf(STD, "Directory not found");
                break;
            }

            for (auto& dir : entries)
                printf(STD, "%s\n", dir->meta.name.c_str());

            break;
        }
        case hash_fnv1a_64("cat"): {
            // TODO @since 11/10/2025 -- 01:09
            // check if is directory
            std::dynamic_array<vfs_node_t*> entries {};
            string arg_path = "";
            if (args.length() >= 1)
                arg_path = *args.get_at(0);

            file_descriptor_t result = vfs_open_file(get_global_vfs(), arg_path);
            if (result == FILE_DESCRIPTOR_INVALID) {
                printf(STD, "File not found");
                break;
            }

            std::dynamic_array<uint8_t> data {};
            vfs_read_file(get_global_vfs(), result, &data);
            for (auto& ch : data) {
                printf(STD, "%c", ch);
            }
            printf(STD, "\n");
            break;
        }
        case hash_fnv1a_64("help"): {
            printf(STD, "help                           Displays this help message\n");
            printf(STD, "memstat                        Memory info\n");
            printf(STD, "netstat                        Network card info\n");
            printf(STD, "pcistat                        PCI(e) info\n");
            printf(STD, "ls                             Lists files and directories\n");
            printf(STD, "cat                            Display file content\n");
            printf(STD, "systemstat                     Display system information\n");
            printf(STD, "diskstat                       Displays disk info\n");
            printf(STD, "    <path>                     Target disk path\n");
            printf(STD, "driverquery                    Query drivers for information\n");
            printf(STD, "    list                       List all drivers\n");
            printf(STD, "    <name> <feature>           List the capabiliy of a driver feature\n");
            break;
        }
        case hash_fnv1a_64("driverquery"): {
            if (args.length() >= 1) {
                auto arg0 = *args.get_at(0);
                if (arg0 == "list") {
                    printf(STD, "List of loaded drivers:\n");
                    for (const auto& driver : get_global_driver_manager()->loaded_drivers)
                        printf(STD, "    %s\n", driver.value->name.c_str());
                    
                    break;
                }

                if (args.length() >= 2) {
                    auto arg1 = *args.get_at(1);
                    system_driver_handle_t handle = driver_manager_get_driver_handle(get_global_driver_manager(), arg0.c_str());
                    if (handle == SYSTEM_DRIVER_HANDLE_INVALID) {
                        printf(STD, "Invalid driver name\n");
                        break;
                    }

                    printf(STD, "%s:\n", arg0.c_str());
                    uint64_t capability = driver_query_capability(get_global_driver_manager(), handle, arg1.c_str());
                    if (capability == MAX_UINT64) {
                        printf(STD, "    Capability: %s not supported", arg1.c_str());
                    } else if (capability == 0) {
                        printf(STD, "    Capability: %s not implemented", arg1.c_str());
                    } else {
                        printf(STD, "    Capability: %s version %u", arg1.c_str(), capability);
                    }
                }
            }
            break;
        }
        case hash_fnv1a_64("diskstat"): {
            if (args.length() >= 1) {
                auto arg0 = *args.get_at(0);
    
                vfs_storage_info_t storage_info {};
                if (!vfs_get_disk_info(get_global_vfs(), arg0.c_str(), &storage_info)) {
                    printf(STD, "Disk or drive not found\n");
                    break;
                }
    
                printf(STD, "%s:\n", storage_info.model.c_str());
                printf(STD, "    Serial: %s\n", storage_info.serial.c_str());
                printf(STD, "    Firmware: %s\n", storage_info.firmare.c_str());
                printf(STD, "    Disk size: %s\n", str_format_size(storage_info.capacity).c_str());
            }
            break;
        }
        case hash_fnv1a_64("systemstat"): {
            system_info_manager_t* sysinfo = get_global_system_info_manager();
            printf(STD, "%s %s %s %s\n", sysinfo->manufacturer.c_str(), sysinfo->product_name.c_str(), sysinfo->version.c_str(), sysinfo->serial_number.c_str());
            break;
        }
        default:
            printf(STD, "command not found");
            break;
    }
}

std::dynamic_array<char> terminal_current_input {};
bool keep_terminal_alive = true;

void terminal_keydown_callback(virtual_key_t vk) {
    switch (vk) {
        case VK_ENTER: {
            printf(STD, "\n");
            terminal_current_input.insert_back(0);

            auto parts = str_split(terminal_current_input.get_data(), ' ');
            std::dynamic_array<string> args {};
            args.resize(parts.length() - 1);
            for (size_t i = 1; i < parts.length(); i++)
                args.insert_back(*parts.get_at(i));

            if (parts.length() == 0) {
                exec("", {});
            } else {
                exec(*parts.get_at(0), args);
            }

            terminal_current_input.clear();
            printf(STD, "\n> ");
            break;
        }
        case VK_BACKSPACE:
            if (terminal_current_input.length() > 0) {
                printf(STD, "%c", '\b');
                terminal_current_input.delete_at(terminal_current_input.length() - 1);
            }
            break;
        case VK_TAB:
            for (size_t i = 0; i < 4; i++) {
                printf(STD, " ");
                terminal_current_input.insert_back(' ');
            }
            break;
        default:
            if (char ch = vk_to_ascii(vk, holding_shift(), holding_caps())) {
                printf(STD, "%c", ch);
                terminal_current_input.insert_back(ch);
            }
            break;
    }
}

int terminal() {
    printf(STD, "VirtualReflectionsOS Interacive Terminal [v0.1:%s]\n", GIT_COMMIT_HASH);
    printf(STD, "System booted succesfully\n");
    printf(STD, "Type 'help' for a list of commands.\n");

    if (!ps2_port_test_device(ps2_device_type_t::KEYBOARD)) {
        printf(STD, "\nNo keyboard found, exiting ...\n");
        return 1;
    }

    printf(STD, "\n> ");
    subscribe_on_key_down(terminal_keydown_callback);
    while (keep_terminal_alive) {}
    return 0;
}

void tcp_callback(const uint8_t* data, size_t size) {
    for (size_t i = 0; i < size; i++)
        printf(DBG, "%c", data[i]);

    printf(DBG, "\n");
}

#include "dhcp.hpp"

void dhcp_callback(uint8_t* packet, size_t size) {
    DHCPPacket* p = (DHCPPacket*)packet;

    system_driver_handle_t driver_handle = driver_manager_get_driver_handle(get_global_driver_manager(), "INetDrivers");
    auto dhcp_client_recieve_fn = (decltype(DHCPClientRecieve)*)driver_get_function(get_global_driver_manager(), driver_handle, "DHCPClientRecieve");
    
    auto device = nidm_get_device(get_global_nidm(), "eth0 (Intel e1000)");

    uint32_t ip;
    uint32_t subnet_mask;
    uint32_t gateway;
    uint32_t dhcp_server_id;
    int result = dhcp_client_recieve_fn(p, p->m_nXID, &ip, &subnet_mask, &gateway, &dhcp_server_id);

    if (result == DHCP_CLIENT_RECIEVE_ACK) {
        device->ip4 = ip;
        device->subnet_mask = subnet_mask;
        device->gateway_ip = gateway;
        printf(DBG, "dhcp ack\n");
        return;
    }

    if (result == DHCP_CLIENT_RECIEVE_REQ) {
        auto dhcp_create_request_packet_fn = (decltype(DHCPCreateRequestPacket)*)driver_get_function(get_global_driver_manager(), driver_handle, "DHCPCreateRequestPacket");
        auto request = dhcp_create_request_packet_fn("test!", ip, dhcp_server_id, p->m_nXID, device->mac);
        device->ip4 = ip;
        device->subnet_mask = subnet_mask;
        device->gateway_ip = gateway;
        udp_send(device, dhcp_server_id, DHCP_PORT_CLIENT, DHCP_PORT_SERVER, (uint8_t*)&request, sizeof(DHCPPacket));
        printf(DBG, "sending dhcp request\n");
        return;
    }

    printf(DBG, "dhcp unkown\n");
}

void dhcp_client(network_interface_device_t* device) {
    system_driver_handle_t driver_handle = driver_manager_get_driver_handle(get_global_driver_manager(), "INetDrivers");
    auto dhcp_client_discover_fn = (decltype(DHCPClientDiscover)*)driver_get_function(get_global_driver_manager(), driver_handle, "DHCPClientDiscover");

    printf(DBG, "sending dhcp discover\n");
    auto xid = dhcp_client_discover_fn("hostname", device->mac);
    nidm_udp_bind(get_global_nidm(), DHCP_PORT_CLIENT, dhcp_callback);
}

extern "C" void kernel_entry(void* p_multiboot_struct, void* p_kpml4) {
    // validate multiboot
    if (!mb_has_valid_magic((multiboot_t*)p_multiboot_struct))
        kernel_fatal(KERNEL_FATAL_MULTIBOOT_MAGIC_VALIDATE, "multiboot magic was not valid");

    // initialize the gdt / tss
    gdt_init();

    // initialze vga text mode
    // TODO @since 10/10/2025 -- 01:24
    // vga (device) manager
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

    // initialize threading
    if (vthread_start_and_setup_main() == VTHREAD_HANDLE_INVALID)
        kernel_fatal(KERNEL_FATAL_VHREAD_INIT, "virtual threads failed to intialize");

    pit_add_interrupt_function(vthread_handle_interrupt);

    system_info_manager_t sim {};
    set_global_system_info_manager(&sim);
    system_info_parse_memory_size(get_global_system_info_manager(), (multiboot_t*)p_multiboot_struct);
    system_info_parse_system_information(get_global_system_info_manager());

    vfs_t vfs {};
    vfs_init(&vfs);
    set_global_vfs(&vfs);
    vfs_create_directory(get_global_vfs(), "/mnt");
    vfs_create_directory(get_global_vfs(), "/dev");

    // finished core startup

    keyboard_initialize();

    nidm_t nidm {};
    nidm_init(&nidm);
    set_global_nidm(&nidm);

    pcie_device_manager_t pciedm {};
    set_global_pcie_device_manager(&pciedm);
    
    pci_enumerate_devices(get_global_pcie_device_manager());

    // ide device
    pci_class_info_t ide_device_class_info {
        .revision_id = (uint8_t)PCI_UNKNOWN,
        .prog_if = (uint8_t)PCI_UNKNOWN,
        .sub_class = (uint8_t)1,
        .class_code = (uint8_t)1
    };
    const pci_device_t* ide_controller = pci_find_device(get_global_pcie_device_manager(), &ide_device_class_info);

    linked_list<ide_device_t> ide_devices {};
    ide_init(ide_controller, &ide_devices);
    
    size_t ide_device_index = 0;
    for (auto& drive : ide_devices) {
        // init device interface
        auto ide_storage = std::make_unique<ide_storage_driver_t>(&drive);

        char disk_mount_name_buffer[20];
        sprintf(disk_mount_name_buffer, 20, "/mnt/disk%i", ide_device_index++);

        if (!mount_disk(move(ide_storage), disk_mount_name_buffer)) {
            printf(DBG, "failed to mount disk: %s\n", disk_mount_name_buffer);
        } else {
            printf(DBG, "mounted: %s\n", disk_mount_name_buffer);
        }
    }

    // ahci device
    pci_class_info_t ahci_device_class_info { .revision_id = (uint8_t)PCI_UNKNOWN, .prog_if = (uint8_t)1, .sub_class = (uint8_t)6, .class_code = (uint8_t)1 };
    const pci_device_t* ahci_controller = pci_find_device(get_global_pcie_device_manager(), &ahci_device_class_info);

    linked_list<ahci_drive_t> ahci_devices {};
    ahci_init(ahci_controller, &ahci_devices);

    size_t ahci_device_index = 0;
    for (auto& drive : ahci_devices) {
        if (drive.was_setup) {
            auto ahci_storage = std::make_unique<ahci_storage_driver_t>(&drive);

            char disk_mount_name_buffer[20];
            sprintf(disk_mount_name_buffer, 20, "/mnt/drive%i", ahci_device_index++);

            if (!mount_disk(move(ahci_storage), disk_mount_name_buffer)) {
                printf(DBG, "failed to mount drive: %s\n", disk_mount_name_buffer);
            } else {
                printf(DBG, "mounted: %s\n", disk_mount_name_buffer);
            }
        }
    }

    // network device
    pci_class_info_t network_device_class_info { .revision_id = (uint8_t)PCI_UNKNOWN, .prog_if = (uint8_t)PCI_UNKNOWN, .sub_class = (uint8_t)0, .class_code = (uint8_t)2 };
    const pci_device_t* network_controller = pci_find_device(get_global_pcie_device_manager(), &network_device_class_info);

    e1000_t e1000 {};
    network_interface_device_t e1000_nid {};
    const auto e1000_init_result = e1000_init_device(network_controller, &e1000);
    if (e1000_init_result == 0) {
        e1000_nid.name = "eth0 (Intel e1000)";
        // TODO @since 27/08/2025 -- 03:49
        // dhcp :)
        // e1000_nid.ip4 = TO_IP(192, 168, 178, 50);
        // e1000_nid.ip4 = TO_IP(10, 0, 2, 2);
        e1000_nid.ip4 = TO_IP(0, 0, 0, 0);
        e1000_nid.is_up = true;
        e1000_nid.device_data = &e1000;
        // e1000_nid.gateway_ip = TO_IP(192, 168, 178, 1);
        e1000_nid.gateway_ip = TO_IP(10, 0, 2, 1);
        e1000_nid.subnet_mask = TO_IP(255, 255, 255, 0);
        memcpy(e1000_nid.mac, e1000.mac, sizeof(e1000_nid.mac));
        e1000_nid.send_packet = e1000_nidm_send_packet;
        nidm_register_device(get_global_nidm(), e1000_nid);

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

    driver_manager_t driver_manager {};
    set_global_driver_manager(&driver_manager);

    std::dynamic_array<vfs_node_t*> nodes {};
    if (vfs_list_directory(get_global_vfs(), "/mnt/disk0", &nodes)) {
        for (auto& node : nodes) {
            if (str_ends_with(node->meta.name.c_str(), ".sys")) {
                string driver_name = node->meta.name.substr(0, node->meta.name.length() - 4);

                std::dynamic_array<uint8_t> driver_file {};
                file_descriptor_t driver_file_handle = vfs_open_file(get_global_vfs(), string("/mnt/disk0/") + driver_name + ".sys");
                if (driver_file_handle == FILE_DESCRIPTOR_INVALID) {
                    printf(DBG, "failed to open handle to driver '%s'\n", driver_name.c_str());
                    continue;
                }

                if (!vfs_read_file(get_global_vfs(), driver_file_handle, &driver_file)) {
                    printf(DBG, "failed to read driver '%s'\n", driver_name.c_str());
                    continue;
                }

                system_driver_handle_t driver_handle = driver_load(get_global_driver_manager(), driver_name.c_str(), driver_file.get_data());
                if (driver_handle == SYSTEM_DRIVER_HANDLE_INVALID) {
                    printf(DBG, "failed to load driver '%s'\n", driver_name.c_str());
                    continue;
                }

                int result = driver_start(get_global_driver_manager(), driver_handle);
                if (result != 0) {
                    printf(DBG, "failed to start driver '%s'. code: %i\n", driver_name.c_str(), result);
                    continue;
                }

                printf(DBG, "loaded driver '%s'. code: %i\n", driver_name.c_str(), result);
            }
        }
    }

    dhcp_client(&e1000_nid);

    // kernel finished
    // printf(STD, "> SYSTEM READY\n");
    printf(DBG, "Kernel finished initializing\n");

    // if (vthread_create(desktop_init, p_kpml4) == VTHREAD_HANDLE_INVALID)
    //     printf(DBG, "failed to create desktop thread\n");
    
    // while (!is_desktop_ready());
    // minesweeper_init();

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