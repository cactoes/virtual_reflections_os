#include "arch/generic.hpp"
#include "arch/gdt.hpp"
#include "arch/interrupt.hpp"
#include "arch/pit.hpp"

#include "drivers/vga.hpp"
#include "drivers/pcie.hpp"
#include "drivers/ps2/keyboard.hpp"
#include "drivers/ps2/mouse.hpp"

#include "memory/vmem.hpp"
#include "memory/heap.hpp"

#include "utils/debug.hpp"
#include "utils/vector.hpp"
#include "utils/event.hpp"

#include "multiboot.hpp"
#include "string.hpp"
#include "common.hpp"
#include "crash_handler.hpp"

enum print_mode_t {
    STD,
    DBG
};

void printf(print_mode_t mode, const char* p_str, ...) {
    char buffer[256] = { 0 };

    va_list args;
    va_start(args, p_str);
    size_t strlen = sprintf(buffer, (unsigned long int)sizeof(buffer), p_str, args);
    va_end(args);

    switch (mode) {
        case DBG:
            debug_puts(buffer);
            break;
        case STD:
        default:
            vga_tm_puts(&g_vga_tm_buffer, buffer);
            break;
    }
}

void pit_handle_interrupt() {
    // TODO @since 14/07/2025 -- 18:59
}

void* interrupt_handler(uint64_t code, cpu_state_t* p_rsp) {
    if (code >= 0 && code <= 0x15)
        __kernel_fatal(code, "critical interrupt triggerd", p_rsp);

    if (code >= 0x20 && code < 0x2F) {
        switch (code) {
            case 0x20:
                pit_handle_interrupt();
                interrupt_send_eoi(X86_64_INT_IRQ_PIT);
                return p_rsp;
            case 0x21:
                ps2_keyboard_handle_interrupt();
                interrupt_send_eoi(X86_64_INT_IRQ_PS2_KEYBOARD);
                return p_rsp;
            case 0x2C:
                ps2_mouse_handle_interrupt();
                interrupt_send_eoi(X86_64_INT_IRQ_PS2_MOUSE);
                return p_rsp;
            default:
                interrupt_send_eoi(code - 0x20);
        }
    }

    printf(DBG, "unkown interrupt triggerd: 0x%uh\n", code);
    return p_rsp;
}

// void trigger_pf() {
//     volatile int* ptr = (int*)0x1234564478;
//     int val = *ptr;
// }

// void trigger_gpf() {
//     asm volatile (
//         "mov $0xFFFF, %%ax\n"
//         "ltr %%ax\n"
//         :
//         :
//         : "rax"
//     );
// }

void on_key_down(const ps2_key_state_t* p_state) {
    static char s_ascii_table[128] = {
        0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
        '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
        0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-',
        '4', '5', '6', '+', '1', '2', '3', '0', '.'
    };

    if (p_state->scan_code > ARRAY_SIZE(s_ascii_table))
        return;

    printf(STD, "%c", s_ascii_table[p_state->scan_code]);
}

extern "C" void kernel_entry(void* p_multiboot_struct, void* p_kpml4) {
    // validate multiboot
    UNUSED(mb_has_valid_magic((multiboot_t*)p_multiboot_struct));

    // initialize the gdt / tss
    gdt_init();

    // initialze vga text mode
    vga_tm_init_buffer(&g_vga_tm_buffer, (void*)VGA_TM_BUFFER_ADDR, VGA_TM_NUM_COLS, VGA_TM_NUM_ROWS);
    vga_tm_clear_buffer(&g_vga_tm_buffer);

    // initialze the debug out stream
    debug_init();

    // initialze virtual memory
    UNUSED(vmem_init(p_multiboot_struct, p_kpml4));
    vmem_identity_map(p_kpml4);
    set_pml4(p_kpml4);

    // initialze the global heap
    heap_t heap {};
    UNUSED(heap_init(&heap, p_kpml4, (void*)VMEM_HEAP_START_ADDR, 0x100000 * 32));
    set_global_heap(&heap);

    // initialze the interrupt line(s)
    interrupt_set_handler(interrupt_handler);
    ps2_mouse_init();
    pit_init(1000);
    interrupt_init(gdt_get_kernel_code_selector());

    // TODO @since 14/07/2025 -- 18:58
    // threads / processes

    // TODO @since 14/07/2025 -- 18:58
    // pci(e)
    vector<pci_device_t> pci_devices {};
    pci_enumerate_devices(&pci_devices);

    for (auto& device : pci_devices) {
        const char* cd = pci_get_class_description(&device);
        printf(DBG, "[+] %s\n", cd);
    }

    pci_class_info_t ahci_device_class_info {
        .revision_id = (uint8_t)PCI_UNKNOWN,
        .prog_if = (uint8_t)1,
        .sub_class = (uint8_t)6,
        .class_code = (uint8_t)1
    };
    const pci_device_t* ahci_controller = pci_find_device(&pci_devices, &ahci_device_class_info);
    // TODO @since 14/07/2025 -- 21:52
    
    pci_class_info_t ide_device_class_info {
        .revision_id = (uint8_t)PCI_UNKNOWN,
        .prog_if = (uint8_t)PCI_UNKNOWN,
        .sub_class = (uint8_t)1,
        .class_code = (uint8_t)1
    };
    const pci_device_t* ide_controller = pci_find_device(&pci_devices, &ide_device_class_info);
    // TODO @since 14/07/2025 -- 21:52
    
    pci_class_info_t network_device_class_info {
        .revision_id = (uint8_t)PCI_UNKNOWN,
        .prog_if = (uint8_t)PCI_UNKNOWN,
        .sub_class = (uint8_t)0,
        .class_code = (uint8_t)2
    };
    const pci_device_t* network_controller = pci_find_device(&pci_devices, &network_device_class_info);
    // TODO @since 14/07/2025 -- 21:52

    // TODO @since 14/07/2025 -- 18:58
    // setup vfs

    ps2_keyboard_event_subscribe(on_key_down);

    // kernel finished
    printf(STD, "> SYSTEM READY\n");
    printf(DBG, "Kernel finished initializing\n");

    // we shoudn t reach this point since the kernel should never stop
    // incase we do just hang here so we dont break anything
    while (true);
}