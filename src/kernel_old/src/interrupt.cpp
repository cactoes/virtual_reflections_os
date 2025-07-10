#include "interrupt.hpp"
#include "cpu.hpp"
#include "hardware_compatibility.hpp"

static interrupt_callback g_int_cb_array[INT_TYPE_COUNT] {};

cpu_state_t* int_handler(uint64_t code, cpu_state_t* rsp) {
    switch (code) {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3:
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7:
        case 0x8:
        case 0x9:
        case 0xA:
        case 0xB:
        case 0xC:
        case 0xD:
        case 0xE:
        case 0xF:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x14:
        case 0x15: { // end of "basic" interrupts
            if (const auto callback = g_int_cb_array[INT_TYPE_CAST(interrupt_type::CRITICAL)])
                rsp = callback(code, rsp);
            break;
        }
        case 0x20: { // PIT
            if (const auto callback = g_int_cb_array[INT_TYPE_CAST(interrupt_type::PIT)])
                rsp = callback(code, rsp);
            hc::interrupt::pic_send_eoi(INT_IRQ_PIT);
            break;
        }
        case 0x21: { // keyboard
            if (const auto callback = g_int_cb_array[INT_TYPE_CAST(interrupt_type::KEYBOARD)])
                rsp = callback(code, rsp);
            hc::interrupt::pic_send_eoi(INT_IRQ_PS2_KEYBOARD);
            break;
        }
        case 0x2C: { // mouse
            if (const auto callback = g_int_cb_array[INT_TYPE_CAST(interrupt_type::MOUSE)])
                rsp = callback(code, rsp);
            hc::interrupt::pic_send_eoi(INT_IRQ_PS2_MOUSE);
            break;
        }
        default: {
            if (const auto callback = g_int_cb_array[INT_TYPE_CAST(interrupt_type::OTHER)])
                rsp = callback(code, rsp);
            break;
        }
    }

    return rsp;
}

void int_init() {
    uint8_t irqs[] = {
        INT_IRQ_PIT,
        INT_IRQ_PS2_KEYBOARD,
        INT_IRQ_PS2_MOUSE
    };

    hc::interrupt::init(int_handler, irqs, sizeof(irqs) / sizeof(uint8_t));
}

void int_set_callback(interrupt_type type, interrupt_callback int_cb) {
    g_int_cb_array[INT_TYPE_CAST(type)] = int_cb;
}