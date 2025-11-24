#include "interrupt_manager.hpp"
#include "std/map.hpp"
#include "utils/bitmap.hpp"
#include "crash_handler.hpp"
#include "arch/interrupt.hpp"
#include "io.hpp"

static interrupt_callback_t global_interrupt_callback_array[(size_t)interrupt_t::SIZE] { nullptr };

bool is_interrupt_exception(uint64_t code) {
    return (code >= 0 && code <= 21);
}

bool is_interrupt_exception(interrupt_t code) {
    return is_interrupt_exception((uint64_t)code);
}

bool is_interrupt_hardware(uint64_t code) {
    code -= 10;
    return (code >= 22 && code <= 37);
}

bool is_interrupt_hardware(interrupt_t code) {
    return is_interrupt_hardware((uint64_t)code + 10);
}

interrupt_t convert_to_interrupt(uint64_t code) {
    // exceptions
    if (is_interrupt_exception(code))
        return (interrupt_t)code;

    // hardware
    if (is_interrupt_hardware(code))
        return (interrupt_t)(code - 10);

    // software
    if (code >= 48 && code <= 255) {
        switch (code) {
            case 128: return interrupt_t::SOFTWARE_SYSTEMCALL;
            case 129: return interrupt_t::SOFTWARE_SCHEDULER;
            case 130: return interrupt_t::SOFTWARE_CRASH_HANDLER;
        }
    }

    return interrupt_t::UNKOWN;
}

bool set_interrupt_callback(interrupt_t code, interrupt_callback_t callback) {
    if ((uint64_t)code > 255)
        return false;

    if (global_interrupt_callback_array[(size_t)code])
        return false;

    global_interrupt_callback_array[(size_t)code] = callback;
    return true;
}

uint64_t interrupt_irq_to_int(uint64_t irq) {
    // very simple for now :)
    return irq + 32;
}

void* handle_interrupt(uint64_t code, cpu_state_t* p_rsp) {
    const auto interrupt_type = convert_to_interrupt(code);

    if (is_interrupt_exception(interrupt_type))
        __kernel_fatal(code, "critical interrupt triggerd", p_rsp);

    if (is_interrupt_hardware(interrupt_type)) {
        if (auto callback = global_interrupt_callback_array[(size_t)interrupt_type]) {
            p_rsp = callback(p_rsp);
            interrupt_send_eoi(code - 0x20);
            return p_rsp;
        }

        // still eoi but dont early end it
        interrupt_send_eoi(code - 0x20);
    }

    // FIXME @since 08/09/2025 -- 18:53
    // make better handler for software
    switch (interrupt_type) {
        case interrupt_t::SOFTWARE_SCHEDULER:
        case interrupt_t::SOFTWARE_CRASH_HANDLER:
        case interrupt_t::SOFTWARE_SYSTEMCALL: {
            if (auto callback = global_interrupt_callback_array[(size_t)interrupt_type])
                return callback(p_rsp);
            break;
        }
    }

    kprintf("unhandled interrupt triggerd: 0x%uh\n", code);
    return p_rsp;
}