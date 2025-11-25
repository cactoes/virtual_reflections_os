#include "drivers/keyboard.hpp"
#include "drivers/ps2/keyboard.hpp"

bool holding_shift() {
    return ps2_keyboard_get_key_state(PS2_KEYBOARD_SC_LSHIFT)->is_pressed || ps2_keyboard_get_key_state(PS2_KEYBOARD_SC_RSHIFT)->is_pressed;
}

bool holding_caps() {
    return ps2_keyboard_get_key_state(PS2_KEYBOARD_SC_CAPS_LOCK)->is_pressed;
}

virtual_key_t scan_to_virtual(uint32_t scan_code, bool escaped) {
    if (escaped) {
        switch (scan_code) {
            case 0x48: return VK_UP;
            case 0x50: return VK_DOWN;
            case 0x4B: return VK_LEFT;
            case 0x4D: return VK_RIGHT;
            case 0x47: return VK_HOME;
            case 0x4F: return VK_END;
            case 0x49: return VK_PAGE_UP;
            case 0x51: return VK_PAGE_DOWN;
            case 0x52: return VK_INSERT;
            case 0x53: return VK_DELETE;

            case 0x1D: return VK_CTRL;
            case 0x38: return VK_ALT;
            case 0x35: return VK_NUMPAD_DIVIDE;
            case 0x1C: return VK_NUMPAD_ENTER;

            default:   return VK_NONE;
        }
    }

    switch (scan_code) {
        case 0x1E: return VK_A;
        case 0x30: return VK_B;
        case 0x2E: return VK_C;
        case 0x20: return VK_D;
        case 0x12: return VK_E;
        case 0x21: return VK_F;
        case 0x22: return VK_G;
        case 0x23: return VK_H;
        case 0x17: return VK_I;
        case 0x24: return VK_J;
        case 0x25: return VK_K;
        case 0x26: return VK_L;
        case 0x32: return VK_M;
        case 0x31: return VK_N;
        case 0x18: return VK_O;
        case 0x19: return VK_P;
        case 0x10: return VK_Q;
        case 0x13: return VK_R;
        case 0x1F: return VK_S;
        case 0x14: return VK_T;
        case 0x16: return VK_U;
        case 0x2F: return VK_V;
        case 0x11: return VK_W;
        case 0x2D: return VK_X;
        case 0x15: return VK_Y;
        case 0x2C: return VK_Z;

        case 0x0B: return VK_0;
        case 0x02: return VK_1;
        case 0x03: return VK_2;
        case 0x04: return VK_3;
        case 0x05: return VK_4;
        case 0x06: return VK_5;
        case 0x07: return VK_6;
        case 0x08: return VK_7;
        case 0x09: return VK_8;
        case 0x0A: return VK_9;

        case 0x39: return VK_SPACE;
        case 0x1C: return VK_ENTER;
        case 0x0F: return VK_TAB;
        case 0x0E: return VK_BACKSPACE;
        case 0x01: return VK_ESCAPE;

        case 0x2A: return VK_SHIFT;
        case 0x36: return VK_SHIFT;
        case 0x1D: return VK_CTRL;
        case 0x38: return VK_ALT;
        case 0x3A: return VK_CAPSLOCK;

        case 0x0C: return VK_MINUS;
        case 0x0D: return VK_EQUALS;
        case 0x1A: return VK_LEFT_BRACKET;
        case 0x1B: return VK_RIGHT_BRACKET; 
        case 0x2B: return VK_BACKSLASH; 
        case 0x27: return VK_SEMICOLON;
        case 0x28: return VK_APOSTROPHE;
        case 0x29: return VK_GRAVE; 
        case 0x33: return VK_COMMA;
        case 0x34: return VK_PERIOD;
        case 0x35: return VK_SLASH;

        case 0x3B: return VK_F1;
        case 0x3C: return VK_F2;
        case 0x3D: return VK_F3;
        case 0x3E: return VK_F4;
        case 0x3F: return VK_F5;
        case 0x40: return VK_F6;
        case 0x41: return VK_F7;
        case 0x42: return VK_F8;
        case 0x43: return VK_F9;
        case 0x44: return VK_F10;
        case 0x57: return VK_F11;
        case 0x58: return VK_F12;

        case 0x52: return VK_NUMPAD0;
        case 0x4F: return VK_NUMPAD1;
        case 0x50: return VK_NUMPAD2;
        case 0x51: return VK_NUMPAD3;
        case 0x4B: return VK_NUMPAD4;
        case 0x4C: return VK_NUMPAD5;
        case 0x4D: return VK_NUMPAD6;
        case 0x47: return VK_NUMPAD7;
        case 0x48: return VK_NUMPAD8;
        case 0x49: return VK_NUMPAD9;
        case 0x4A: return VK_NUMPAD_MINUS;
        case 0x4E: return VK_NUMPAD_PLUS;
        case 0x37: return VK_NUMPAD_MULTIPLY;
        case 0x53: return VK_NUMPAD_PERIOD;

        default:   return VK_NONE;
    }
}

char vk_to_ascii(virtual_key_t vk, bool shift, bool caps) {
    bool upper = shift ^ caps;

    switch (vk) {
        case VK_A: return upper ? 'A' : 'a';
        case VK_B: return upper ? 'B' : 'b';
        case VK_C: return upper ? 'C' : 'c';
        case VK_D: return upper ? 'D' : 'd';
        case VK_E: return upper ? 'E' : 'e';
        case VK_F: return upper ? 'F' : 'f';
        case VK_G: return upper ? 'G' : 'g';
        case VK_H: return upper ? 'H' : 'h';
        case VK_I: return upper ? 'I' : 'i';
        case VK_J: return upper ? 'J' : 'j';
        case VK_K: return upper ? 'K' : 'k';
        case VK_L: return upper ? 'L' : 'l';
        case VK_M: return upper ? 'M' : 'm';
        case VK_N: return upper ? 'N' : 'n';
        case VK_O: return upper ? 'O' : 'o';
        case VK_P: return upper ? 'P' : 'p';
        case VK_Q: return upper ? 'Q' : 'q';
        case VK_R: return upper ? 'R' : 'r';
        case VK_S: return upper ? 'S' : 's';
        case VK_T: return upper ? 'T' : 't';
        case VK_U: return upper ? 'U' : 'u';
        case VK_V: return upper ? 'V' : 'v';
        case VK_W: return upper ? 'W' : 'w';
        case VK_X: return upper ? 'X' : 'x';
        case VK_Y: return upper ? 'Y' : 'y';
        case VK_Z: return upper ? 'Z' : 'z';

        case VK_1: return shift ? '!' : '1';
        case VK_2: return shift ? '@' : '2';
        case VK_3: return shift ? '#' : '3';
        case VK_4: return shift ? '$' : '4';
        case VK_5: return shift ? '%' : '5';
        case VK_6: return shift ? '^' : '6';
        case VK_7: return shift ? '&' : '7';
        case VK_8: return shift ? '*' : '8';
        case VK_9: return shift ? '(' : '9';
        case VK_0: return shift ? ')' : '0';

        case VK_MINUS:        return shift ? '_' : '-';
        case VK_EQUALS:       return shift ? '+' : '=';
        case VK_LEFT_BRACKET: return shift ? '{' : '[';
        case VK_RIGHT_BRACKET:return shift ? '}' : ']';
        case VK_BACKSLASH:    return shift ? '|' : '\\';
        case VK_SEMICOLON:    return shift ? ':' : ';';
        case VK_APOSTROPHE:   return shift ? '"' : '\'';
        case VK_GRAVE:        return shift ? '~' : '`';
        case VK_COMMA:        return shift ? '<' : ',';
        case VK_PERIOD:       return shift ? '>' : '.';
        case VK_SLASH:        return shift ? '?' : '/';

        case VK_SPACE:   return ' ';
        case VK_ENTER:   return '\n';
        case VK_TAB:     return '\t';
        case VK_BACKSPACE: return '\b';
        case VK_ESCAPE:  return 27;

        case VK_NUMPAD0: return '0';
        case VK_NUMPAD1: return '1';
        case VK_NUMPAD2: return '2';
        case VK_NUMPAD3: return '3';
        case VK_NUMPAD4: return '4';
        case VK_NUMPAD5: return '5';
        case VK_NUMPAD6: return '6';
        case VK_NUMPAD7: return '7';
        case VK_NUMPAD8: return '8';
        case VK_NUMPAD9: return '9';
        case VK_NUMPAD_PLUS:      return '+';
        case VK_NUMPAD_MINUS:     return '-';
        case VK_NUMPAD_MULTIPLY:  return '*';
        case VK_NUMPAD_DIVIDE:    return '/';
        case VK_NUMPAD_PERIOD:    return '.';
        case VK_NUMPAD_ENTER:     return '\n';

        default: return 0;
    }
}

void keyboard_initialize() {
}

virtual_key_t wait_for_key() {
    uint32_t scan_code = 0;
    virtual_key_t vk = VK_NONE;
    while (scan_code == MAX_UINT32 || vk == 0) {
        scan_code = ps2_keyboard_get_last_scancode();
        auto key_state = ps2_keyboard_get_key_state(scan_code);

        if (scan_code == MAX_UINT32 || !key_state->is_pressed || key_state->is_escaped)
            continue;

        vk = scan_to_virtual(scan_code, key_state->is_escaped);
    }

    ps2_keyboard_clear_last_scancode();
    return vk;
}