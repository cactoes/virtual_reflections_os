#include "drivers/keyboard_driver.hpp"
#include "cpu.hpp"

extern void critical_fatal(uint64_t code, const char* message);

static key_state_t* g_key_states = nullptr;
static event_manager_t<key_state_t*> g_keyboard_event_manager {};
static uint64_t g_scan_code = 0;

void keyboard_init(key_state_t keystates[KEY_STATE_ARRAY_SIZE]) {
    g_key_states = keystates;
}

key_state_t* keyboard_get_key_state(uint64_t scan_code) {
    if (scan_code >= KEY_STATE_ARRAY_SIZE)
        return nullptr;

    return &g_key_states[scan_code];
}

cpu_state_t* keyboard_handle_interrupt(uint64_t code, cpu_state_t* rsp) {
    key_state_t key_state {};
    key_state.full_code = cpu_inb(KEYBOARD_PORT_SCANCODE);

    if (key_state.full_code == FULL_CODE_ESCAPED) {
        key_state.is_escaped = true;
        key_state.full_code = cpu_inb(KEYBOARD_PORT_SCANCODE);
    }

    key_state.scan_code = (key_state.full_code & SCAN_CODE_MASK_CHAR);
    key_state.is_released = (key_state.full_code & SCAN_CODE_MASK_PRESSED) != 0;
    key_state.is_pressed = (key_state.full_code & SCAN_CODE_MASK_PRESSED) == 0;

    if (key_state.scan_code < KEY_STATE_ARRAY_SIZE) {
        key_state.is_shift = key_state.scan_code == SC_LSHIFT || key_state.scan_code == SC_RSHIFT;
        key_state.is_capslock = key_state.scan_code == SC_CAPS_LOCK;
        g_key_states[key_state.scan_code] = key_state;
        g_scan_code = key_state.scan_code;
        g_keyboard_event_manager.fire_event(&key_state);
    } else {
        critical_fatal(key_state.scan_code, "FATAL (keyboard): key code out of range");
    }

    return rsp;
}

void keyboard_add_handler(void(*handler)(key_state_t*)) {
    g_keyboard_event_manager.add_handler(handler);
}

uint64_t keyboard_get_last_key() {
    return g_scan_code;
}

void keyboard_clear_last_key() {
    g_scan_code = 0;
}