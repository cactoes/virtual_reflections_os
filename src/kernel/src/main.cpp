#include "arch/generic.hpp"
#include "arch/gdt.hpp"
#include "arch/interrupt.hpp"
#include "arch/pit.hpp"
#include "arch/msr.hpp"

#include "drivers/vga.hpp"
#include "drivers/pcie.hpp"
#include "drivers/pit.hpp"
#include "drivers/keyboard.hpp"
#include "drivers/ps2/keyboard.hpp"
#include "drivers/ps2/mouse.hpp"
#include "drivers/ps2/ps2.hpp"
#include "drivers/driver.hpp"
#include "drivers/network/e1000.hpp"
#include "network/nidm.hpp"
#include "network/tcp.hpp"
#include "network/arp.hpp"
#include "network/udp.hpp"
#include "network/network_manager.hpp"

#include "interrupt_manager.hpp"

#include "drivers/storage/ide.hpp"
#include "drivers/storage/ahci.hpp"
#include "drivers/storage/block_device.hpp"
#include "filesystems/vfs.hpp"
#include "drivers/storage/mbr.hpp"
#include "filesystems/iso9660.hpp"
#include "filesystems/fat32.hpp"
#include "storage/storage_manager.hpp"

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
#include "linker.hpp"
#include "cpu.hpp"
#include "memory/paging.hpp"
#include "elf.hpp"

#include "network/socket.hpp"
#include "network/dhcp.hpp"
#include "network/dns.hpp"
#include "network/ip.hpp"

#define HEAP_START_SIZE 0x100000 * 32 // 32 mb
#define PIT_TIMER_INTERVAL 1000 // times per second

// void tcp_socket(socket_t*, uint32_t ip, uint16_t port, const uint8_t* packet, size_t size) {
//     char* data = (char*)malloc(size + 1);
//     memzero(data, size + 1);
//     memcpy(data, packet, size);

//     kprintf(data);
//     kprintf("\n");
// }

// void do_tcp() {
//     socket_t socket {};
//     socket.local_ip = TO_IP(0, 0, 0, 0);
//     socket.local_port = random_number(49152, 65535);
//     socket.remote_ip = TO_IP(10, 0, 2, 2);
//     socket.remote_port = 80;
//     socket.protocol = socket_protocol_t::TCP;
//     socket.listener = tcp_socket;
//     socket_bind(&socket);

//     const char http_request[] = "GET / HTTP/1.0\r\n\r\n";
//     socket_send(&socket, (const uint8_t*)http_request, sizeof(http_request) - 1);

//     while (true) {}
// }

void init_pci_devices(const pci_device_t* device) {
    if (is_e1000_device(device)) {
        e1000_t* e1000 = (e1000_t*)malloc(sizeof(e1000_t));
        memzero(e1000, sizeof(e1000_t));
        if (e1000_init_device(device, e1000) == 0) {
            network_interface_t* e1000_network_interface = (network_interface_t*)malloc(sizeof(network_interface_t));
            memzero(e1000_network_interface, sizeof(network_interface_t));
            
            e1000_network_interface->device = e1000;
            e1000_network_interface->device_type = network_interface_device_type_t::E1000;
            e1000_network_interface->is_configured = true;

            memcpy(e1000_network_interface->mac, e1000->mac, 6);

            const char* device_name = "Intel E1000";
            strncpy(e1000_network_interface->device_name, device_name, sizeof(e1000_network_interface->device_name));

            // for now force this device to be prefered
            e1000_network_interface->is_prefered = true;

            nic_register_interface(get_global_nic(), e1000_network_interface);

            network_manager_configre_interface(get_global_network_manager(), e1000_network_interface);
        }

        // valid device so we can continue to the next device
        return;
    }

    if (is_ide_device(device)) {
        auto& ide_devices = get_global_storage_manager()->ide.devices;
        if (ide_init(device, &ide_devices)) {
            size_t ide_device_index = 0;
            for (auto& device : ide_devices) {
                char name[18];
                sprintf(name, sizeof(name), "harddisk%i", ide_device_index++);
                if (!vfs_mount_device(get_global_vfs(), &device, block_device_type_t::IDE, name)) {
                    kprintf("[IDE] failed to mount drive: %s\n", name);
                } else {
                    kprintf("[IDE] mounted: %s\n", name);
                }
            }
        } else {
            kprintf("[IDE] driver failed to init\n");
        }

        // valid device so we can continue to the next device
        return;
    }

    if (is_ahci_device(device)) {
        auto& ahci_driver_ctx = get_global_storage_manager()->ahci.driver_ctx;
        auto& ahci_devices = get_global_storage_manager()->ahci.devices;
        if (ahci_init(device, &ahci_driver_ctx, &ahci_devices)) {
            size_t ahci_device_index = 0;
            for (auto& device : ahci_devices) {
                char name[18];
                sprintf(name, sizeof(name), "drive%i", ahci_device_index++);
                if (!vfs_mount_device(get_global_vfs(), &device, block_device_type_t::AHCI, name)) {
                    kprintf("failed to mount drive: %s\n", name);
                } else {
                    kprintf("[AHCI] mounted: %s\n", name);
                }
            }
        } else {
            kprintf("[AHCI] driver failed to init\n");
        }

        // valid device so we can continue to the next device
        return;
    }
};

extern "C" uint64_t syscall_dispatch(uint64_t syscall_num, syscall_regs_t* regs) {
    if (syscall_num == 0) {
        kprintf("[ SYSCALL ] terminated process\n");
        vthread_terminate();

        // safetey catch
        while (true);
    }

    switch (syscall_num) {
        case 1: return (uint64_t)heap_alloc(&get_current_process()->heap, regs->rdi);
        case 2: {
            heap_free(&get_current_process()->heap, (void*)regs->rdi);
            return 0;
        }
        default:
            break;
    }

    kprintf("[ \033[91mSYSCALL\033[0m ] unhandled syscall = %ul\n", syscall_num);
    return 0;
}

NORETURN void virtual_kernel_entry(multiboot_t* multiboot_struct, void* kernel_pt_vaddr) {
    // initialze vga text mode
    // TODO @since 10/10/2025 -- 01:24
    // vga (device) manager
    vga_tm_init_buffer(&g_vga_tm_buffer, (void*)VGA_TM_BUFFER_ADDR, VGA_TM_NUM_COLS, VGA_TM_NUM_ROWS);
    vga_tm_clear_buffer(&g_vga_tm_buffer);

    // initialze the debug out stream
    debug_init();

    // validate multiboot
    if (mb_has_valid_magic(multiboot_struct) != MULTIBOOT_VER2)
        kernel_fatal(KERNEL_FATAL_MULTIBOOT_MAGIC_VALIDATE, "multiboot was not the excpected version");

    kprintf("[ \033[92mOK\033[0m ] multiboot validated\n");
    printf("[ \033[92mOK\033[0m ] multiboot validated\n");

    // initialize the gdt / tss
    gdt_init();

    kprintf("[ \033[92mOK\033[0m ] installed gdt / tss\n");
    printf("[ \033[92mOK\033[0m ] installed gdt / tss\n");

    // we already need system info here ...
    // just make sure we dont use the string's yet since memory is not setup yet
    system_info_manager_t sim {};
    set_global_system_info_manager(&sim);
    system_info_parse_memory_size(get_global_system_info_manager(), multiboot_struct);

    kprintf("[ \033[92mOK\033[0m ] parsed memory: %ul\n", sim.memory_size);
    printf("[ \033[92mOK\033[0m ] parsed memory: %ul\n", sim.memory_size);

    // same as the assembly
    const uint64_t kernel_page_count = (LINKER_END_KERNEL_PHYS + (PAGE_SIZE_LARGE - 1)) >> 21;
    for (uint64_t i = 0; i < kernel_page_count; i++)
        vmem_unmap_2mb(kernel_pt_vaddr, (void*)(i * PAGE_SIZE_LARGE));

    // initialze virtual memory
    if (!vmem_init(kernel_pt_vaddr, multiboot_struct))
        kernel_fatal(KERNEL_FATAL_VMEM_INIT, "vmem failed to initialize");

    // initialze the global heap
    heap_t heap {};
    if (!heap_init(&heap, kernel_pt_vaddr, (void*)VMEM_KERNEL_HEAP_START, HEAP_START_SIZE))
        kernel_fatal(KERNEL_FATAL_HEAP_INIT, "kernel heap fail to initialze");

    set_global_heap(&heap);

    void* kernel_pt_paddr = vmem_virtual_to_physical(kernel_pt_vaddr);

    kprintf("[ \033[92mOK\033[0m ] initialized memory\n");
    printf("[ \033[92mOK\033[0m ] initialized memory\n");

    // initialze the interrupt line(s)
    set_interrupt_hook(interrupt_t::HARDWARE_PIT, pit_handle_interrupt, nullptr);
    set_interrupt_hook(interrupt_t::HARDWARE_KEYBOARD, ps2_keyboard_handle_interrupt, nullptr);
    set_interrupt_hook(interrupt_t::HARDWARE_PS2_MOUSE, ps2_mouse_handle_interrupt, nullptr);
    set_interrupt_hook(interrupt_t::SOFTWARE_SCHEDULER, vthread_handle_interrupt, nullptr);

    interrupt_set_handler((void *(*)(uint64_t, void *))handle_interrupt);
    ps2_mouse_init();
    pit_init(PIT_TIMER_INTERVAL);
    interrupt_init(gdt_get_kernel_code_selector());

    initialize_cpus();

    dma_heap_manager_t allocator {};
    set_global_dma_heap_manager(&allocator);
    dma_heap_manager_init(get_global_dma_heap_manager(), kernel_pt_vaddr, (void*)VMEM_DMA_ALLOCATOR_START, PAGE_SIZE_LARGE * 128);

    // initialize threading
    if (vthread_start_and_setup_main() == VTHREAD_HANDLE_INVALID)
        kernel_fatal(KERNEL_FATAL_VTHREAD_INIT, "virtual threads failed to intialize");

    kprintf("[ \033[92mOK\033[0m ] enabled virtual threading\n");
    printf("[ \033[92mOK\033[0m ] enabled virtual threading\n");

    pit_add_interrupt_function(vthread_handle_interrupt);

    system_info_parse_system_information(get_global_system_info_manager());
    system_info_get_cpu_name(get_global_system_info_manager());

    // TODO @since 06/02/2026 -- 10:34
    // proper ps2 startup etc
    keyboard_initialize();

    network_interface_controller_t nic {};
    nic_init(&nic);
    set_global_nic(&nic);

    network_manager_t network_manager {};
    network_manager_init(&network_manager);
    set_global_network_manager(&network_manager);
    if (network_manager_configure(&network_manager)) {
        kprintf("[ \033[92mOK\033[0m ] configured network manager\n");
        printf("[ \033[92mOK\033[0m ] configured network manager\n");
    } else {
        kprintf("[ \033[91mERROR\033[0m ] failed to configure network manager\n");
        printf("[ \033[91mERROR\033[0m ] failed to configure network manager\n");
    }

    vfs_t vfs {};
    vfs_init(&vfs);
    set_global_vfs(&vfs);

    storage_manager_t storage_manager {};
    storage_manager_init(&storage_manager);
    set_global_storage_manager(&storage_manager);

    pcie_device_manager_t pciedm {};
    set_global_pcie_device_manager(&pciedm);
    pci_enumerate_devices(get_global_pcie_device_manager());
    pci_loop_devices(get_global_pcie_device_manager(), init_pci_devices);

    driver_manager_t driver_manager {};
    set_global_driver_manager(&driver_manager);

    // std::dynamic_array<vfs_node_t> nodes {};
    // if (vfs_list_directory(&vfs, "/harddisk0", &nodes)) {
    //     for (auto& node : nodes) {
    //         if (str_ends_with(node.name.c_str(), ".sys")) {
    //             std::string driver_name = node.name.substr(0, node.name.length() - 4);

    //             const std::string driver_path = std::string("/harddisk0/") + driver_name + ".sys";
    //             file_descriptor_t driver_file_handle = vfs_open_file(&vfs, driver_path.c_str());
    //             if (driver_file_handle == FILE_DESCRIPTOR_INVALID) {
    //                 kprintf("[ \033[91mERROR\033[0m ] failed open file handle to driver: '%s'\n", driver_name.c_str());
    //                 printf("[ \033[91mERROR\033[0m ] failed open file handle to driver: '%s'\n", driver_name.c_str());
    //                 continue;
    //             }

    //             // DONT FREE IT!
    //             uint8_t* driver_file_data = nullptr;
    //             size_t size;
    //             if (!vfs_read_file(&vfs, driver_file_handle, &driver_file_data, &size)) {
    //                 kprintf("[ \033[91mERROR\033[0m ] failed to parse driver file '%s'\n", driver_name.c_str());
    //                 printf("[ \033[91mERROR\033[0m ] failed to parse driver file '%s'\n", driver_name.c_str());
    //                 continue;
    //             }

    //             system_driver_handle_t driver_handle = driver_load(get_global_driver_manager(), driver_name.c_str(), driver_file_data);
    //             if (driver_handle == SYSTEM_DRIVER_HANDLE_INVALID) {
    //                 kprintf("[ \033[91mERROR\033[0m ] failed to load driver '%s'\n", driver_name.c_str());
    //                 printf("[ \033[91mERROR\033[0m ] failed to load driver '%s'\n", driver_name.c_str());
    //                 free(driver_file_data);
    //                 continue;
    //             }

    //             int result = driver_start(get_global_driver_manager(), driver_handle);
    //             if (result != 0) {
    //                 kprintf("[ \033[91mERROR\033[0m ] failed to start driver '%s'. code: %i\n", driver_name.c_str(), result);
    //                 printf("[ \033[91mERROR\033[0m ] failed to start driver '%s'. code: %i\n", driver_name.c_str(), result);
    //                 free(driver_file_data);
    //                 continue;
    //             }

    //             kprintf("[ \033[92mOK\033[0m ] loaded driver '%s'. code: %i\n", driver_name.c_str(), result);
    //             printf("[ \033[92mOK\033[0m ] loaded driver '%s'. code: %i\n", driver_name.c_str(), result);
    //         }
    //     }
    // }

    // auto net_test = []() {
    //     auto tcp_callback = [](const uint8_t* data, size_t size) {
    //         char* str = (char*)malloc(size + 1);
    //         memzero(str, size + 1);
    //         memcpy(str, data, size);
    //         kprintf(str);
    //         kprintf("\n");
    //         free(str);
    //     };

    //     const auto subsys_dns_client = subsys_get<subsys_dns_client_t>(SUBSYS_DNS_CLIENT);

    //     while (!nidm_get_prefered_device(get_global_nidm())->is_configured);
    //     while (!subsys_dns_client->is_configured());

    //     auto ip = subsys_dns_client->resolve("cactoes.xyz");
    //     auto conn = tcp_connect(ip, 80, tcp_callback);

    //     if (conn->state != tcp_state_t::ESTABLISHED) {
    //         kprintf("failed to make tcp connection\n");
    //         return 1;
    //     }

    //     const char* http_request = 
    //         "GET / HTTP/1.1\r\n"
    //         "Host: cactoes.xyz\r\n"
    //         "Connection: close\r\n"
    //         "User-Agent: virtual reflections e0\r\n"
    //         "\r\n";
    //     kprintf("[HTTP] Sending request:\n%s", http_request);
    //     tcp_send_packet((uint8_t*)http_request, strlen(http_request), TCP_FLAG_ACK | TCP_FLAG_PSH, conn);
    //     return 0;
    // };

    // vthread_create(net_test, kernel_pt_paddr);

    // if (vthread_create(desktop_init, p_kpml4) == VTHREAD_HANDLE_INVALID)
    //     kprintf("failed to create desktop thread\n");
    
    // while (!is_desktop_ready());
    // minesweeper_init();

    // if (vthread_create(desktop_init, p_kpml4) == VTHREAD_HANDLE_INVALID)
    //     printf("Failed start graphical environment\n");

    // desktop_init();

    const vthread_handle_t critical_threads[] = {
        vthread_create(nic_thread, kernel_pt_paddr, "network interface controller"),
        vthread_create([]() { while (true) ps2_mouse_process_packet(); return 1; }, kernel_pt_paddr, "PS/2 Mouse"),
        vthread_create([]() { while (true) ps2_keyboard_process_packet(); return 1; }, kernel_pt_paddr, "PS/2 Keyboard")
    };

    for (const auto& thread : critical_threads)
        vthread_set_critical(thread, true);

    // kernel finished
    kprintf("[ KERNEL SETUP FINISHED ]\n");
    printf("[ KERNEL SETUP FINISHED ]\n");

    if (vthread_create(terminal_thread_main, kernel_pt_paddr) == VTHREAD_HANDLE_INVALID) {
        kprintf("[ \033[91mERROR\033[0m ] failed to start terminal\n");
        printf("[ \033[91mERROR\033[0m ] failed to start terminal\n");
    }

    // we shoudn t reach this point since the kernel should never stop
    // incase we do just hang here so we dont break anything
    while (true) vthread_sleep(1);
}

extern "C" void kernel_entry(void* multiboot_struct, void* kernel_page_table) {
    vmem_recusive_map_page_table(kernel_page_table, kernel_page_table);
    reload_page_table();

    uint64_t new_stack;
    asm volatile (
        "mov %%rsp, %0\n\t"
        "add %1, %0\n\t"
        "mov %0, %%rsp"
        : "=&r"(new_stack)
        : "r"(KERNEL_VIRTUAL_BASE)
        : "memory"
    );

    ((multiboot_t*)multiboot_struct)->info = (void*)PTOV_I(((multiboot_t*)multiboot_struct)->info);
    virtual_kernel_entry((multiboot_t*)PTOV_I(multiboot_struct), (void*)PTOV_I(kernel_page_table));
}