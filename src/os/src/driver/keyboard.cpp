#include "driver/keyboard.hpp"
#include "critical/kernel.hpp"

kernel::driver::keyboard::key_state_t g_key_states[256] = {};

bool kernel::driver::keyboard::handle_special_key(keyboard_state_t* keyboard_state, key_state_t* key_state) {
    if (key_state->scan_code == SC_CAPS_LOCK) {
        keyboard_state->caps_lock != keyboard_state->caps_lock;
        return true;
    }

    if (key_state->scan_code == SC_LSHIFT || key_state->scan_code == SC_RSHIFT) {
        keyboard_state->shift = !key_state->is_released;
        return true;
    }

    if (key_state->scan_code == SC_LALT) {
        keyboard_state->alt = !key_state->is_released;
        return true;
    }

    if (key_state->scan_code == SC_CTRL) {
        keyboard_state->control = !key_state->is_released;
        return true;
    }

    if (key_state->scan_code == SC_NUMLOCK) {
        keyboard_state->num_lock = !key_state->is_released;
        return true;
    }

    return false;
}

char key_state_to_char(kernel::driver::keyboard::keyboard_state_t* keyboard_state, kernel::driver::keyboard::key_state_t* key_state) {
    static char scancode_to_ascii[] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0,
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
        'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0,
    };

    static char scancode_to_ascii_caps[] = {
        0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0, 0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'I', 'O', '{', '}', '\n', 0,
        'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0, '|',
        'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '?', 0, 0, 0, ' ', 0,
    };

    // if only caps or shift is pressed
    if ((keyboard_state->shift && !keyboard_state->caps_lock) ||
        (keyboard_state->control && !keyboard_state->caps_lock)) {

        if (key_state->scan_code < sizeof(scancode_to_ascii)) {
            char ascii = scancode_to_ascii_caps[key_state->scan_code];
            if (!key_state->is_released)
                return ascii;
        }
    }

    // simple char
    if (key_state->scan_code < sizeof(scancode_to_ascii)) {
        char ascii = scancode_to_ascii[key_state->scan_code];
        if (!key_state->is_released)
            return ascii;
    }

    // F(x) keys
    // numpad (https://aeb.win.tue.nl/linux/kbd/scancodes-1.html)
    // 0x47 = 7 // 0x48 = 8 // 0x49 = 9 // 0x4A = -
    // 0x4B = 4 // 0x4C = 5 // 0x4D = 6
    // 0x4F = 1 // 0x50 = 2 // 0x51 = 3
    // 0x52 = 0 // 0x53 = .
    // 0x60:
    //     0x1C -> \n
    //     0x35 -> /

    // 0x37 = *

    // other specials

    return 0;
}

void kernel::driver::keyboard::handle_interrupt(keyboard_state_t* state) {
    // !!! incomplete

    key_state_t key_state {};
    (void)cpu::in_port(cpu::PT_B, KEYBOARD_PORT_SCANCODE, (uint32_t*)&key_state.full_code);
    if (key_state.full_code == FULL_CODE_ESCAPED) {
        key_state.is_escaped = true;
        (void)cpu::in_port(cpu::PT_B, KEYBOARD_PORT_SCANCODE, (uint32_t*)&key_state.full_code);
    }

    key_state.scan_code = (key_state.full_code & SCAN_CODE_MASK_CHAR);
    key_state.is_released = (key_state.full_code & SCAN_CODE_MASK_PRESSED) != 0;

    g_key_states[key_state.scan_code] = key_state;

    if (handle_special_key(state, &key_state))
        return;

    // kernel_print("%c", key_state_to_char(state, &key_state));
}

kernel::driver::keyboard::key_state_t* kernel::driver::keyboard::get_key_state(uint64_t ch) {
    if (ch > sizeof(g_key_states))
        return nullptr;

    return &g_key_states[ch];
}