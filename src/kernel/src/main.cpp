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
#include "drivers/network/dns.hpp"

#include "subsystem_interface.hpp"
#include "subsystems/dhcp/interface.hpp"
#include "subsystems/dhcp/dhcp_client_driver.hpp"
#include "subsystems/dns/interface.hpp"
#include "subsystems/dns/dns_client_driver.hpp"

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
#include "gui/programs/minesweeper.hpp"

#include "std/random.hpp"

#include "multiboot.hpp"
#include "std/string.hpp"
#include "common.hpp"
#include "crash_handler.hpp"
#include "virtual_thread.hpp"
#include "system_info.hpp"
#include "smbios.hpp"
#include "io.hpp"
#include "terminal.hpp"

#define HEAP_START_SIZE 0x100000 * 32 // 32 mb
#define PIT_TIMER_INTERVAL 1000 // times per second
#define DEVICE_HOST_NAME "VirtualReflections Host"

extern "C" void kernel_entry(void* p_multiboot_struct, void* p_kpml4) {
    // validate multiboot
    if (mb_has_valid_magic((multiboot_t*)p_multiboot_struct) != 2)
        kernel_fatal(KERNEL_FATAL_MULTIBOOT_MAGIC_VALIDATE, "multiboot magic was not the excpected version");

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
    if (!vmem_init(p_kpml4, p_multiboot_struct))
        kernel_fatal(KERNEL_FATAL_VMEM_INIT, "vmem failed to initialize");

    // initialze the global heap
    heap_t heap {};
    if (!heap_init(&heap, p_kpml4, (void*)VMEM_KERNEL_HEAP_START, HEAP_START_SIZE))
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

    dma_heap_manager_t allocator {};
    set_global_dma_heap_manager(&allocator);
    dma_heap_manager_init(get_global_dma_heap_manager(), p_kpml4, (void*)VMEM_DMA_ALLOCATOR_START, PAGE_SIZE_LARGE * 128);
    
    vfs_t vfs {};
    vfs_init(&vfs);
    set_global_vfs(&vfs);
    // vfs_create_directory(get_global_vfs(), "/mnt");

    // initialize threading
    if (vthread_start_and_setup_main() == VTHREAD_HANDLE_INVALID)
        kernel_fatal(KERNEL_FATAL_VTHREAD_INIT, "virtual threads failed to intialize");

    pit_add_interrupt_function(vthread_handle_interrupt);

    system_info_manager_t sim {};
    set_global_system_info_manager(&sim);
    system_info_parse_memory_size(get_global_system_info_manager(), (multiboot_t*)p_multiboot_struct);
    system_info_parse_system_information(get_global_system_info_manager());
    system_info_get_cpu_name(get_global_system_info_manager());

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
        sprintf(disk_mount_name_buffer, 20, "/disk%i", ide_device_index++);

        if (!mount_disk(move(ide_storage), disk_mount_name_buffer)) {
            kprintf("failed to mount disk: %s\n", disk_mount_name_buffer);
        } else {
            kprintf("mounted: %s\n", disk_mount_name_buffer);
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
            sprintf(disk_mount_name_buffer, 20, "/drive%i", ahci_device_index++);

            if (!mount_disk(move(ahci_storage), disk_mount_name_buffer)) {
                kprintf("failed to mount drive: %s\n", disk_mount_name_buffer);
            } else {
                kprintf("mounted: %s\n", disk_mount_name_buffer);
            }
        }
    }

    driver_manager_t driver_manager {};
    set_global_driver_manager(&driver_manager);

    std::dynamic_array<vfs_node_t*> nodes {};
    if (vfs_list_directory(get_global_vfs(), "/disk0", &nodes)) {
        for (auto& node : nodes) {
            if (str_ends_with(node->meta.name.c_str(), ".sys")) {
                std::string driver_name = node->meta.name.substr(0, node->meta.name.length() - 4);

                std::dynamic_array<uint8_t> driver_file {};
                file_descriptor_t driver_file_handle = vfs_open_file(get_global_vfs(), std::string("/disk0/") + driver_name + ".sys");
                if (driver_file_handle == FILE_DESCRIPTOR_INVALID) {
                    kprintf("failed to open handle to driver '%s'\n", driver_name.c_str());
                    continue;
                }

                if (!vfs_read_file(get_global_vfs(), driver_file_handle, &driver_file)) {
                    kprintf("failed to read driver '%s'\n", driver_name.c_str());
                    continue;
                }

                system_driver_handle_t driver_handle = driver_load(get_global_driver_manager(), driver_name.c_str(), driver_file.get_data());
                if (driver_handle == SYSTEM_DRIVER_HANDLE_INVALID) {
                    kprintf("failed to load driver '%s'\n", driver_name.c_str());
                    continue;
                }

                int result = driver_start(get_global_driver_manager(), driver_handle);
                if (result != 0) {
                    kprintf("failed to start driver '%s'. code: %i\n", driver_name.c_str(), result);
                    continue;
                }

                kprintf("loaded driver '%s'. code: %i\n", driver_name.c_str(), result);
            }
        }
    }

    const system_driver_handle_t inet_driver_handle = driver_manager_get_driver_handle(get_global_driver_manager(), "INetDrivers");
    if (inet_driver_handle != SYSTEM_DRIVER_HANDLE_INVALID) {
        // start our driver as the dhcp subsystem
        if (driver_query_capability(get_global_driver_manager(), inet_driver_handle, "dhcp") >= 1)
            subsys_init(SUBSYS_DHCP_CLIENT, std::make_unique<subsys_dhcp_client_driver_t>(DEVICE_HOST_NAME));

        // start our driver as the dhcp subsystem
        if (driver_query_capability(get_global_driver_manager(), inet_driver_handle, "dns") >= 1)
            subsys_init(SUBSYS_DNS_CLIENT, std::make_unique<subsys_dns_client_driver_t>());
    }

    // network device
    pci_class_info_t network_device_class_info { .revision_id = (uint8_t)PCI_UNKNOWN, .prog_if = (uint8_t)PCI_UNKNOWN, .sub_class = (uint8_t)0, .class_code = (uint8_t)2 };
    const pci_device_t* network_controller = pci_find_device(get_global_pcie_device_manager(), &network_device_class_info);

    e1000_t e1000 {};
    const auto e1000_init_result = e1000_init_device(network_controller, &e1000);
    if (e1000_init_result == 0) {
        std::unique_ptr<e1000_nid_t> e1000_nid = std::make_unique<e1000_nid_t>(e1000);
        e1000_nid->is_up = true;
        e1000_nid->is_prefered = true;
        e1000_nid->interface = "eth0";
        e1000_nid->is_configured = false;
        nidm_register_device(get_global_nidm(), move(e1000_nid));

        auto subsystem_dhcp_client = subsys_get<subsys_dhcp_client_t>(SUBSYS_DHCP_CLIENT);
        subsystem_dhcp_client->configure(nidm_get_device_on_interface(get_global_nidm(), "eth0"));
    }

    auto net_test = []() {
        auto tcp_callback = [](const uint8_t* data, size_t size) {
            char* str = (char*)malloc(size + 1);
            memzero(str, size + 1);
            memcpy(str, data, size);
            kprintf(str);
            kprintf("\n");
            free(str);
        };

        const auto subsys_dns_client = subsys_get<subsys_dns_client_t>(SUBSYS_DNS_CLIENT);

        while (!nidm_get_prefered_device(get_global_nidm())->is_configured);
        while (!subsys_dns_client->is_configured());

        auto ip = subsys_dns_client->resolve("cactoes.xyz");
        auto conn = tcp_connect(ip, 80, tcp_callback);

        if (conn->state != tcp_state_t::ESTABLISHED) {
            kprintf("failed to make tcp connection\n");
            return 1;
        }

        const char* http_request = 
            "GET / HTTP/1.1\r\n"
            "Host: cactoes.xyz\r\n"
            "Connection: close\r\n"
            "User-Agent: virtual reflections e0\r\n"
            "\r\n";
        kprintf("[HTTP] Sending request:\n%s", http_request);
        tcp_send_packet((uint8_t*)http_request, strlen(http_request), TCP_FLAG_ACK | TCP_FLAG_PSH, conn);
        return 0;
    };

    // vthread_create(net_test, p_kpml4);

    // kernel finished
    kprintf("kernel finished initializing\n");

    // if (vthread_create(desktop_init, p_kpml4) == VTHREAD_HANDLE_INVALID)
    //     kprintf("failed to create desktop thread\n");
    
    // while (!is_desktop_ready());
    // minesweeper_init();

    vthread_handle_t vth = vthread_create(terminal_thread_main, p_kpml4);
    if (vth == VTHREAD_HANDLE_INVALID)
        kprintf("failed to start terminal");

    const vthread_handle_t critical_threads[] = {
        vthread_create([]() { while (true) nidm_process_packet(); return 1; }, p_kpml4),
        vthread_create([]() { while (true) ps2_mouse_process_packet(); return 1; }, p_kpml4),
        vthread_create([]() { while (true) ps2_keyboard_process_packet(); return 1; }, p_kpml4)
    };

    // make sure the critical threads are not dying
    // this also checks if any of the threads are actaully valid incase the startup fails
    while (vthread_get_count() > 1) {
        for (const auto handle : critical_threads) {
            if (!vthread_get(handle))
                kernel_fatal(KERNEL_FATAL_CRITICAL_THREAD_DIED, "critical thread died!");
        }

        vthread_sleep(1);
    }

    kernel_fatal(KERNEL_FATAL_KERNEL_EXITED, "all kernel processes ended!");

    // we shoudn t reach this point since the kernel should never stop
    // incase we do just hang here so we dont break anything
    while (true);
}