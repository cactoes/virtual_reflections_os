#include "drivers/keyboard.hpp"
#include "drivers/ps2/keyboard.hpp"

char key_to_ascii(uint32_t scan_code, bool shift, bool caps) {
    static char ascii_table[128] = {
        0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
        '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
        0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-',
        '4', '5', '6', '+', '1', '2', '3', '0', '.'
    };

    static char ascii_table_upper[128] = {
        0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
        '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
        0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
        '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*',
        0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-',
        '4', '5', '6', '+', '1', '2', '3', '0', '.'
    };

    if (scan_code > 128)
        return 0;

    if (shift && !caps)
        return ascii_table_upper[scan_code];

    if (shift && caps)
        return ascii_table[scan_code];

    if (!shift && caps)
        return ascii_table_upper[scan_code];

    return ascii_table[scan_code];
}

char wait_for_key() {
    uint32_t scan_code = 0;
    char ascii = 0;
    while (scan_code == 0 || ascii == 0) {
        scan_code = ps2_keyboard_get_last_scancode();

        if (!ps2_keyboard_get_key_state(scan_code)->is_pressed)
            continue;

        auto shift_key = ps2_keyboard_get_key_state(PS2_KEYBOARD_SC_LSHIFT);
        auto caps_key = ps2_keyboard_get_key_state(PS2_KEYBOARD_SC_CAPS_LOCK);
        ascii = key_to_ascii(scan_code, shift_key->is_pressed, caps_key->is_pressed);
    }

    ps2_keyboard_clear_last_scancode();
    return ascii;
}