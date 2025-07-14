#include "drivers/ps2/keyboard.hpp"
#include "utils/debug.hpp"
#include "arch/generic.hpp"

static uint64_t g_last_scan_code = 0;

void ps2_keyboard_handle_interrupt() {
    // TODO @since 14/07/2025 -- 18:59
    in_port<uint8_t>(0x60);
}

uint64_t ps2_keyboard_get_last_scancode() {
    return g_last_scan_code;
}

void ps2_keyboard_clear_last_scancode() {
    g_last_scan_code = 0;
}