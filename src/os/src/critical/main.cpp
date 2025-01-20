#define __KERNEL_MEMORY_FULL__

#include "critical/memory.hpp"
#include "critical/kernel.hpp"
#include "critical/interrupt.hpp"

#include "driver/keyboard.hpp"
#include "driver/pci.hpp"

#include "string.hpp"

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" uint64_t KPML4T;

extern "C" void call_constructors() {
    for (constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}

void kernel_assert(bool status, uint64_t identifier) {
    if (status == false)
        kernel_fatal(KFATAL_KERNEL_ASSERTION_FAILED, identifier);
}

void kernel_assert_status(bool status, const char* message, uint64_t identifier) {
    kernel_assert(status, identifier);

    kernel_set_print_color(vga_color_t::VGAC_LIGHT_GRAY, vga_color_t::VGAC_BLACK);
    kernel_print("[ ");
    kernel_set_print_color(status ? vga_color_t::VGAC_GREEN : vga_color_t::VGAC_RED, vga_color_t::VGAC_BLACK);
    kernel_print(status ? "OK" : "FAILED");
    kernel_set_print_color(vga_color_t::VGAC_LIGHT_GRAY, vga_color_t::VGAC_BLACK);
    kernel_print(" ] ");
    kernel_print(message);
    kernel_print("\n");
}

void kernel_wait_status(const char* message) {
    kernel_set_print_color(vga_color_t::VGAC_LIGHT_GRAY, vga_color_t::VGAC_BLACK);
    kernel_print("[ ");
    kernel_set_print_color(vga_color_t::VGAC_YELLOW, vga_color_t::VGAC_BLACK);
    kernel_print("WAITING");
    kernel_set_print_color(vga_color_t::VGAC_LIGHT_GRAY, vga_color_t::VGAC_BLACK);
    kernel_print(" ] ");
    kernel_print(message);
    kernel_print("\n");
}

extern "C" uint64_t multiboot_magic;

// extern "C" uint64_t bss_size;
// extern "C" uint64_t bss_start;

void safe_draw_logo() {
    kernel_set_cursor(5, 44);  kernel_print("         *  ..        \n");
    kernel_set_cursor(6, 44);  kernel_print("        @@# @@.       \n");
    kernel_set_cursor(7, 44);  kernel_print("       @@*@@ @@.      \n");
    kernel_set_cursor(8, 44);  kernel_print("     .@@  #@@ @@:     \n");
    kernel_set_cursor(9, 44);  kernel_print("     @@ *@ :@@ @@:    \n");
    kernel_set_cursor(10, 44); kernel_print("   :@@ #@%  .@@ @@=   \n");
    kernel_set_cursor(11, 44); kernel_print("  :@@ %@#    .@@ @@+  \n");
    kernel_set_cursor(12, 44); kernel_print(" +@@ @@*       @@ %@* \n");
    kernel_set_cursor(13, 44); kernel_print(" @@: @@.       @@ -@@ \n");
    kernel_set_cursor(14, 44); kernel_print("  @@- @@:     @@ =@@  \n");
    kernel_set_cursor(15, 44); kernel_print("   @@* @@:   @@ *@@   \n");
    kernel_set_cursor(16, 44); kernel_print("    @@# @@. @@ *@@    \n");
    kernel_set_cursor(17, 44); kernel_print("     #@@ . @@ %@#     \n");
    kernel_set_cursor(18, 44); kernel_print("      +@@ @@ #@#      \n");
    kernel_set_cursor(19, 44); kernel_print("       -@@@ @@+       \n");
    kernel_set_cursor(20, 44); kernel_print("        :@  %=        \n");
}

extern "C" void kernel_main() {
    /// =============
    /// boot logo (VGA MODE)
    /// =============
    kernel_set_print_color(vga_color_t::VGAC_LIGHT_GRAY, vga_color_t::VGAC_BLACK);
    kernel_clear_screen();
    safe_draw_logo();
    kernel_set_cursor(6, 10);
    kernel_print("VirtualReflectionsOS [AS 0]");

    /// =============
    /// debug stage display
    /// =============
    kernel_set_cursor(NUM_ROWS - 1, NUM_COLS - 7);
    kernel_print("stage 0");

    /// =============
    /// check multiboot
    /// =============
    kernel_set_cursor(7, 10);
    kernel_wait_status("multiboot");
    kernel_clear_row(7);
    safe_draw_logo();
    kernel_set_cursor(7, 10);
    kernel_assert_status(multiboot_magic == 0x2BADB002, "multiboot", 0x1);

    /// =============
    /// setup interrupts
    /// =============
    kernel_set_cursor(8, 10);
    kernel_wait_status("interrupts");
    kernel::interrupt::init();
    kernel_clear_row(8);
    safe_draw_logo();
    kernel_set_cursor(8, 10);
    kernel_assert_status(true, "interrupts (0-32, keyboard)", 0x2);

    /// =============
    /// setup & test virtual memory
    /// =============
    kernel_set_cursor(9, 10);
    kernel_wait_status("memory");
    kernel::memory::heap_t kheap = {};
    kernel::memory::vmem::init(&kheap, &KPML4T);

    int* test_ptr = (int*)kernel::memory::vmem::kalloc(sizeof(int));
    *test_ptr = 0x1234567;

    kernel_clear_row(9);
    safe_draw_logo();
    kernel_set_cursor(9, 10);
    kernel_assert_status(*test_ptr == 0x1234567, "memory", 0x3);
    kernel::memory::vmem::kfree((void*)test_ptr);

    /// =============
    /// setup PCI(e) & find basic devices
    /// =============
    kernel_set_cursor(10, 10);
    kernel_wait_status("PCI");

    // get pci(e) device count
    size_t device_count;
    (void)kernel::driver::pci::get_pci_device_count(&device_count);

    auto pci_entries = kernel_make_unique_ptr<kernel::driver::pci::pci_device_info_t[]>(device_count);

    // store all devices
    (void)kernel::driver::pci::enumerate_pci_devices(pci_entries.get(), device_count, &device_count);

    kernel::driver::pci::pci_device_request_t requested_devices[2];
    constexpr auto requested_devices_size = sizeof(requested_devices) / sizeof(kernel::driver::pci::pci_device_request_t);

    // network controller
    requested_devices[0].class_code = 0x02;

    // S-ATA controller
    requested_devices[1].class_code = 0x01;
    requested_devices[1].sub_class = 0x06;

    kresult_t pci_device_find_result = kernel::driver::pci::find_pci_devices(pci_entries.get(), device_count, requested_devices, requested_devices_size);

    // print to kernel
    char buffer[256];
    sprintf(buffer, 256, "PCI (%u - %u (%u devices))", requested_devices_size, device_count - requested_devices_size, device_count);

    kernel_clear_row(10);
    safe_draw_logo();
    kernel_set_cursor(10, 10);
    kernel_assert_status(KRESULT_IS_OK(pci_device_find_result), buffer, 0x4);

    /// =============
    /// setup XXX
    /// =============
    kernel_set_cursor(11, 10);

    kernel_set_graphics_mode();

    volatile uint8_t* vga_frame_buffer = (volatile uint8_t*)0xA0000;
    for (int i = 0; i < 640 * 480; i++)
        vga_frame_buffer[i] = 0x0F;

    // dont exit the kernel lol
    while (true) {}
}

/*
    shit ahh code for AHCI which never wants to work ...

    sources:
        https://forum.osdev.org/viewtopic.php?t=56479
        https://f.osdev.org/viewtopic.php?t=40718
        http://kurtqiao.github.io/uefi/2014/12/24/AHCI-mode.html
        https://wiki.osdev.org/PCI
        https://wiki.osdev.org/AHCI#Checklist


    kernel_clear_screen();

    auto& sata_device = pci_entries.get()[requested_devices[1].pci_device_index];

    // enable ahci in the controller
    uint32_t command;
    (void)kernel::driver::pci::config_read(sata_device.bus, sata_device.device, sata_device.function, 0x04, &command);
    command |= PCI_CMD_MMIO;
    command |= PCI_CMD_BUS_MASTERING;
    (void)kernel::driver::pci::config_write(sata_device.bus, sata_device.device, sata_device.function, 0x04, command);

    uint64_t ahci_base = sata_device.bar5_address;

    // kernel_print("0x%p < 0x%p\n", ahci_base, sata_device.bar5_address & ~0xF);

    typedef volatile struct __KD_PCI_HBA_PORT {
        uint32_t clb;
        uint32_t clbu;
        uint32_t fb;
        uint32_t fbu;
        uint32_t is;
        uint32_t ie;
        uint32_t cmd;
        uint32_t rsv0;
        uint32_t tfd;
        uint32_t sig;
        uint32_t ssts;
        uint32_t sctl;
        uint32_t serr;
        uint32_t sact;
        uint32_t ci;
        uint32_t sntf;
        uint32_t fbs;
        uint32_t rsv1[11];
        uint32_t vendor[4];
    } HBA_PORT;

    typedef volatile struct __KD_PCI_HBA_MEM {
        uint32_t CAP;
        uint32_t GHC;
        uint32_t IS;
        uint32_t PI;
        uint32_t VS;
        uint32_t CCC_CTL;
        uint32_t CCC_PORTS;
        uint32_t EM_LOC;
        uint32_t EM_CTL;
        uint32_t CAP2;
        uint32_t BOHC;

        uint8_t rsv[0xA0-0x2C];
        uint8_t vendor[0x100-0xA0];

        HBA_PORT ports[6];
    } HBA_MEM;

    typedef volatile struct __HBA_PRDT_ENTRY {
        uint32_t dba;      // Data Base Address
        uint32_t dbau;     // Data Base Address Upper 32 bits
        uint32_t dbc : 22; // Byte Count (0-based)
        uint32_t rsv0 : 9; // Reserved
        uint32_t i : 1;    // Interrupt on Completion
    } HBA_PRDT_ENTRY;

    typedef struct {
        uint8_t fis[64];  // FIS structure
        uint8_t acmd[16]; // ATAPI command
        uint8_t rsv[48];  // Reserved
        uint8_t prdt[128]; // Physical Region Descriptor Table
    } HBA_CMD_TABLE;

    typedef struct {
        uint8_t cfl:5;     // Command FIS length
        uint8_t a:1;       // ATAPI
        uint8_t w:1;       // Write
        uint8_t p:1;       // Prefetchable
        uint8_t r:1;       // Reset
        uint8_t b:1;       // BIST
        uint8_t c:1;       // Clear busy upon R_OK
        uint8_t rsv0:1;    // Reserved
        uint8_t pmp:4;     // Port multiplier port
        uint16_t prdtl;    // Physical region descriptor table length
        uint32_t prdbc;    // Physical region descriptor byte count
        uint32_t ctba;     // Command table descriptor base address
        uint32_t ctbau;    // Command table descriptor base address upper 32 bits
        uint32_t rsv1[4];  // Reserved
    } HBA_CMD_HEADER;

    volatile HBA_MEM* hba_physical = (volatile HBA_MEM*)ahci_base;
    uint64_t hba_aligned = kernel::memory::align_up_to((uint64_t)hba_physical, PAGE_SIZE_LARGE) - PAGE_SIZE_LARGE;
    kernel::memory::paging::map_large_page(&KPML4T, (void*)hba_aligned, (void*)hba_aligned);
    volatile HBA_MEM* hba = (volatile HBA_MEM*)hba_physical;

    // uint64_t identify_data_physical = (uint64_t)kernel::memory::paging::get_page();
    // void* identify_data_virtual = (void*)0x7f000000;
    // kernel::memory::paging::map_large_page(&KPML4T, identify_data_virtual, (void*)identify_data_physical);
    // memzero(identify_data_virtual, PAGE_SIZE);

    void* start_driver_virtual = (void*)0x7f000000;
    void* start_driver_physical = kernel::memory::paging::get_page();
    kernel::memory::paging::map_large_page(&KPML4T, (void*)((uint64_t)start_driver_virtual), start_driver_physical);
    kernel::memory::paging::page_reserve(PAGE_SIZE_LARGE / PAGE_SIZE - 1);
    memzero(start_driver_virtual, PAGE_SIZE_LARGE);

    // x3 size so we can align it properly to 1KiB
    // void* clb_base_unaligned = kernel::memory::vmem::kalloc(1024 * 3);
    // void* clb_base_virtual = (void*)kernel::memory::align_up_to((uint64_t)clb_base_unaligned, 1024);
    // uint64_t clb_base_physical = kernel::memory::paging::virtual_to_physical(&KPML4T, clb_base_virtual);
    // memzero(clb_base_unaligned, 1024 * 3);

    void* clb_base_unaligned = start_driver_virtual;
    void* clb_base_virtual = (void*)kernel::memory::align_up_to((uint64_t)clb_base_unaligned, 1024);
    uint64_t clb_base_physical = (uint64_t)start_driver_physical;

    // x3 size so we can align it properly to 256 bytes
    void* fb_base_unaligned = (void*)((uint64_t)start_driver_virtual + 1024);
    void* fb_base_virtual = (void*)kernel::memory::align_up_to((uint64_t)fb_base_unaligned, 256);
    // uint64_t fb_base_physical = kernel::memory::paging::virtual_to_physical(&KPML4T, fb_base_virtual);
    uint64_t fb_base_physical = (uint64_t)start_driver_physical + 1024;
    // memzero(fb_base_unaligned, 256 * 3);

    // +2 256 size so we can align it properly to 256 bytes
    void* command_table_unaligned = (void*)((uint64_t)start_driver_virtual + 1024 + 256);
    void* command_table_virtual = (void*)kernel::memory::align_up_to((uint64_t)command_table_unaligned, 256);
    uint64_t command_table_physical = (uint64_t)start_driver_physical + 1024 + 256;
    // memzero(command_table_unaligned, 1024 * 8 + 256 * 2);

    for (int i = 0; i < 32; i++) {
        if (hba->PI & (1 << i)) {
            volatile HBA_PORT* hba_port = &hba->ports[i];
            
            hba_port->cmd &= ~((1 << 4) | (1 << 14));
            while (hba_port->cmd & ((1 << 4) | (1 << 14))) {}

            hba_port->clb = (uint32_t)(clb_base_physical & 0xFFFFFFFF);
            hba_port->clbu = (uint32_t)((clb_base_physical >> 32) & 0xFFFFFFFF);

            hba_port->fb = (uint32_t)(fb_base_physical & 0xFFFFFFFF);
            hba_port->fbu = (uint32_t)((fb_base_physical >> 32) & 0xFFFFFFFF);

            volatile HBA_CMD_HEADER* clb_cmd_headers = (volatile HBA_CMD_HEADER*)clb_base_virtual;

            for (size_t j = 0; j < 32; j++) {
                clb_cmd_headers[j].ctba = (uint32_t)((command_table_physical + 256 * j) & 0xFFFFFFFF);
                clb_cmd_headers[j].ctbau = (uint32_t)(((command_table_physical + 256 * j) >> 32) & 0xFFFFFFFF);
                clb_cmd_headers[j].prdtl = 8;
            }

            hba_port->cmd |= ((1 << 0) | (1 << 4));

            kernel_print("initialized device [%u]: ", i);
            switch (hba_port->sig) {
                case 0x00000101: kernel_print("ATA device\n", i); break;
                case 0xEB140101: kernel_print("ATAPI device\n", i); break;
                case 0xC33C0101: kernel_print("Port Multiplier\n", i); break;
                default: kernel_print("Unknown device type (sig=0x%uh)\n", i, hba_port->sig); return;
            }

            break;
        }
    }

    volatile HBA_CMD_HEADER* clb_cmd_headers = (volatile HBA_CMD_HEADER*)clb_base_virtual;
    volatile HBA_PORT* hba_port = &hba->ports[0];  // Assume port 0 for this example

    // uint8_t identify_buffer[512];
    // uint64_t identify_buffer_physical = kernel::memory::paging::virtual_to_physical(&KPML4T, identify_buffer);

    void* identify_data_unaligned = (void*)((uint64_t)start_driver_virtual + 1024 + 256 + 1024 * 8);
    void* identify_data_virtual = (void*)kernel::memory::align_up_to((uint64_t)identify_data_unaligned, 128);
    uint64_t identify_data_physical = ((uint64_t)start_driver_physical + 1024 + 256 + 1024 * 8);
    // memzero(identify_data_unaligned, 512 + 128);

    // uint64_t identify_data_physical = (uint64_t)kernel::memory::paging::get_page();
    // void* identify_data_virtual = (void*)0x7f000000;
    // kernel::memory::paging::map_large_page(&KPML4T, identify_data_virtual, (void*)identify_data_physical);
    // memzero(identify_data_virtual, PAGE_SIZE);

    // Clear port interrupt status
    hba_port->is = 0xFFFFFFFF;

    // if (((hba_port->ssts & 0x0F) != 0x03)) {
    //     kernel_fatal(0x1234, hba_port->ssts);
    // }

    // Prepare the Command Header
    volatile HBA_CMD_HEADER* cmd_header = &clb_cmd_headers[0];
    cmd_header->cfl = 5;         // Command FIS length (5 DWORDs)
    cmd_header->a = 0;           // ATAPI = 0 (not ATAPI)
    cmd_header->w = 0;           // Write = 0 (Read command)
    cmd_header->prdtl = 1;       // One PRDT entry

    cmd_header->prdbc = 0;       // PRDT byte count
    cmd_header->ctba = (uint32_t)(command_table_physical & 0xFFFFFFFF);
    cmd_header->ctbau = (uint32_t)((command_table_physical >> 32) & 0xFFFFFFFF);

    // Prepare the Command Table
    volatile HBA_CMD_TABLE* cmd_table = (volatile HBA_CMD_TABLE*)command_table_virtual;
    memzero((void*)cmd_table, sizeof(HBA_CMD_TABLE));

    volatile HBA_PRDT_ENTRY* prdt_entry = (volatile HBA_PRDT_ENTRY*)cmd_table->prdt;
    prdt_entry->dba = (uint32_t)(identify_data_physical & 0xFFFFFFFF);
    prdt_entry->dbau = (uint32_t)((identify_data_physical >> 32) & 0xFFFFFFFF);
    prdt_entry->dbc = 512 - 1;    // Byte count (512 bytes, 0-based)
    prdt_entry->i = 1;            // Interrupt on completion

    // Prepare the FIS
    uint8_t* cfis = (uint8_t*)cmd_table->fis;
    cfis[0] = 0x27;               // Host to Device FIS
    cfis[1] = 0x80;               // Command FIS
    cfis[2] = 0xEC;               // IDENTIFY DEVICE command

    // Issue the command
    hba_port->ci = 1 << 0;        // Set Command Issue bit for command 0

    // Wait for the command to complete
    while (hba_port->ci & (1 << 0)) {}  // Wait until the command is done

    // Check if the IDENTIFY DEVICE command was successful
    if (hba_port->is & (1 << 30)) {
        kernel_print("Error during IDENTIFY DEVICE command.\n");
    } else {
        uint16_t* identify_data = (uint16_t*)identify_data_virtual;

        // Extract total disk size from IDENTIFY DEVICE data
        uint32_t total_sectors = (identify_data[100] | (identify_data[101] << 16));
        uint64_t disk_size_bytes = (uint64_t)total_sectors * 512;

        kernel_print("Disk size: %ul bytes\n", disk_size_bytes);

        // Extract the model name from IDENTIFY DEVICE data
        char model_name[41]; // Model name is 40 characters + null terminator
        for (int i = 0; i < 20; i++) {
            model_name[i * 2] = (identify_data[54 + i * 2 + 1]);
            model_name[i * 2 + 1] = (identify_data[54 + i * 2]);
        }
        model_name[40] = '\0'; // Null-terminate the string

        kernel_print("Disk model: %s\n", model_name);
    }







    // volatile HBA_CMD_HEADER* clb_cmd_headers = (volatile HBA_CMD_HEADER*)clb_base_virtual;

    // // Send a simple command (e.g., a Read Status command) to the device and check the response
    // volatile HBA_PORT* hba_port = &hba->ports[0];  // Assume port 0 for this example

    // // Clear any existing interrupts
    // hba_port->is = 0xFFFFFFFF;

    // // Prepare a Command Header
    // volatile HBA_CMD_HEADER* cmd_header = &clb_cmd_headers[0];
    // cmd_header->cfl = 5; // Command FIS length (5 DWORDs for a simple Read Status command)
    // cmd_header->a = 0;   // ATAPI = 0 (not ATAPI)
    // cmd_header->w = 0;   // Write = 0 (Read command)
    // cmd_header->p = 0;   // Prefetchable = 0
    // cmd_header->r = 0;   // Reset = 0
    // cmd_header->b = 0;   // BIST = 0
    // cmd_header->c = 0;   // Clear busy upon R_OK = 0
    // cmd_header->prdtl = 1;  // Number of PRDT entries (1 for a simple read)

    // cmd_header->prdbc = 0;  // PRDT byte count (set to 0 for now)
    // cmd_header->ctba = (uint32_t)((command_table_physical) & 0xFFFFFFFF);
    // cmd_header->ctbau = (uint32_t)(((command_table_physical) >> 32) & 0xFFFFFFFF);

    // // Set the Command List Base Address in the port's CLB
    // hba_port->clb = (uint32_t)(clb_base_physical & 0xFFFFFFFF);
    // hba_port->clbu = (uint32_t)((clb_base_physical >> 32) & 0xFFFFFFFF);

    // // Issue the command (start the command processing)
    // hba_port->ci = 1 << 0;  // Set the Command Issue bit for command 0

    // // Wait for the command to complete
    // while (hba_port->ci & (1 << 0)) {}  // Wait until the command is done

    // // Check the response
    // if (hba_port->is & (1 << 0)) {  // Check if the interrupt status bit is set (successful command)
    //     kernel_print("Device successfully initialized and responded.\n");
    // } else {
    //     kernel_print("Device failed to respond to the command.\n");
    // }






    // volatile HBA_MEM* hba_physical = (volatile HBA_MEM*)ahci_base;

    // uint64_t hba_aligned = kernel::memory::align_up_to((uint64_t)hba_physical, PAGE_SIZE_LARGE) - PAGE_SIZE_LARGE;
    // // uint64_t hba_real = (uint64_t)hba_physical;
    // // auto virtual_hba_page = (void*)(kernel::memory::align_up_to(0x07F00000000, PAGE_SIZE_LARGE));

    // kernel::memory::paging::map_large_page(&KPML4T, (void*)hba_aligned, (void*)hba_aligned);

    // // auto offset_in_page = (hba_real - hba_aligned);
    // // volatile HBA_MEM* hba = (volatile HBA_MEM*)((uint64_t)virtual_hba_page + (hba_real - hba_aligned));
    
    // volatile HBA_MEM* hba = (volatile HBA_MEM*)hba_physical;

    // // hba->GHC |= (1<<31);
    // // hba->GHC |= (1<<0);
    // // while (hba->GHC & (1 << 0));

    // void* clb_base = kernel::memory::vmem::kalloc(1024);
    // // uint8_t* aligned_clb_base = (uint8_t*)(((uint64_t)clb_base + 1023) & ~1023);

    // void* fb_base = kernel::memory::vmem::kalloc(256);
    // // uint8_t* aligned_fb_base = (uint8_t*)(((uint64_t)fb_base + 255) & ~255);

    // for (int i = 0; i < 32; i++) {
    //     if (hba->PI & (1 << i)) {
    //         kernel_print("Port %u is implemented\n", i);
    //         volatile HBA_PORT* port = &hba->ports[i];

    //         port->cmd &= ~((1 << 4) | (1 << 14));
    //         while (port->cmd & ((1 << 4) | (1 << 14))) {}

    //         port->cmd |= (1 << 1);
    //         port->serr = 0xFFFFFFFF;

    //         auto paclbb = kernel::memory::paging::virtual_to_physical(&KPML4T, clb_base);

    //         port->clb = (uint32_t)(paclbb & 0xFFFFFFFF);
    //         port->clbu = (uint32_t)((paclbb >> 32) & 0xFFFFFFFF);

    //         memzero(clb_base, 1024);

    //         port->fb = (uint32_t)(kernel::memory::paging::virtual_to_physical(&KPML4T, fb_base) & 0xFFFFFFFF);
    //         port->fbu = (uint32_t)((kernel::memory::paging::virtual_to_physical(&KPML4T, fb_base) >> 32) & 0xFFFFFFFF);

    //         memzero(fb_base, 256);

    //         port->is = 0xFFFFFFFF;
    //         port->ie = 1;

    //         port->cmd |= (1 << 0);
    //         port->cmd |= (1 << 4);

    //         switch (port->sig) {
    //             case 0x00000101:  // ATA device
    //                 kernel_print("port %u: ATA device\n", i);
    //                 break;
    //             case 0xEB140101:  // ATAPI device (e.g., CD-ROM)
    //                 kernel_print("port %u: ATAPI device\n", i);
    //                 break;
    //             case 0xC33C0101:  // Enclosure management bridge
    //                 kernel_print("port %u: Port Multiplier\n", i);
    //                 break;
    //             default:
    //                 kernel_print("port %u: Unknown device type (sig=0x%uh)\n", i, port->sig);
    //                 return;
    //         }

    //         break;
    //     }
    // }

    // volatile HBA_PORT* ata_device = &hba->ports[0];

    // #define TFD_ERR 0x01
    // #define SSTS_DET_MASK 0x0F
    // #define SSTS_DET_PRESENT 0x03
    // #define SSTS_IPM_MASK 0xF0
    // #define SSTS_IPM_ACTIVE 0x10



    // volatile HBA_CMD_HEADER* cmd_header = (HBA_CMD_HEADER*)clb_base;
    // volatile HBA_CMD_TABLE* cmd_table = (HBA_CMD_TABLE*)kernel::memory::vmem::kalloc(sizeof(HBA_CMD_TABLE));
    // volatile HBA_CMD_TABLE* aligned_cmd_table = (volatile HBA_CMD_TABLE*)kernel::memory::align_up_to((uint64_t)cmd_table, 128);
    // memzero((void*)cmd_table, sizeof(HBA_CMD_TABLE) + 128);

    // cmd_header->cfl = 5;  // Command FIS length (in DWORDs)
    // cmd_header->prdtl = 1;  // One PRDT entry
    // cmd_header->w = 0;
    // cmd_header->ctba = (uint32_t)(kernel::memory::paging::virtual_to_physical(&KPML4T, (void*)aligned_cmd_table) & 0xFFFFFFFF);
    // cmd_header->ctbau = (uint32_t)((kernel::memory::paging::virtual_to_physical(&KPML4T, (void*)aligned_cmd_table) >> 32) & 0xFFFFFFFF);

    // // Allocate the data buffer
    // uint8_t* data_buffer = (uint8_t*)kernel::memory::vmem::kalloc(512);
    // uint8_t* aligned_data_buffer = (uint8_t*)kernel::memory::align_up_to((uint64_t)data_buffer, 8);

    // HBA_PRDT_ENTRY* prdt_entry = (HBA_PRDT_ENTRY*)&aligned_cmd_table->prdt[0];
    // prdt_entry->dba = (uint32_t)(kernel::memory::paging::virtual_to_physical(&KPML4T, (void*)aligned_data_buffer) & 0xFFFFFFFF);
    // prdt_entry->dbau = (uint32_t)(kernel::memory::paging::virtual_to_physical(&KPML4T, (void*)aligned_data_buffer) >> 32);
    // prdt_entry->dbc = 512 - 1;
    // prdt_entry->i = 1;

    // // Set up the FIS (example for Identify Device)
    // aligned_cmd_table->fis[0] = 0x27;    // Host to Device FIS (write)
    // aligned_cmd_table->fis[1] = 0x00;    // Reserved
    // aligned_cmd_table->fis[2] = 0xEC;    // Identify Device command
    // aligned_cmd_table->fis[3] = 0x00;    // LBA Low (0 for Identify Device)
    // aligned_cmd_table->fis[4] = 0x00;    // LBA Mid (0 for Identify Device)
    // aligned_cmd_table->fis[5] = 0x00;    // LBA High (0 for Identify Device)
    // aligned_cmd_table->fis[6] = 0xA0;    // Device Register (should be 0xA0 for the Identify Device command)
    // aligned_cmd_table->fis[7] = 0x00;    // Reserved

    // ata_device->ci = (1 << 0);

    // uint64_t timeout = 1000000;
    // while ((ata_device->ci & (1 << 0)) && timeout--) {
    //     if (ata_device->tfd & TFD_ERR) {
    //         // kernel_fatal("Command failed: Task File Data error.");
    //         kernel_assert(false, 0x10001);
    //     }
    // }
    // if (timeout == 0) {
    //     // kernel_fatal("Command timeout: Identify Device did not complete.");
    //     kernel_assert(false, 0x10002);
    // }

    // kernel_print("Port status: 0x%08X\n", ata_device->ssts);
    // if ((ata_device->ssts & 0xF) != 0x3) {  // 0x3 means "Active" state
    //     kernel_print("Port not ready, skipping command\n");
    // }

    // if ((ata_device->ssts & SSTS_DET_MASK) != SSTS_DET_PRESENT) {
    //     kernel_assert(false, 0x10003);
    // }
    // if ((ata_device->ssts & SSTS_IPM_MASK) != SSTS_IPM_ACTIVE) {
    //     kernel_assert(false, 0x10004);
    // }


    // while (ata_device->ci & (1 << 0)) {
    //     kernel_print("Waiting for command completion...\n");
    // }

    // if (ata_device->tfd & (1 << 0)) {
    //     kernel_print("Error in command execution\n");
    // }

    // if (ata_device->is & 0xFFFFFFFF) {
    //     kernel_print("Interrupt occurred: 0x%uh\n", ata_device->is);
    //     ata_device->is = 0xFFFFFFFF;  // Clear all interrupt status flags
    // }

    // kernel_print("done\n");

    // uint16_t* identify_data = (uint16_t*)aligned_data_buffer;
    // kernel_print("Model: ");
    // kernel_print("%c", identify_data[0]);
    // kernel_print("%c", identify_data[1]);
    // kernel_print("-");
    // for (int i = 27; i <= 46; i++) {
    //     char char1 = (identify_data[i] >> 8) & 0xFF; // High byte
    //     char char2 = identify_data[i] & 0xFF;        // Low byte
    //     if (char1) kernel_print("%c", char1);
    //     if (char2) kernel_print("%c", char2);
    // }
    // kernel_print("\n");

*/