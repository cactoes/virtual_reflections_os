#include "drivers/ps2/keyboard.hpp"
#include "drivers/ps2/ps2.hpp"
#include "utils/debug.hpp"
#include "utils/event.hpp"

static event_manager_t<const ps2_key_state_t*> g_keyboard_event_manager {};
static uint32_t g_last_scan_code = 0;
static ps2_key_state_t g_key_state_array[PS2_KEYBOARD_KEY_STATE_ARRAY_SIZE] {};

void ps2_keyboard_handle_interrupt() {
    ps2_key_state_t key_state {};
    key_state.full_code = ps2_read(PS2_DATA_PORT);
    key_state.is_escaped = false;

    if (key_state.full_code == PS2_KEYBOARD_FULL_CODE_ESCAPED) {
        key_state.is_escaped = true;
        key_state.full_code = ps2_read(PS2_DATA_PORT);
    }

    key_state.scan_code = (key_state.full_code & PS2_KEYBOARD_SCAN_CODE_MASK_CHAR);
    key_state.is_released = (key_state.full_code & PS2_KEYBOARD_SCAN_CODE_MASK_PRESSED) != 0;
    key_state.is_pressed = (key_state.full_code & PS2_KEYBOARD_SCAN_CODE_MASK_PRESSED) == 0;
    key_state.is_shift = key_state.scan_code == PS2_KEYBOARD_SC_LSHIFT || key_state.scan_code == PS2_KEYBOARD_SC_RSHIFT;
    key_state.is_capslock = key_state.scan_code == PS2_KEYBOARD_SC_CAPS_LOCK;

    if (key_state.scan_code >= PS2_KEYBOARD_KEY_STATE_ARRAY_SIZE)
        return;

    g_key_state_array[key_state.scan_code] = key_state;
    g_last_scan_code = key_state.scan_code;
    g_keyboard_event_manager.fire_event(&g_key_state_array[key_state.scan_code]);
}

uint32_t ps2_keyboard_get_last_scancode() {
    return g_last_scan_code;
}

void ps2_keyboard_clear_last_scancode() {
    g_last_scan_code = 0;
}

void ps2_keyboard_event_subscribe(void(*p_handler)(const ps2_key_state_t*)) {
    g_keyboard_event_manager.subscribe(p_handler);
}

const ps2_key_state_t* ps2_keyboard_get_key_state(uint32_t scan_code) {
    if (scan_code >= PS2_KEYBOARD_KEY_STATE_ARRAY_SIZE)
        return nullptr;

    return &g_key_state_array[scan_code];
}