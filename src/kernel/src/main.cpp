

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

#include "drivers/graphics/graphics_driver.hpp"

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

// void tcp_socket(socket_t*, u32 ip, u16 port, const u8* packet, size_t size) {
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
//     socket_send(&socket, (const u8*)http_request, sizeof(http_request) - 1);

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
                    printf("[IDE] mounted: %s\n", name);
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
                    printf("[AHCI] mounted: %s\n", name);
                }
            }
        } else {
            kprintf("[AHCI] driver failed to init\n");
        }

        // valid device so we can continue to the next device
        return;
    }
};

void* crash_handler_callback(void* stack, void* data) {
    // TODO @since 19/05/2026 -- 16:09
    // this is still heavily based on architecture
    // so we need to find some alternative for this
    kernel_fatal_internal((u64)data, "critical interrupt triggerd!", (struct interrupt_regs_t*)stack);
    return stack;
}

extern bool io_term_init(size_t w, size_t h);

extern "C" NORETURN void virtual_kernel_entry(multiboot2_info_t* multiboot_struct) {
    // initialze the debug out stream
    debug_init();

    // we already need system info here ...
    // just make sure we dont use the string's yet since memory is not setup yet
    system_info_manager_t sim {};
    set_global_system_info_manager(&sim);
    system_info_parse_memory_size(get_global_system_info_manager(), multiboot_struct);

    kprintf("[ \033[92mOK\033[0m ] parsed memory: %ul\n", sim.memory_size);

    // same as the assembly
    const u64 kernel_page_count = (LINKER_END_KERNEL_PHYS + (PAGE_SIZE_LARGE - 1)) >> 21;
    for (u64 i = 0; i < kernel_page_count; i++)
        vmem_unmap_2mb((void*)(i * PAGE_SIZE_LARGE));

    // initialze virtual memory
    if (!vmem_init(multiboot_struct))
        kernel_fatal(KERNEL_FATAL_VMEM_INIT, "vmem failed to initialize");

    // initialze the global heap
    heap_t heap {};
    if (!heap_init(&heap, (void*)VMEM_KERNEL_HEAP_START, HEAP_START_SIZE))
        kernel_fatal(KERNEL_FATAL_HEAP_INIT, "kernel heap fail to initialze");

    set_global_heap(&heap);

    kprintf("[ \033[92mOK\033[0m ] initialized memory\n");

    graphics_driver_t gd {};
    graphics_driver_init(&gd, multiboot_struct);
    set_global_graphics_driver(&gd);
    io_term_init(gd.framebuffer->width, gd.framebuffer->height);

    kprintf("[ \033[92mOK\033[0m ] initialized graphics driver\n");
    printf("[ \033[92mOK\033[0m ] initialized graphics driver\n");

    // initialze the interrupt line(s)
    hook_interrupt(interrupt_t::EXCEPTION_DIVISION_BY_ZERO, crash_handler_callback, (void*)interrupt_t::EXCEPTION_DIVISION_BY_ZERO);
    hook_interrupt(interrupt_t::EXCEPTION_SINGLE_STEP_INTERRUPT, crash_handler_callback, (void*)interrupt_t::EXCEPTION_SINGLE_STEP_INTERRUPT);
    hook_interrupt(interrupt_t::EXCEPTION_NMI, crash_handler_callback, (void*)interrupt_t::EXCEPTION_NMI);
    hook_interrupt(interrupt_t::EXCEPTION_BREAKPOINT, crash_handler_callback, (void*)interrupt_t::EXCEPTION_BREAKPOINT);
    hook_interrupt(interrupt_t::EXCEPTION_OVERFLOW, crash_handler_callback, (void*)interrupt_t::EXCEPTION_OVERFLOW);
    hook_interrupt(interrupt_t::EXCEPTION_BOUND_RANGE_EXCEEDED, crash_handler_callback, (void*)interrupt_t::EXCEPTION_BOUND_RANGE_EXCEEDED);
    hook_interrupt(interrupt_t::EXCEPTION_INVALID_OPCODE, crash_handler_callback, (void*)interrupt_t::EXCEPTION_INVALID_OPCODE);
    hook_interrupt(interrupt_t::EXCEPTION_COPROCESSOR_NOT_AVAILABLE, crash_handler_callback, (void*)interrupt_t::EXCEPTION_COPROCESSOR_NOT_AVAILABLE);
    hook_interrupt(interrupt_t::EXCEPTION_DOUBLE_FAULT, crash_handler_callback, (void*)interrupt_t::EXCEPTION_DOUBLE_FAULT);
    hook_interrupt(interrupt_t::EXCEPTION_COPROCESSOR_SEGMENT_OVERRUN, crash_handler_callback, (void*)interrupt_t::EXCEPTION_COPROCESSOR_SEGMENT_OVERRUN);
    hook_interrupt(interrupt_t::EXCEPTION_INVALID_TSS, crash_handler_callback, (void*)interrupt_t::EXCEPTION_INVALID_TSS);
    hook_interrupt(interrupt_t::EXCEPTION_SEGMENT_NOT_PRESENT, crash_handler_callback, (void*)interrupt_t::EXCEPTION_SEGMENT_NOT_PRESENT);
    hook_interrupt(interrupt_t::EXCEPTION_STACK_SEGMENT_FAULT, crash_handler_callback, (void*)interrupt_t::EXCEPTION_STACK_SEGMENT_FAULT);
    hook_interrupt(interrupt_t::EXCEPTION_GENERAL_PROTECTION_FAULT, crash_handler_callback, (void*)interrupt_t::EXCEPTION_GENERAL_PROTECTION_FAULT);
    hook_interrupt(interrupt_t::EXCEPTION_PAGE_FAULT, crash_handler_callback, (void*)interrupt_t::EXCEPTION_PAGE_FAULT);
    hook_interrupt(interrupt_t::EXCEPTION_RESERVED, crash_handler_callback, (void*)interrupt_t::EXCEPTION_RESERVED);
    hook_interrupt(interrupt_t::EXCEPTION_X87_FLOATING_POINT_EXCEPTION, crash_handler_callback, (void*)interrupt_t::EXCEPTION_X87_FLOATING_POINT_EXCEPTION);
    hook_interrupt(interrupt_t::EXCEPTION_ALIGNMENT_CHECK, crash_handler_callback, (void*)interrupt_t::EXCEPTION_ALIGNMENT_CHECK);
    hook_interrupt(interrupt_t::EXCEPTION_MACHINE_CHECK, crash_handler_callback, (void*)interrupt_t::EXCEPTION_MACHINE_CHECK);
    hook_interrupt(interrupt_t::EXCEPTION_SIMD_FP_EXCEPTION, crash_handler_callback, (void*)interrupt_t::EXCEPTION_SIMD_FP_EXCEPTION);
    hook_interrupt(interrupt_t::EXCEPTION_VIRTUALIZATION_EXCEPTION, crash_handler_callback, (void*)interrupt_t::EXCEPTION_VIRTUALIZATION_EXCEPTION);
    hook_interrupt(interrupt_t::EXCEPTION_CONTROL_PROTECTION_EXCEPTION, crash_handler_callback, (void*)interrupt_t::EXCEPTION_CONTROL_PROTECTION_EXCEPTION);

    hook_interrupt(interrupt_t::HARDWARE_PIT, pit_handle_interrupt, nullptr);
    hook_interrupt(interrupt_t::HARDWARE_KEYBOARD, ps2_keyboard_handle_interrupt, nullptr);
    hook_interrupt(interrupt_t::HARDWARE_PS2_MOUSE, ps2_mouse_handle_interrupt, nullptr);

    hook_interrupt(interrupt_t::SOFTWARE_SCHEDULER, vthread_handle_interrupt, nullptr);

    ps2_mouse_init();
    pit_init(PIT_TIMER_INTERVAL);

    dma_heap_manager_t allocator {};
    set_global_dma_heap_manager(&allocator);
    dma_heap_manager_init(get_global_dma_heap_manager(), (void*)VMEM_DMA_ALLOCATOR_START, PAGE_SIZE_LARGE * 128);

    // initialize threading
    if (vthread_start_and_setup_main() == VTHREAD_HANDLE_INVALID)
        kernel_fatal(KERNEL_FATAL_VTHREAD_INIT, "virtual threads failed to intialize");

    kprintf("[ \033[92mOK\033[0m ] enabled virtual threading\n");
    printf("[ \033[92mOK\033[0m ] enabled virtual threading\n");

    // TODO @since 19/05/2026 -- 16:28
    // move this shit
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
    //             u8* driver_file_data = nullptr;
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

    const vthread_handle_t critical_threads[] = {
        vthread_create_local(nic_thread, "network interface controller"),
        vthread_create_local([]() { while (true) ps2_mouse_process_packet(); return 1; }, "PS/2 Mouse"),
        vthread_create_local([]() { while (true) ps2_keyboard_process_packet(); return 1; }, "PS/2 Keyboard")
    };

    for (const auto& thread : critical_threads)
        vthread_set_critical(thread, true);

    // kernel finished
    kprintf("[ KERNEL SETUP FINISHED ]\n");
    printf("[ KERNEL SETUP FINISHED ]\n");

    if (vthread_create_local(terminal_thread_main) == VTHREAD_HANDLE_INVALID) {
        kprintf("[ \033[91mERROR\033[0m ] failed to start terminal\n");
        printf("[ \033[91mERROR\033[0m ] failed to start terminal\n");
    }

    // we shoudn t reach this point since the kernel should never stop
    // incase we do just hang here so we dont break anything
    while (true) vthread_yield();
}