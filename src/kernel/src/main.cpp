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
// #include "drivers/storage/ide.hpp"
// #include "drivers/storage/ahci.hpp"
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

// #include "filesystems/iso9660.hpp"
// #include "filesystems/fat32.hpp"
// #include "filesystems/vfs.hpp"

#include "storage/drivers/ide.hpp"
#include "storage/drivers/ahci.hpp"
#include "storage/block_device.hpp"
#include "storage/vfs.hpp"
#include "storage/mbr.hpp"
#include "storage/filesystems/iso9660.hpp"
#include "storage/filesystems/fat32.hpp"
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

#define HEAP_START_SIZE 0x100000 * 32 // 32 mb
#define PIT_TIMER_INTERVAL 1000 // times per second
#define DEVICE_HOST_NAME "VirtualReflections Machine"

bool setup_network_functionality() {
    const system_driver_handle_t inet_driver_handle = driver_manager_get_driver_handle(get_global_driver_manager(), "INetDrivers");

    if (inet_driver_handle == SYSTEM_DRIVER_HANDLE_INVALID)
        return false;

    // start our driver as the dhcp subsystem
    if (driver_query_capability(get_global_driver_manager(), inet_driver_handle, "dhcp") >= 1)
        subsys_init(SUBSYS_DHCP_CLIENT, std::make_unique<subsys_dhcp_client_driver_t>(DEVICE_HOST_NAME));

    // start our driver as the dhcp subsystem
    if (driver_query_capability(get_global_driver_manager(), inet_driver_handle, "dns") >= 1)
        subsys_init(SUBSYS_DNS_CLIENT, std::make_unique<subsys_dns_client_driver_t>());

    // lets hope that eth0 was configured qq
    auto subsystem_dhcp_client = subsys_get<subsys_dhcp_client_t>(SUBSYS_DHCP_CLIENT);
    if (!subsystem_dhcp_client)
        return false;

    auto network_interface = nidm_get_device_on_interface(get_global_nidm(), "eth0");
    if (!network_interface)
        return false;

    subsystem_dhcp_client->configure(network_interface);

    return true;
}

void init_pci_devices(const pci_device_t* device) {
    if (is_e1000_device(device)) {
        // TODO @since 06/02/2026 -- 09:50
        // add logging
        auto e1000 = std::make_unique<e1000_t>();
        if (e1000_init_device(device, e1000.get()) == 0) {
            std::unique_ptr<e1000_nid_t> e1000_nid = std::make_unique<e1000_nid_t>(move(e1000));
            e1000_nid->is_up = true;
            e1000_nid->is_configured = false;

            // TODO: move these into nidm since its responsible for managing network devices
            e1000_nid->is_prefered = true;
            e1000_nid->interface = "eth0";
            
            nidm_register_device(get_global_nidm(), move(e1000_nid));
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

struct cpu_local_t {
    uint64_t kernel_rsp;
    uint64_t user_rsp;
};

static cpu_local_t cpu0 {};

void enter_usermode(uint64_t entry_point, uint64_t user_stack_top) {
    cli();

    uint16_t user_ds = (uint16_t)(X86_64_GDT_INDEX_TO_ENTRY(USER_DATA_SELECTOR_INDEX) | 3);
    uint16_t user_cs = (uint16_t)(X86_64_GDT_INDEX_TO_ENTRY(USER_CODE_SELECTOR_INDEX) | 3);

    asm volatile (
        // set data segments
        "mov %w[ds], %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        // build iretq frame
        "pushq %[ss]\n"
        "pushq %[rsp]\n"
        "pushq %[rflags]\n"
        "pushq %[cs]\n"
        "pushq %[rip]\n"
        "iretq\n"
        :
        : [ds]     "r" ((uint64_t)user_ds),
          [ss]     "r" ((uint64_t)user_ds),
          [rsp]    "r" (user_stack_top),
          [rflags] "r" (0x202ULL),
          [cs]     "r" ((uint64_t)user_cs),
          [rip]    "r" (entry_point)
        : "rax", "memory"
    );
}

void user_function() {
    uint64_t ret;
    asm volatile(
        "mov $0, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        :
        : "rcx", "r11", "memory"
    );

    // we cant leave user mode in the current state
    while (true);
}

extern "C" uint64_t syscall_dispatch(uint64_t syscall_num) {
    kprintf("syscall_num = %ul\n", syscall_num);
    return 0;
}

extern "C" void x86_64_syscall_handler();

void user_mode() {
    // TODO @since 24/02/2026 -- 21:01
    // 1. custom pml4 struct per process
    // 2. kernel api via syscalls
    // 3. process exit
    // 4. make this actually the part that the user interacts with, no longer with the kernel terminal etc....

    extern uint8_t KSTACK_TOP[];
    cpu0.kernel_rsp = (uint64_t)KSTACK_TOP;

    wrmsr(0xC0000102, (uint64_t)&cpu0);

    uint64_t efer = rdmsr(0xC0000080);
    efer |= 1;
    wrmsr(0xC0000080, efer);

    uint64_t star = ((uint64_t)X86_64_GDT_INDEX_TO_ENTRY(KERNEL_DATA_SELECTOR_INDEX) << 48)
              | ((uint64_t)X86_64_GDT_INDEX_TO_ENTRY(KERNEL_CODE_SELECTOR_INDEX) << 32);
    wrmsr(0xC0000081, star);
    wrmsr(0xC0000082, (uint64_t)x86_64_syscall_handler);
    wrmsr(0xC0000084, 0x300);

    void* kernel_page_table = get_pml4();

    void* user_stack_virtual = malloc_aligned(PAGE_SIZE, 16);

    enter_usermode((uint64_t)user_function, (uint64_t)((uint8_t*)user_stack_virtual + PAGE_SIZE));
}

extern "C" void kernel_entry(void* p_multiboot_struct, void* p_kpml4) {
    // validate multiboot
    if (mb_has_valid_magic((multiboot_t*)p_multiboot_struct) != MULTIBOOT_VER2)
        kernel_fatal(KERNEL_FATAL_MULTIBOOT_MAGIC_VALIDATE, "multiboot was not the excpected version");

    // initialize the gdt / tss
    gdt_init();

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

    // initialze vga text mode
    // TODO @since 10/10/2025 -- 01:24
    // vga (device) manager
    vga_tm_init_buffer(&g_vga_tm_buffer, (void*)VGA_TM_BUFFER_ADDR, VGA_TM_NUM_COLS, VGA_TM_NUM_ROWS);
    vga_tm_clear_buffer(&g_vga_tm_buffer);

    dma_heap_manager_t allocator {};
    set_global_dma_heap_manager(&allocator);
    dma_heap_manager_init(get_global_dma_heap_manager(), p_kpml4, (void*)VMEM_DMA_ALLOCATOR_START, PAGE_SIZE_LARGE * 128);

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

    // TODO @since 06/02/2026 -- 10:34
    // proper ps2 startup etc
    keyboard_initialize();

    nidm_t nidm {};
    nidm_init(&nidm);
    set_global_nidm(&nidm);

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

    std::dynamic_array<vfs_node_t> nodes {};
    if (vfs_list_directory(&vfs, "/harddisk0", &nodes)) {
        for (auto& node : nodes) {
            if (str_ends_with(node.name.c_str(), ".sys")) {
                std::string driver_name = node.name.substr(0, node.name.length() - 4);

                const std::string driver_path = std::string("/harddisk0/") + driver_name + ".sys";
                file_descriptor_t driver_file_handle = vfs_open_file(&vfs, driver_path.c_str());
                if (driver_file_handle == FILE_DESCRIPTOR_INVALID) {
                    kprintf("failed to open handle to driver '%s'\n", driver_name.c_str());
                    continue;
                }

                // DONT FREE IT!
                uint8_t* driver_file_data = nullptr;
                size_t size;
                if (!vfs_read_file(&vfs, driver_file_handle, &driver_file_data, &size)) {
                    kprintf("failed to read driver '%s'\n", driver_name.c_str());
                    continue;
                }

                system_driver_handle_t driver_handle = driver_load(get_global_driver_manager(), driver_name.c_str(), driver_file_data);
                if (driver_handle == SYSTEM_DRIVER_HANDLE_INVALID) {
                    kprintf("failed to load driver '%s'\n", driver_name.c_str());
                    free(driver_file_data);
                    continue;
                }

                int result = driver_start(get_global_driver_manager(), driver_handle);
                if (result != 0) {
                    kprintf("failed to start driver '%s'. code: %i\n", driver_name.c_str(), result);
                    free(driver_file_data);
                    continue;
                }

                kprintf("loaded driver '%s'. code: %i\n", driver_name.c_str(), result);
            }
        }
    }

    if (!setup_network_functionality())
        kprintf("failed to setup network functionality!\n");

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

    // if (vthread_create(desktop_init, p_kpml4) == VTHREAD_HANDLE_INVALID)
    //     printf("Failed start graphical environment\n");

    // desktop_init();

    const vthread_handle_t critical_threads[] = {
        vthread_create([]() { while (true) nidm_process_packet(); return 1; }, p_kpml4, "NIDM"),
        vthread_create([]() { while (true) ps2_mouse_process_packet(); return 1; }, p_kpml4, "PS/2 Mouse"),
        vthread_create([]() { while (true) ps2_keyboard_process_packet(); return 1; }, p_kpml4, "PS/2 Keyboard")
    };

    for (const auto& thread : critical_threads)
        vthread_set_critical(thread, true);

    if (vthread_create(terminal_thread_main, p_kpml4) == VTHREAD_HANDLE_INVALID)
        kprintf("failed to start terminal");

    // after usermode switch it basically shouldnt go back
    // user_mode();

    // we shoudn t reach this point since the kernel should never stop
    // incase we do just hang here so we dont break anything
    while (true) vthread_sleep(1);
}