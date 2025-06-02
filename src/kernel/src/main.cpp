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

void draw_logo_vga_tm() {
    constexpr uint32_t x = 29;
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

cpu_state_t* handle_other_interrupt(uint64_t code, cpu_state_t* rsp) {
    critical_fatal_ex(code, "FATAL: unhandled interrupt", rsp);
    return rsp;
}

void thread_test() {
    debug_print("test\n");
    while (true) {}
}

void sata_init(void* pml4);

extern "C" void kernel_entry(multiboot_t* multiboot_struct, void* kpml4) {
    debug_init();

    hc::gdt_tss::init();

    vga_tm_clear_screen();
    draw_logo_vga_tm();
    vga_tm_set_cursor(29, 20);
    vga_tm_print("initializing kernel ...");

    if (!mb_has_valid_magic(multiboot_struct))
        critical_fatal(multiboot_struct->magic, "multiboot magic was invalid");

    if (!vmem_init(multiboot_struct, kpml4))
        critical_fatal(0x0, "vmem_init failed");

    heap_t heap {};
    if (!heap_init(&heap, kpml4, (void*)0x40000000, 0x100000 * 32))
        critical_fatal(0x0, "heap_init failed");
    set_global_heap(&heap);

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

    sata_init(kpml4);

    // manual (main) thread setup
    vthread_t main_thread {};
    main_thread.vt_state = vthread_state_t::RUNNING;
    main_thread.vtid = 0;
    ((tls_base_t*)main_thread.tls)->vtid = 0;
    vthread_add(&main_thread);

    vthread_t thread{};
    vthread_create(&thread, thread_test);

    // disabled since we cant draw anything yet (other than a pixel)
    // vga_generic_buffer_t buffer{};
    // vga_gm_buffer_create(&buffer);
    // vga_gm_startup(&buffer);

    debug_print("[INFO]: kernel finished initializing\n");

    const char* spinner[] = { ".  ", " . ", "  ." };
    constexpr size_t chars_size = (sizeof(spinner) / sizeof(char*));
    size_t i = 0;
    while (true) {
        vga_tm_set_cursor(49, 20);
        vga_tm_print("%s", spinner[i++ % chars_size]);
        vga_tm_set_cursor(VGA_TM_NUM_COLS, VGA_TM_NUM_ROWS);
        pit_sleep(250);
    }
}

struct HBA_PORT {
	uint32_t clb;		// 0x00, command list base address, 1K-byte aligned
	uint32_t clbu;		// 0x04, command list base address upper 32 bits
	uint32_t fb;		// 0x08, FIS base address, 256-byte aligned
	uint32_t fbu;		// 0x0C, FIS base address upper 32 bits
	uint32_t is;		// 0x10, interrupt status
	uint32_t ie;		// 0x14, interrupt enable
	uint32_t cmd;		// 0x18, command and status
	uint32_t rsv0;		// 0x1C, Reserved
	uint32_t tfd;		// 0x20, task file data
	uint32_t sig;		// 0x24, signature
	uint32_t ssts;		// 0x28, SATA status (SCR0:SStatus)
	uint32_t sctl;		// 0x2C, SATA control (SCR2:SControl)
	uint32_t serr;		// 0x30, SATA error (SCR1:SError)
	uint32_t sact;		// 0x34, SATA active (SCR3:SActive)
	uint32_t ci;		// 0x38, command issue
	uint32_t sntf;		// 0x3C, SATA notification (SCR4:SNotification)
	uint32_t fbs;		// 0x40, FIS-based switch control
	uint32_t rsv1[11];	// 0x44 ~ 0x6F, Reserved
	uint32_t vendor[4];	// 0x70 ~ 0x7F, vendor specific
};

struct HBA_MEM {
	// 0x00 - 0x2B, Generic Host Control
	uint32_t cap;		// 0x00, Host capability
	uint32_t ghc;		// 0x04, Global host control
	uint32_t is;		// 0x08, Interrupt status
	uint32_t pi;		// 0x0C, Port implemented
	uint32_t vs;		// 0x10, Version
	uint32_t ccc_ctl;	// 0x14, Command completion coalescing control
	uint32_t ccc_pts;	// 0x18, Command completion coalescing ports
	uint32_t em_loc;		// 0x1C, Enclosure management location
	uint32_t em_ctl;		// 0x20, Enclosure management control
	uint32_t cap2;		// 0x24, Host capabilities extended
	uint32_t bohc;		// 0x28, BIOS/OS handoff control and status

	// 0x2C - 0x9F, Reserved
	uint8_t  rsv[0xA0-0x2C];

	// 0xA0 - 0xFF, Vendor specific registers
	uint8_t  vendor[0x100-0xA0];

	// 0x100 - 0x10FF, Port control registers
	HBA_PORT	ports[1];	// 1 ~ 32
};

struct HBA_CMD_HEADER {
    // DW0
    uint8_t  cfl     : 5;  // Command FIS length in DWORDS (2 DWORDs = 8 bytes min)
    uint8_t  a       : 1;  // ATAPI
    uint8_t  w       : 1;  // Write (1 = write, 0 = read)
    uint8_t  p       : 1;  // Prefetchable

    uint8_t  r       : 1;  // Reset
    uint8_t  b       : 1;  // BIST
    uint8_t  c       : 1;  // Clear Busy upon R_OK
    uint8_t  rsv0    : 1;  // Reserved
    uint8_t  pmp     : 4;  // Port multiplier port

    uint16_t prdtl;        // Physical region descriptor table length (in entries)

    // DW1
    volatile uint32_t prdbc;   // Physical region descriptor byte count transferred

    // DW2/DW3
    uint32_t ctba;         // Command table descriptor base address
    uint32_t ctbau;        // Command table descriptor base address upper 32 bits

    // DW4–DW7
    uint32_t rsv1[4];      // Reserved
} PACKED;

struct FIS_REG_H2D {
    // DWORD 0
    uint8_t  fis_type;    // FIS_TYPE_REG_H2D = 0x27

    uint8_t  pmport : 4;  // Port multiplier
    uint8_t  rsv0   : 3;  // Reserved
    uint8_t  c      : 1;  // 1: Command, 0: Control

    uint8_t  command;     // ATA command (e.g. 0xEC for IDENTIFY DEVICE)
    uint8_t  featurel;    // Feature (low byte)

    // DWORD 1
    uint8_t  lba0;        // LBA low (7:0)
    uint8_t  lba1;        // LBA mid (15:8)
    uint8_t  lba2;        // LBA high (23:16)
    uint8_t  device;      // Device register

    // DWORD 2
    uint8_t  lba3;        // LBA (31:24)
    uint8_t  lba4;        // LBA (39:32)
    uint8_t  lba5;        // LBA (47:40)
    uint8_t  featureh;    // Feature (high byte)

    // DWORD 3
    uint8_t  countl;      // Sector count (low byte)
    uint8_t  counth;      // Sector count (high byte)
    uint8_t  icc;         // Isochronous Command Completion
    uint8_t  control;     // Control register

    // DWORD 4
    uint8_t  rsv1[4];     // Reserved
} PACKED;

struct HBA_CMD_TBL {
    uint8_t  cfis[64];        // 0x00: Command FIS (Host to Device FIS)
    uint8_t  acmd[16];        // 0x40: ATAPI command (12 or 16 bytes)
    uint8_t  rsv[48];         // 0x50: Reserved

    // 0x80: Physical Region Descriptor Table (PRDT) — array of up to 65535 entries
    // The number of PRDT entries used is specified in the Command Header (prdtl)
    struct HBA_PRDT_ENTRY {
        uint32_t dba;         // Data base address
        uint32_t dbau;        // Data base address upper 32 bits
        uint32_t rsv0;

        // DW3
        uint32_t dbc : 22;    // Byte count (0-based: 0 = 1 byte, 0x3FFFF = 4 MiB)
        uint32_t rsv1 : 9;
        uint32_t i    : 1;    // Interrupt on completion
    } prdt_entry[1];          // This can be an array with max 65535 entries
} PACKED;

void sata_init(void* pml4) {
    dma_heap_t dma_heap {};
    (void)dma_heap_init(pml4, &dma_heap, (void*)0x3FC00000);

    // get pcie devices
    vector<pci_device_info_t> pci_devices {};
    pci_enumerate_devices(&pci_devices);

    // find sata controller
    vector<pci_device_request_t> pci_devices_requested {};
    pci_devices_requested.insert_back(pci_device_request_t { .revision_id = (uint8_t)PCI_UNKNOWN, .prog_if = (uint8_t)1, .sub_class = 6, .class_code = 1 });
    const bool found = pci_find_devices(&pci_devices, &pci_devices_requested);

    if (!found) {
        debug_print("AHCI device not found\n");
        return;
    }

    const pci_device_info_t* ahci_device = pci_devices.get_at(pci_devices_requested.get_at(0)->pci_device_index);
    
    // map hba_mem / bar5
    uint64_t abar_phys = ahci_device->bar5_address & ~0xF;
    uint64_t abar_page = mem_align_down(abar_phys, PAGE_SIZE_LARGE);
    uint64_t abar_offset = abar_phys - abar_page;

    (void)vmem_map_2mb_page(pml4, (void*)0x3fe00000, (void*)abar_page);
    volatile HBA_MEM* hba = (volatile HBA_MEM*)((uint8_t*)0x3fe00000 + abar_offset);

    // get version
    uint32_t version = hba->vs;
    debug_print("AHCI Controller version: 0x%uh\n", version);

    // get ports
    uint32_t ports_implemented = hba->pi;
    debug_print("Ports Implemented: 0x%uh\n", ports_implemented);

    for (int i = 0; i < 32; i++) {
        if (ports_implemented & (1 << i)) {
            volatile HBA_PORT* port = &hba->ports[i];
            debug_print("port[%i] SATA status: 0x%uh; signature: 0x%uh \n", i, port->ssts, port->sig);
        }
    }

    // assume port 0 has a drive
    volatile HBA_PORT* port = &hba->ports[0];

    // reset device
    // Clear ST (bit 0)
    port->cmd &= ~0x01;

    // Wait for CR to clear again
    while (port->cmd & (1 << 15));

    // Clear FRE (bit 4)
    port->cmd &= ~(1 << 4);

    // setup commands

    // Allocate 4 KB-aligned pages for CLB and RFIS
    auto clb = dma_heap_alloc(&dma_heap);   // 1 KB Command List Base
    auto rfis = dma_heap_alloc(&dma_heap);  // 256-byte Received FIS

    memset(clb, 0, 4096);
    memset(rfis, 0, 4096);

    port->clb = dma_get_physical_lower(&dma_heap, clb);
    port->clbu = dma_get_physical_upper(&dma_heap, clb);
    port->fb = dma_get_physical_lower(&dma_heap, rfis);
    port->fbu = dma_get_physical_upper(&dma_heap, rfis);

    port->ie = 0;  // Disable interrupts (for now)

    port->cmd |= (1 << 4);  // FRE
    port->cmd |= (1 << 0);  // ST

    ////////////////////////////////////////////////////////////////////////////

    // dma_memory_region_t::block_t* dma_buf = dma_heap_alloc(&dma_heap);
    // memset(dma_buf, 0, 0x1000);
    // uint64_t dma_physl = dma_get_physical_lower(&dma_heap, dma_buf);
    // uint64_t dma_physu = dma_get_physical_upper(&dma_heap, dma_buf);

    // HBA_CMD_HEADER* cmdhdr = (HBA_CMD_HEADER*)clb;
    // memset(cmdhdr, 0, sizeof(HBA_CMD_HEADER));

    // cmdhdr->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    // cmdhdr->w = 0; // Read
    // cmdhdr->prdtl = 1;


    // HBA_CMD_TBL* cmdtbl = (HBA_CMD_TBL*)dma_heap_alloc(&dma_heap);
    // memset(cmdtbl, 0, sizeof(HBA_CMD_TBL));
    // uint64_t cmdtbl_physl = dma_get_physical_lower(&dma_heap, (dma_memory_region_t::block_t*)cmdtbl);
    // uint64_t cmdtbl_physu = dma_get_physical_upper(&dma_heap, (dma_memory_region_t::block_t*)cmdtbl);

    // cmdhdr->ctba = cmdtbl_physl;
    // cmdhdr->ctbau = cmdtbl_physu;

    // constexpr uint32_t sector_count = 8;

    // cmdtbl->prdt_entry[0].dba = dma_physl;
    // cmdtbl->prdt_entry[0].dbau = dma_physu;
    // cmdtbl->prdt_entry[0].dbc = 512 * (sector_count) - 1;
    // cmdtbl->prdt_entry[0].i = 1;

    // constexpr uint64_t lba = 0;

    // FIS_REG_H2D* fis = (FIS_REG_H2D*)(&cmdtbl->cfis);
    // memset(fis, 0, sizeof(FIS_REG_H2D));
    // fis->fis_type = 0x27;
    // fis->c = 1;
    // fis->command = 0x25; // ATA_CMD_READ_DMA_EXT;
    // fis->device = 0x40; // LBA mode

    // fis->lba0 = (uint8_t)(lba);
    // fis->lba1 = (uint8_t)(lba >> 8);
    // fis->lba2 = (uint8_t)(lba >> 16);
    // fis->lba3 = (uint8_t)(lba >> 24);
    // fis->lba4 = (uint8_t)(lba >> 32);
    // fis->lba5 = (uint8_t)(lba >> 40);

    // fis->countl = (uint8_t)(sector_count);
    // fis->counth = (uint8_t)(sector_count >> 8);

    // // Clear interrupts
    // port->is = (uint32_t)-1;

    // // Wait until CR is cleared
    // // while (port->cmd & (1 << 15));

    // // Start command engine if not running
    // if (!(port->cmd & (1 << 4))) {
    //     port->cmd |= (1 << 4); // FRE
    //     port->cmd |= (1 << 0); // ST
    // }

    // // Issue command slot 0
    // port->ci = 1;

    // // Wait for completion
    // while (port->ci & 1) {
    //     if (port->is & (1 << 30)) {
    //         debug_print("AHCI read: Task file error");
    //         return;
    //     }
    // }

    // if (port->is & (1 << 30)) {
	// 	debug_print("Read disk error\n");
	// 	return;
	// }

    // uint8_t* data = (uint8_t*)dma_buf;
    // for (int i = 0; i < 512*sector_count; ++i) {
    //     debug_print("%uh ", data[i]);
    //     if ((i + 1) % 16 == 0) debug_print("\n");
    // }

    // debug_print("START LBA: %u\n", *(uint32_t*)&data[454]);
    // debug_print("TOTAL SECTORS: %u\n", *(uint32_t*)&data[458]);

    // Command List entry
    HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)clb;
    cmdheader[0].cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);  // Command FIS length in DWORDs
    cmdheader[0].w = 0;     // READ = 0, WRITE = 1
    cmdheader[0].prdtl = 1; // 1 PRDT

    // Allocate Command Table (256 + PRDT entries)
    auto cmdtbl = dma_heap_alloc(&dma_heap);
    memset(cmdtbl, 0, 4096);
    cmdheader[0].ctba = dma_get_physical_lower(&dma_heap, cmdtbl);
    cmdheader[0].ctbau = dma_get_physical_upper(&dma_heap, cmdtbl);

    // Fill PRDT to receive 512 bytes
    HBA_CMD_TBL* tbl = (HBA_CMD_TBL*)cmdtbl;
    auto identify_buf = dma_heap_alloc(&dma_heap);
    tbl->prdt_entry[0].dba = dma_get_physical_lower(&dma_heap, identify_buf);
    tbl->prdt_entry[0].dbau = dma_get_physical_upper(&dma_heap, identify_buf);
    tbl->prdt_entry[0].dbc = 512 - 1;
    tbl->prdt_entry[0].i = 0;  // Interrupt on completion

    FIS_REG_H2D* fis = (FIS_REG_H2D*)(&tbl->cfis);
    fis->fis_type = 0x27; // FIS_TYPE_REG_H2D;
    fis->c = 1;  // command
    fis->command = 0xEC;  // IDENTIFY DEVICE
    fis->device = 0;      // Drive 0

    port->ci = 1 << 0;  // Command slot 0

    // Wait for completion
    while (port->ci & (1 << 0)) {
        // optional: timeout
    }

    // Cast it to 16-bit words (ATA spec is in words)
    uint16_t identify_words[256];
    for (int i = 0; i < 256; ++i)
        identify_words[i] = ((uint8_t*)identify_buf)[2*i] | (((uint8_t*)identify_buf)[2*i + 1] << 8);

    // read results
    char model[41];
    memcpy(model, (char*)identify_buf + 54, 40);
    model[40] = '\0';

    // Convert from ATA's word-swapped format
    for (int i = 0; i < 40; i += 2) {
        char tmp = model[i];
        model[i] = model[i + 1];
        model[i + 1] = tmp;
    }

    debug_print("port[0] Drive Model: %s\n", model);

    uint32_t logical_sector_size = (uint32_t)identify_words[117] | ((uint32_t)identify_words[118] << 16);

    if (logical_sector_size == 0) {
        logical_sector_size = 512; // fallback default
    }

    // Word 106 gives physical sector info
    uint16_t word106 = identify_words[106];

    uint32_t physical_sector_size = logical_sector_size; // default assumption

    // Check if word 106 indicates multiple logical per physical
    if (word106 & (1 << 13)) {
        uint8_t log2_multiple = word106 & 0x1F; // Bits 4–0
        physical_sector_size = logical_sector_size << log2_multiple; // Multiply by 2^n
    }

    debug_print("Logical Sector Size: %i bytes\n", logical_sector_size);
    debug_print("Physical Sector Size: %i bytes\n", physical_sector_size);

    // uint8_t* data = (uint8_t*)identify_buf;
    // for (int i = 0; i < 512; ++i) {
    //     debug_print("%uh ", data[i]);
    //     if ((i + 1) % 16 == 0) debug_print("\n");
    // }
}

/*

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Trim leading/trailing spaces from a fixed-size string
void trim(char* str) {
    int len = strlen(str);
    int start = 0, end = len - 1;

    // Trim leading spaces
    while (start < len && str[start] == ' ') start++;

    // Trim trailing spaces
    while (end >= start && str[end] == ' ') end--;

    // Shift and null-terminate
    int new_len = end - start + 1;
    if (new_len <= 0) {
        str[0] = '\0';
    } else {
        memmove(str, str + start, new_len);
        str[new_len] = '\0';
    }
}

// Convert 16-bit word string (big-endian chars) to C string
void decode_string(const uint16_t* src, int word_count, char* dest, int max_len) {
    int pos = 0;
    for (int i = 0; i < word_count && pos + 1 < max_len; ++i) {
        dest[pos++] = (char)(src[i] >> 8);
        dest[pos++] = (char)(src[i] & 0xFF);
    }
    dest[pos] = '\0';
    trim(dest);
}

// Convert bytes to human-readable size
void human_readable_size(uint64_t bytes, char* out, int out_size) {
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    int unit_index = 0;
    double size = (double)bytes;

    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }

    snprintf(out, out_size, "%.2f %s", size, units[unit_index]);
}

// Main parser function
void parse_identify_device(const uint8_t* buffer) {
    const uint16_t* data = (const uint16_t*)buffer;

    char model[41], serial[21], firmware[9];

    decode_string(&data[27], 20, model, sizeof(model));
    decode_string(&data[10], 10, serial, sizeof(serial));
    decode_string(&data[23], 4, firmware, sizeof(firmware));

    // LBA28 fallback
    uint64_t total_lba = (uint32_t)data[60] | ((uint32_t)data[61] << 16);

    // Check for 48-bit LBA
    if (data[83] & (1 << 10)) {
        total_lba = ((uint64_t)data[100]) |
                    ((uint64_t)data[101] << 16) |
                    ((uint64_t)data[102] << 32) |
                    ((uint64_t)data[103] << 48);
    }

    // Logical sector size
    uint32_t logical_sector_size = (uint32_t)data[117] | ((uint32_t)data[118] << 16);
    if (logical_sector_size == 0) logical_sector_size = 512;

    // Physical sector size
    uint32_t physical_sector_size = logical_sector_size;
    uint16_t word106 = data[106];
    if (word106 & (1 << 13)) {
        uint8_t log2_multiple = word106 & 0x1F;
        physical_sector_size = logical_sector_size << log2_multiple;
    }

    uint64_t capacity_bytes = total_lba * (uint64_t)logical_sector_size;

    char size_str[32];
    human_readable_size(capacity_bytes, size_str, sizeof(size_str));

    // Print results
    printf("Model Number       : %s\n", model);
    printf("Serial Number      : %s\n", serial);
    printf("Firmware Revision  : %s\n", firmware);
    printf("Total LBA Sectors  : %llu\n", (unsigned long long)total_lba);
    printf("Drive Capacity     : %llu bytes (%s)\n",
           (unsigned long long)capacity_bytes, size_str);
    printf("Logical Sector Size: %u bytes\n", logical_sector_size);
    printf("Physical Sector Size: %u bytes\n", physical_sector_size);
}


*/