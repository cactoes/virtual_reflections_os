#include "drivers/vga.hpp"
#include "drivers/pcie.hpp"
#include "drivers/pit.hpp"
#include "drivers/keyboard.hpp"
#include "drivers/ps2/keyboard.hpp"
#include "drivers/ps2/mouse.hpp"
#include "drivers/ps2/ps2.hpp"
#include "drivers/driver.hpp"
#include "drivers/network/e1000.hpp"
#include "drivers/network/rtl8168.hpp"
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
    // const char* cd = pci_get_class_description(device);
    // printf("[PCIe] %s: (%u:%u.%u) 0x%uh:0x%uh\n", cd, device->bus, device->device, device->function, device->vendor_device_id.vendor_id, device->vendor_device_id.device_id);

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

            nic_register_interface(get_global_nic(), e1000_network_interface);

            network_manager_configre_interface(get_global_network_manager(), e1000_network_interface);
        }

        // valid device so we can continue to the next device
        return;
    }

    if (is_rtl8168_device(device)) {
        rtl8168_t* rtl8168 = (rtl8168_t*)malloc(sizeof(rtl8168_t));
        memzero(rtl8168, sizeof(rtl8168_t));

        if (rtl8168_init_device(device, rtl8168)) {
            network_interface_t* rtl8168_network_interface = (network_interface_t*)malloc(sizeof(network_interface_t));
            memzero(rtl8168_network_interface, sizeof(network_interface_t));

            rtl8168_network_interface->device = rtl8168;
            rtl8168_network_interface->device_type = network_interface_device_type_t::RTL8168;
            rtl8168_network_interface->is_configured = true;

            memcpy(rtl8168_network_interface->mac, rtl8168->mac, 6);

            const char* device_name = "Realtek RTL8168";
            strncpy(rtl8168_network_interface->device_name, device_name, sizeof(rtl8168_network_interface->device_name));

            nic_register_interface(get_global_nic(), rtl8168_network_interface);

            // network_manager_configre_interface(get_global_network_manager(), rtl8168_network_interface);

            printf("[PCIe] initialized rtl8168 device\n");
        } else {
            printf("[PCIe] failed to initialize rtl8168 device\n");
        }

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
                    printf("[IDE] failed to mount drive: %s\n", name);
                } else {
                    printf("[IDE] mounted: %s\n", name);
                }
            }
        } else {
            printf("[IDE] driver failed to init\n");
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
                    printf("failed to mount drive: %s\n", name);
                } else {
                    printf("[AHCI] mounted: %s\n", name);
                }
            }
        } else {
            printf("[AHCI] driver failed to init\n");
        }

        // valid device so we can continue to the next device
        return;
    }
};

union kernel_components_t {
    u64 raw;
    struct {
        u64 heap                : 1;
        u64 dma                 : 1;
        u64 graphics            : 1;
        u64 interrupts          : 1;
        u64 system_info         : 1;
        u64 virtual_threading   : 1;
        u64 virtual_filesystem  : 1;
        u64 nic                 : 1;
    };
};

static
void system_log(const char* level, const char* color, const char* message) {
    kprintf("[ %s%s\033[0m ] %s\n", color, level, message);
    printf("[ %s%s\033[0m ] %s\n", color, level, message);
}

static
void system_log_ok(const char* message) { 
    system_log("OK", "\033[92m", message); 
}

static
void system_log_error(const char* message) { 
    system_log("ERROR", "\033[91m", message); 
}

static
void system_log_info(const char* system, const char* message) { 
    system_log(system, "\033[96m", message);
}

static kernel_components_t initialized_kernel_components {};
static heap_t kernel_heap {};
static dma_heap_manager_t kernel_dma_allocator {};
static graphics_driver_t kernel_graphics_driver {};
static vfs_t kernel_vfs {};
static network_interface_controller_t kernel_nic {};
static system_info_manager_t kernel_sim {};

static
void init_memory() {
    if (!heap_init(&kernel_heap, (void*)VMEM_KERNEL_HEAP_START, HEAP_START_SIZE))
        kernel_fatal(KERNEL_FATAL_HEAP_INIT, "kernel heap fail to initialize");

    set_global_heap(&kernel_heap);

    initialized_kernel_components.heap = true;

    if (dma_heap_manager_init(&kernel_dma_allocator, (void*)VMEM_DMA_ALLOCATOR_START, PAGE_SIZE_LARGE * 128)) {
        initialized_kernel_components.dma = true;
        set_global_dma_heap_manager(&kernel_dma_allocator);
    } else {
        // TODO @since 01/06/2026 -- 23:59
        // this should also fatal
        system_log_error("failed to initialize dma allocator");
    }

    system_log_ok("setup virtual memory");
}

extern bool io_term_init(size_t w, size_t h);

void init_graphics(multiboot2_info_t* multiboot_struct) {
    if (graphics_driver_init(&kernel_graphics_driver, multiboot_struct)) {
        set_global_graphics_driver(&kernel_graphics_driver);

        graphics_driver_reset(&kernel_graphics_driver);
        graphics_driver_render(&kernel_graphics_driver);

        initialized_kernel_components.graphics = true;

        io_term_init(kernel_graphics_driver.framebuffer->width, kernel_graphics_driver.framebuffer->height);
    } else {
        kernel_fatal(KERNEL_FATAL_GRAPHICS_INIT, "graphics failed driver to initialize");
    }

    system_log_ok("initialized graphics driver");
}

void* crash_handler_callback(void* stack, void* data) {
    // TODO @since 19/05/2026 -- 16:09
    // this is still heavily based on architecture
    // so we need to find some alternative for this
    kernel_fatal_internal((u64)data, "critical interrupt triggerd!", (struct interrupt_regs_t*)stack);
    return stack;
}

static
void init_interrupts() {
    const interrupt_t exception_interrupts[] = {
        interrupt_t::EXCEPTION_DIVISION_BY_ZERO,
        interrupt_t::EXCEPTION_SINGLE_STEP_INTERRUPT,
        interrupt_t::EXCEPTION_NMI,
        interrupt_t::EXCEPTION_BREAKPOINT,
        interrupt_t::EXCEPTION_OVERFLOW,
        interrupt_t::EXCEPTION_BOUND_RANGE_EXCEEDED,
        interrupt_t::EXCEPTION_INVALID_OPCODE,
        interrupt_t::EXCEPTION_COPROCESSOR_NOT_AVAILABLE,
        interrupt_t::EXCEPTION_DOUBLE_FAULT,
        interrupt_t::EXCEPTION_COPROCESSOR_SEGMENT_OVERRUN,
        interrupt_t::EXCEPTION_INVALID_TSS,
        interrupt_t::EXCEPTION_SEGMENT_NOT_PRESENT,
        interrupt_t::EXCEPTION_STACK_SEGMENT_FAULT,
        interrupt_t::EXCEPTION_GENERAL_PROTECTION_FAULT,
        interrupt_t::EXCEPTION_PAGE_FAULT,
        interrupt_t::EXCEPTION_RESERVED,
        interrupt_t::EXCEPTION_X87_FLOATING_POINT_EXCEPTION,
        interrupt_t::EXCEPTION_ALIGNMENT_CHECK,
        interrupt_t::EXCEPTION_MACHINE_CHECK,
        interrupt_t::EXCEPTION_SIMD_FP_EXCEPTION,
        interrupt_t::EXCEPTION_VIRTUALIZATION_EXCEPTION,
        interrupt_t::EXCEPTION_CONTROL_PROTECTION_EXCEPTION,
    };

    for (const auto& interrupt : exception_interrupts)
        hook_interrupt(interrupt, crash_handler_callback, (void*)interrupt);

    hook_interrupt(interrupt_t::HARDWARE_PIT, pit_handle_interrupt, nullptr);
    hook_interrupt(interrupt_t::HARDWARE_KEYBOARD, ps2_keyboard_handle_interrupt, nullptr);
    hook_interrupt(interrupt_t::HARDWARE_PS2_MOUSE, ps2_mouse_handle_interrupt, nullptr);

    hook_interrupt(interrupt_t::SOFTWARE_SCHEDULER, vthread_handle_interrupt, nullptr);

    pit_init(PIT_TIMER_INTERVAL);

    initialized_kernel_components.interrupts = true;

    system_log_ok("hooked interrupts");
}

static
void init_system_info(void* multiboot_struct) {
    set_global_system_info_manager(&kernel_sim);
    system_info_parse_memory_size(&kernel_sim, (multiboot2_info_t*)multiboot_struct);
    // only works in non uefi mode
    // system_info_parse_system_information(get_global_system_info_manager());
    system_info_get_cpu_name(&kernel_sim);

    initialized_kernel_components.system_info = true;

    system_log_ok("parsed system info");
}

static
void init_virtual_threading() {
    if (vthread_start_and_setup_main() == VTHREAD_HANDLE_INVALID)
        kernel_fatal(KERNEL_FATAL_VTHREAD_INIT, "virtual threads failed to intialize");

    // TODO @since 02/06/2026 -- 13:06
    // make all of these have their own dedicated thread function
    const vthread_handle_t critical_threads[] = {
        vthread_create_local(nic_thread, "_ZN7kthread3nicEv"),
        vthread_create_local([]() { while (true) ps2_mouse_process_packet(); return 1; }, "PS/2 Mouse"),
        vthread_create_local([]() { while (true) ps2_keyboard_process_packet(); return 1; }, "PS/2 Keyboard")
    };

    for (const auto& thread : critical_threads)
        vthread_set_critical(thread, true);

    initialized_kernel_components.virtual_threading = true;

    system_log_ok("enabled virtual threading");
}

static
void init_virtual_filesystem() {
    vfs_init(&kernel_vfs);
    set_global_vfs(&kernel_vfs);

    initialized_kernel_components.virtual_filesystem = true;

    system_log_ok("initialized the virtual filesystem");
}

static
void init_network_interface_controller() {
    nic_init(&kernel_nic);
    set_global_nic(&kernel_nic);

    initialized_kernel_components.nic = true;

    system_log_ok("initialized the network interface controller");
}

extern "C" NORETURN void virtual_kernel_entry(multiboot2_info_t* multiboot_struct) {
    // initialze the debug out stream
    debug_init();
    
    // stage 1 -- core essentials
    // TODO @since 02/06/2026 -- 13:20
    // init com port / debug port
    init_memory();
    init_graphics(multiboot_struct);
    init_interrupts();
    init_system_info(multiboot_struct);

    // stage 2 -- core functionality
    init_virtual_threading();
    init_virtual_filesystem();
    init_network_interface_controller();
    // pci

    // stage 3 -- hardware

    // pci(e)
    // ps2
    // usb
    // etc...

    // drivers?



    // TODO @since 06/02/2026 -- 10:34
    // proper ps2 startup etc
    keyboard_initialize();
    ps2_mouse_init();

    network_manager_t network_manager {};
    network_manager_init(&network_manager);
    set_global_network_manager(&network_manager);
    if (network_manager_configure(&network_manager)) {
        printf("[ \033[92mOK\033[0m ] configured network manager\n");
    } else {
        printf("[ \033[91mERROR\033[0m ] failed to configure network manager\n");
    }

    // BIT_SET(global_online_systems_flags, KERNEL_SYSTEM_NM_BIT);

    storage_manager_t storage_manager {};
    storage_manager_init(&storage_manager);
    set_global_storage_manager(&storage_manager);

    // BIT_SET(global_online_systems_flags, KERNEL_SYSTEM_SM_BIT);

    pcie_device_manager_t pciedm {};
    set_global_pcie_device_manager(&pciedm);
    pci_enumerate_devices(get_global_pcie_device_manager());
    pci_loop_devices(get_global_pcie_device_manager(), init_pci_devices);

    // BIT_SET(global_online_systems_flags, KERNEL_SYSTEM_PCIE_BIT);

    driver_manager_t driver_manager {};
    set_global_driver_manager(&driver_manager);

    // BIT_SET(global_online_systems_flags, KERNEL_SYSTEM_DM_BIT);

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

    // kernel finished
    kprintf("[ KERNEL SETUP FINISHED ]\n");

    if (vthread_create_local(terminal_thread_main) == VTHREAD_HANDLE_INVALID)
        printf("[ \033[91mERROR\033[0m ] failed to start terminal\n");

    // we shoudn t reach this point since the kernel should never stop
    // incase we do just hang here so we dont break anything
    while (true)
        // amd64_halt();
        vthread_yield();
}