#include "drivers/ps2/keyboard.hpp"
#include "drivers/ps2/ps2.hpp"
#include "utils/debug.hpp"
#include "utils/event.hpp"
#include "std/ring_buffer.hpp"

static u32 g_last_scan_code = MAX_UINT32;
static ps2_key_state_t g_key_state_array[PS2_KEYBOARD_KEY_STATE_ARRAY_SIZE] {};
static std::ring_buffer<8, ps2_key_state_t> global_keyboard_state_buffer {};
static event_manager_t<const ps2_key_state_t*> g_keyboard_event_manager {};

void* ps2_keyboard_handle_interrupt(void* stack, void*) {
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
        return stack;

    // BUG @since 25/09/2025 -- 12:31
    // for some reason PS2_KEYBOARD_FULL_CODE_ESCAPED is not sent when releasing the key?
    if (!key_state.is_escaped && g_key_state_array[key_state.scan_code].is_escaped)
        key_state.is_escaped = true;

    g_key_state_array[key_state.scan_code] = key_state;
    g_last_scan_code = key_state.scan_code;
    global_keyboard_state_buffer.insert(key_state);

    return stack;
}

u32 ps2_keyboard_get_last_scancode() {
    return g_last_scan_code;
}

void ps2_keyboard_clear_last_scancode() {
    g_last_scan_code = MAX_UINT32;
}

const ps2_key_state_t* ps2_keyboard_get_key_state(u32 scan_code) {
    if (scan_code >= PS2_KEYBOARD_KEY_STATE_ARRAY_SIZE)
        return nullptr;

    return &g_key_state_array[scan_code];
}

bool ps2_keyboard_is_scan_code_extended(u32 scan_code) {
    if (auto state = ps2_keyboard_get_key_state(scan_code))
        return state->is_escaped;

    return false;
}

void ps2_keyboard_event_subscribe(void(*p_handler)(const ps2_key_state_t*)) {
    g_keyboard_event_manager.subscribe(p_handler);
}

void ps2_keyboard_process_packet() {
    ps2_key_state_t packet {};
    if (global_keyboard_state_buffer.get(packet))
        g_keyboard_event_manager.fire_event(&packet);
}

void ps2_keyboard_init() {
    ps2_write(PS2_CMD_PORT, PS2_CMD_DISABLE_FIRST_PORT);
    
    if (ps2_read(PS2_CMD_PORT) & PS2_CTRL_STATUS_OUT_BUF)
        ps2_read(PS2_DATA_PORT);

    ps2_write(PS2_CMD_PORT, 0x20);
    u8 config = ps2_read(PS2_DATA_PORT);
    config |= (1 << 0); // interrupts
    // config &= ~(1 << 6); // disable translation
    ps2_write(PS2_CMD_PORT, 0x60);
    ps2_write(PS2_DATA_PORT, config);

    ps2_write(PS2_CMD_PORT, PS2_CMD_ENABLE_FIRST_PORT);
    ps2_write(PS2_DATA_PORT, PS2_DEVICE_RESET);
    
    ps2_read(PS2_DATA_PORT) == PS2_DEVICE_ACK;
    ps2_read(PS2_DATA_PORT) == 0xAA;

    // ps2_write(PS2_DATA_PORT, 0xF0);
    // ps2_read(PS2_DATA_PORT) == PS2_DEVICE_ACK;

    // ps2_write(PS2_DATA_PORT, 0x02);
    // ps2_read(PS2_DATA_PORT) == PS2_DEVICE_ACK;

    ps2_write(PS2_DATA_PORT, PS2_DEVICE_ENABLE_SCANNING);
    ps2_read(PS2_DATA_PORT) == PS2_DEVICE_ACK;
}