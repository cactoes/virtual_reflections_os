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
#include "linker.hpp"
#include "cpu.hpp"

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

#include "memory/paging.hpp"
#include "elf.hpp"

static heap_t process_heap {};

extern "C" uint64_t syscall_dispatch(uint64_t syscall_num, syscall_regs_t* regs) {
    if (syscall_num == 0) {
        kprintf("[ SYSCALL ] terminated process\n");
        vthread_terminate();

        // safetey catch
        while (true);
    }

    switch (syscall_num) {
        case 1: {
            return (uint64_t)heap_alloc(&process_heap, regs->rdi);
        }
        default:
            break;
    }

    kprintf("[ \033[91mSYSCALL\033[0m ] unhandled syscall = %ul\n", syscall_num);
    return 0;
}

extern "C" void x86_64_syscall_handler();

void create_process_test() {
    // BUG @since 05/03/2026 -- 09:22
    // page table never free'd

    // create new page table
    uint64_t* new_pt_virt = (uint64_t*)malloc_aligned(PAGE_SIZE, PAGE_SIZE);
    memzero(new_pt_virt, PAGE_SIZE);
    uint64_t new_pt_phys = (uint64_t)vmem_virtual_to_physical((void*)new_pt_virt);

    // revursive map page table
    vmem_recusive_map_page_table((void*)new_pt_virt, (void*)new_pt_phys);

    // copy the kernel entries so its always linked
    uint64_t* current_pml4 = GET_PML4_VIRT();
    constexpr uint64_t pml4e = KPAGING_GET_PE(KERNEL_VIRTUAL_BASE, 39);
    for (size_t i = pml4e; i < 511; i++)
        new_pt_virt[i] = current_pml4[i];

    // now load the elf file

    file_descriptor_t fd = vfs_open_file(get_global_vfs(), "harddisk0/TestProgram.exe");
    if (fd == FILE_DESCRIPTOR_INVALID) {
        kprintf("failed to open TestProgram.exe\n");
        return;
    }

    // FIXME @since 23/04/2026 -- 13:24
    // leaking program file
    uint8_t* program_file_data = nullptr;
    size_t size;
    if (!vfs_read_file(get_global_vfs(), fd, &program_file_data, &size)) {
        kprintf("failed to load TestProgram.exe\n");
        return;
    }

    if (elf_check_file(program_file_data, elf_type_t::EXECUTABLE) != 0) {
        kprintf("not elf file\n");
        return;
    }

    auto program_section_info = elf_parse_program_sections(program_file_data);
    
    // allocate it in kernel space for now aswell
    void* base_address = malloc_aligned(align_up(program_section_info.size, PAGE_SIZE_LARGE), PAGE_SIZE_LARGE);
    void* base_addr_phys = vmem_virtual_to_physical(base_address);
    if (!base_address) {
        kprintf("failed to allocate base_address");
        return;
    }

    void* current_pt_phys = (void*)(current_pml4[511] & ~0xFFF);
    cli();
    set_pml4((void*)new_pt_phys);
    if (!vmem_map_2mb((void*)new_pt_phys, (void*)program_section_info.min_address, base_addr_phys, true))
        debug_trap("failed to vmem_map");
    set_pml4(current_pt_phys);
    sti();

    cli();
    set_pml4((void*)new_pt_phys);
    heap_init(&process_heap, (void*)new_pt_phys, (void*)PAGE_SIZE_HUGE, PAGE_SIZE_LARGE, true);
    set_pml4(current_pt_phys);
    sti();

    elf_load_program_sections(program_file_data, (uint8_t*)base_address, &program_section_info);

    auto tables = elf_get_tables(program_file_data);
    if (!tables.string_table || !tables.symbol_table)
        return;

    // for now skip relocate sections

    void* entry = (void*)((elf_header_t*)program_file_data)->entry_point;

    kprintf("[PROCESS] process loaded at: 0x%p\n", entry);

    // create a new thread with the new page table

    uint64_t* user_stack_kernel_virt = (uint64_t*)malloc_aligned(VTHREAD_STACK_SIZE, PAGE_SIZE_LARGE);
    memzero(user_stack_kernel_virt, VTHREAD_STACK_SIZE);
    void* user_stack_physical = vmem_virtual_to_physical(user_stack_kernel_virt);
    void* user_stack_virtual = (void*)(PAGE_SIZE_LARGE * 1ULL);

    cli();
    set_pml4((void*)new_pt_phys);
    if (!vmem_map_2mb((void*)new_pt_phys, user_stack_virtual, user_stack_physical, true))
        debug_trap("failed to vmem_map");
    set_pml4(current_pt_phys);
    sti();

    std::unique_ptr<vthread_t> p_vthread = std::make_unique<vthread_t>();
    p_vthread->stack_bottom = user_stack_virtual;

    uint64_t* stack_top = (uint64_t*)(((uint64_t)user_stack_virtual + VTHREAD_STACK_SIZE - sizeof(interrupt_regs_t)) & ~0xF);
    uint64_t* mapped_stack_top = (uint64_t*)(((uint64_t)user_stack_kernel_virt + VTHREAD_STACK_SIZE - sizeof(interrupt_regs_t)) & ~0xF);

    p_vthread->stack_bottom_kernel = (void*)(user_stack_kernel_virt);

    p_vthread->kstack = (void*)((uint64_t)malloc_aligned(VTHREAD_STACK_SIZE, 16) + VTHREAD_STACK_SIZE);
    // cpu0.kernel_rsp = (uint64_t)p_vthread->kstack;

    uint16_t user_ds = (uint16_t)((USER_DATA_SELECTOR_INDEX << 3) | 3);
    uint16_t user_cs = (uint16_t)((USER_CODE_SELECTOR_INDEX << 3) | 3);

    // itret frame
    *(--mapped_stack_top) = (uint64_t)user_ds;
    *(--mapped_stack_top) = (uint64_t)stack_top;
    *(--mapped_stack_top) = 0x202;
    *(--mapped_stack_top) = user_cs;
    *(--mapped_stack_top) = (uint64_t)entry;
    *(--mapped_stack_top) = 0;

    // general registers
    for (int i = 0; i < 12; i++) {
        *(--mapped_stack_top) = 0;
    }

    *(--mapped_stack_top) = 0;
    *(--mapped_stack_top) = 0;

    p_vthread->stack_top = (void*)mapped_stack_top;
    p_vthread->vt_state = vthread_state_t::RUNNING;
    p_vthread->handle = 12345;
    p_vthread->fpu_state = (uint8_t*)malloc_aligned(sizeof(uint8_t) * 512, 16);
    p_vthread->pml4 = (void*)new_pt_phys;
    p_vthread->tls.handle = p_vthread->handle;

    vthread_add(move(p_vthread));
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

    kprintf("[ \033[92mOK\033[0m ] initialized memory\n");
    printf("[ \033[92mOK\033[0m ] initialized memory\n");

    // initialze the interrupt line(s)
    set_interrupt_callback(interrupt_t::HARDWARE_PIT, pit_handle_interrupt);
    set_interrupt_callback(interrupt_t::HARDWARE_KEYBOARD, ps2_keyboard_handle_interrupt);
    set_interrupt_callback(interrupt_t::HARDWARE_PS2_MOUSE, ps2_mouse_handle_interrupt);
    set_interrupt_callback(interrupt_t::SOFTWARE_SCHEDULER, vthread_handle_interrupt);

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
                    kprintf("[ \033[91mERROR\033[0m ] failed open file handle to driver: '%s'\n", driver_name.c_str());
                    printf("[ \033[91mERROR\033[0m ] failed open file handle to driver: '%s'\n", driver_name.c_str());
                    continue;
                }

                // DONT FREE IT!
                uint8_t* driver_file_data = nullptr;
                size_t size;
                if (!vfs_read_file(&vfs, driver_file_handle, &driver_file_data, &size)) {
                    kprintf("[ \033[91mERROR\033[0m ] failed to parse driver file '%s'\n", driver_name.c_str());
                    printf("[ \033[91mERROR\033[0m ] failed to parse driver file '%s'\n", driver_name.c_str());
                    continue;
                }

                system_driver_handle_t driver_handle = driver_load(get_global_driver_manager(), driver_name.c_str(), driver_file_data);
                if (driver_handle == SYSTEM_DRIVER_HANDLE_INVALID) {
                    kprintf("[ \033[91mERROR\033[0m ] failed to load driver '%s'\n", driver_name.c_str());
                    printf("[ \033[91mERROR\033[0m ] failed to load driver '%s'\n", driver_name.c_str());
                    free(driver_file_data);
                    continue;
                }

                int result = driver_start(get_global_driver_manager(), driver_handle);
                if (result != 0) {
                    kprintf("[ \033[91mERROR\033[0m ] failed to start driver '%s'. code: %i\n", driver_name.c_str(), result);
                    printf("[ \033[91mERROR\033[0m ] failed to start driver '%s'. code: %i\n", driver_name.c_str(), result);
                    free(driver_file_data);
                    continue;
                }

                kprintf("[ \033[92mOK\033[0m ] loaded driver '%s'. code: %i\n", driver_name.c_str(), result);
                printf("[ \033[92mOK\033[0m ] loaded driver '%s'. code: %i\n", driver_name.c_str(), result);
            }
        }
    }

    if (setup_network_functionality()) {
        kprintf("[ \033[92mOK\033[0m ] initialized networking\n");
        printf("[ \033[92mOK\033[0m ] initialized networking\n");
    } else {
        kprintf("[ \033[91mERROR\033[0m ] failed to setup networking\n");
        printf("[ \033[91mERROR\033[0m ] failed to setup networking\n");
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

    // if (vthread_create(desktop_init, p_kpml4) == VTHREAD_HANDLE_INVALID)
    //     kprintf("failed to create desktop thread\n");
    
    // while (!is_desktop_ready());
    // minesweeper_init();

    // if (vthread_create(desktop_init, p_kpml4) == VTHREAD_HANDLE_INVALID)
    //     printf("Failed start graphical environment\n");

    // desktop_init();

    void* kernel_pt_paddr = vmem_virtual_to_physical(kernel_pt_vaddr);

    const vthread_handle_t critical_threads[] = {
        vthread_create([]() { while (true) nidm_process_packet(); return 1; }, kernel_pt_paddr, "NIDM"),
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

    // after usermode switch it basically shouldnt go back
    // usermode_test();
    create_process_test();

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