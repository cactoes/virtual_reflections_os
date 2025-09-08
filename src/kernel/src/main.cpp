#include "arch/generic.hpp"
#include "arch/gdt.hpp"
#include "arch/interrupt.hpp"
#include "arch/pit.hpp"

#include "drivers/vga.hpp"
#include "drivers/pcie.hpp"
#include "drivers/pit.hpp"
#include "drivers/ps2/keyboard.hpp"
#include "drivers/ps2/mouse.hpp"
#include "drivers/ps2/ps2.hpp"
#include "drivers/storage/ide.hpp"
#include "drivers/network/e1000.hpp"
#include "drivers/network/nidm.hpp"

#include "interrupt_manager.hpp"

#include "filesystems/iso9660.hpp"
#include "filesystems/vfs.hpp"

#include "memory/vmem.hpp"
#include "memory/heap.hpp"

#include "utils/debug.hpp"
#include "utils/vector.hpp"
#include "utils/event.hpp"

#include "time/clock.hpp"

#include "multiboot.hpp"
#include "string.hpp"
#include "common.hpp"
#include "crash_handler.hpp"
#include "virtual_thread.hpp"
#include "random.hpp"

#define HEAP_START_SIZE 0x100000 * 32 // 32 mb
#define PIT_TIMER_INTERVAL 1000 // times per second

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

void on_key_down(const ps2_key_state_t* p_state) {
    static char s_ascii_table[128] = {
        0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
        '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
        0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-',
        '4', '5', '6', '+', '1', '2', '3', '0', '.'
    };

    static char s_ascii_table_upper[128] = {
        0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
        '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
        0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
        '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*',
        0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-',
        '4', '5', '6', '+', '1', '2', '3', '0', '.'
    };

    if (p_state->scan_code > 128 || !p_state->is_pressed)
        return;

    auto shift_key = ps2_keyboard_get_key_state(PS2_KEYBOARD_SC_LSHIFT);
    auto caps_key = ps2_keyboard_get_key_state(PS2_KEYBOARD_SC_CAPS_LOCK);

    char ch;

    if (shift_key->is_pressed && !caps_key->is_pressed)
        ch = s_ascii_table_upper[p_state->scan_code];
    else if (shift_key->is_pressed && caps_key->is_pressed)
        ch = s_ascii_table[p_state->scan_code];
    else if (!shift_key->is_pressed && caps_key->is_pressed)
        ch = s_ascii_table_upper[p_state->scan_code];
    else
        ch = s_ascii_table[p_state->scan_code];

    printf(DBG, "%c", ch);
}

static int64_t g_desktop_mouse_pos_x = 0;
static int64_t g_desktop_mouse_pos_y = 0;
static int64_t g_desktop_mouse_scroll = 1;
static bool g_desktop_lmb_pressed = false;

void desktop_handle_mouse_input(const ps2_mouse_state_t* p_state) {
    // printf(DBG, "L: %i, M: %i, R: %i, S: %i\n", p_state->buttons.left, p_state->buttons.middle, p_state->buttons.right, p_state->ds);

    g_desktop_mouse_pos_x += p_state->dx;
    g_desktop_mouse_pos_y += p_state->dy;
    g_desktop_mouse_scroll += p_state->ds;

    g_desktop_lmb_pressed = p_state->buttons.left;

    g_desktop_mouse_pos_x = CLAMP(g_desktop_mouse_pos_x, 0, VGA_GM_BUFFER_WIDTH - 1);
    g_desktop_mouse_pos_y = CLAMP(g_desktop_mouse_pos_y, 0, VGA_GM_BUFFER_HEIGHT - 1);
    g_desktop_mouse_scroll = CLAMP(g_desktop_mouse_scroll, 1, 20);
}

void desktop_draw_cursor(vga_buffer_t* p_buffer, uint64_t x, uint64_t y) {
    vga_gm_draw::linev(p_buffer, x, y, 10, vga_gm_color_index_t::BLACK);
    vga_gm_draw::pixel(p_buffer, x + 1, y + 1, vga_gm_color_index_t::BLACK);
    vga_gm_draw::pixel(p_buffer, x + 2, y + 2, vga_gm_color_index_t::BLACK);
    vga_gm_draw::pixel(p_buffer, x + 3, y + 3, vga_gm_color_index_t::BLACK);
    vga_gm_draw::pixel(p_buffer, x + 4, y + 4, vga_gm_color_index_t::BLACK);
    vga_gm_draw::pixel(p_buffer, x + 5, y + 5, vga_gm_color_index_t::BLACK);
    vga_gm_draw::pixel(p_buffer, x + 6, y + 6, vga_gm_color_index_t::BLACK);
    
    vga_gm_draw::lineh(p_buffer, x + 4, y + 7, 3, vga_gm_color_index_t::BLACK);

    vga_gm_draw::pixel(p_buffer, x + 1, y + 9, vga_gm_color_index_t::BLACK);
    vga_gm_draw::pixel(p_buffer, x + 2, y + 8, vga_gm_color_index_t::BLACK);
    vga_gm_draw::pixel(p_buffer, x + 3, y + 7, vga_gm_color_index_t::BLACK);
}

static uint64_t g_last_tick = 0;
constexpr uint64_t game_board_width = 16;
constexpr uint64_t game_board_height = 16;
constexpr uint64_t tile_size = 10;
constexpr uint64_t bomb_count = 40;

struct tile_t {
    int bomb_count;
    int mark_count;

    bool is_bomb;
    bool is_marked;
    bool is_revealed;
    
    uint64_t grid_x;
    uint64_t grid_y;
};

const vga_gm_color_index_t g_tile_empty[10][10] {
    vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY,
    vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY,
    vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY,
    vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY,
    vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY,
    vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY,
    vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY,
    vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY,
    vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY,
    vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY,
};

const vga_gm_color_index_t g_tile_bomb[10][10] {
    vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY,
    vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::BLACK, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::BLACK, vga_gm_color_index_t::LIGHT_GRAY,
    vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::BLACK, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::BLACK, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY,
    vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::BLACK, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::BLACK, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY,
    vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::BLACK, vga_gm_color_index_t::BLACK, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY,
    vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::BLACK, vga_gm_color_index_t::BLACK, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY,
    vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::BLACK, vga_gm_color_index_t::BLACK, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::BLACK, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY,
    vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::BLACK, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::BLACK, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY,
    vga_gm_color_index_t::BLACK, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::BLACK, vga_gm_color_index_t::LIGHT_GRAY,
    vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY,
};

const vga_gm_color_index_t g_tile_unrevealed[10][10] {
    vga_gm_color_index_t::WHITE, vga_gm_color_index_t::WHITE, vga_gm_color_index_t::WHITE, vga_gm_color_index_t::WHITE, vga_gm_color_index_t::WHITE, vga_gm_color_index_t::WHITE, vga_gm_color_index_t::WHITE, vga_gm_color_index_t::WHITE, vga_gm_color_index_t::WHITE, vga_gm_color_index_t::WHITE,
    vga_gm_color_index_t::WHITE, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::DARK_GRAY,
    vga_gm_color_index_t::WHITE, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::DARK_GRAY,
    vga_gm_color_index_t::WHITE, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::DARK_GRAY,
    vga_gm_color_index_t::WHITE, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::DARK_GRAY,
    vga_gm_color_index_t::WHITE, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::DARK_GRAY,
    vga_gm_color_index_t::WHITE, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::DARK_GRAY,
    vga_gm_color_index_t::WHITE, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::DARK_GRAY,
    vga_gm_color_index_t::WHITE, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::LIGHT_GRAY, vga_gm_color_index_t::DARK_GRAY,
    vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY, vga_gm_color_index_t::DARK_GRAY,
};

void draw_tile(vga_buffer_t* p_buffer, tile_t* p_tile, uint64_t offset_x, uint64_t offset_y) {
    if (!p_tile->is_revealed) {
        for (size_t x = 0; x < tile_size; x++) {
            for (size_t y = 0; y < tile_size; y++) {
                vga_gm_draw::pixel(p_buffer, tile_size * p_tile->grid_x + offset_x + x, tile_size * p_tile->grid_y + offset_y + y, g_tile_unrevealed[x][y]);
            }
        }
    } else {

        if (p_tile->is_bomb) {
            for (size_t x = 0; x < tile_size; x++) {
                for (size_t y = 0; y < tile_size; y++) {
                    vga_gm_draw::pixel(p_buffer, tile_size * p_tile->grid_x + offset_x + x, tile_size * p_tile->grid_y + offset_y + y, g_tile_bomb[x][y]);
                }
            }
        } else {
            for (size_t x = 0; x < tile_size; x++) {
                for (size_t y = 0; y < tile_size; y++) {
                    vga_gm_draw::pixel(p_buffer, tile_size * p_tile->grid_x + offset_x + x, tile_size * p_tile->grid_y + offset_y + y, g_tile_empty[x][y]);
                }
            }
        }

    }
}

int desktop_init() {
    g_last_tick = clock_get_time_since_boot();
    vga_buffer_t buff {};
    vga_gm_buffer_create(&buff);
    vga_gm_startup(&buff);
    vga_gm_draw::clear(&buff, vga_gm_color_index_t::WHITE);
    
    seed_random(1234);

    ps2_mouse_event_subscribe(desktop_handle_mouse_input);

    constexpr uint64_t frame_time_ms = 1000 / 60;

    tile_t game_board[game_board_width][game_board_height] {};

    for (int i = 0; i < game_board_width; i++) {
        for (int j = 0; j < game_board_height; j++) {
            auto& tile = game_board[i][j];
            tile.grid_x = i;
            tile.grid_y = j;
        }
    }

    dynamic_array<dynamic_array<int>> bomb_spots {};
    for (int i = 0; i < game_board_width; i++) {
        for (int j = 0; j < game_board_height; j++) {
            bomb_spots.insert_back( dynamic_array<int>{ i, j } );
        }
    }

    for (int n = 0; n < bomb_count; n++) {
        int index = random_number() % bomb_spots.length();
        int x = bomb_spots[index][0];
        int y = bomb_spots[index][1];
        bomb_spots.delete_at(index);
        game_board[x][y].is_bomb = true;
    }

    while (true) {      
        uint64_t now = clock_get_time_since_boot();

        if (now - g_last_tick < frame_time_ms) {
            // vthread_sleep(frame_time_ms - (now - g_last_tick));
            continue;
        }

        vga_gm_draw::clear(&buff, vga_gm_color_index_t::WHITE);

        for (int i = 0; i < game_board_width; i++) {
            for (int j = 0; j < game_board_height; j++) {
                tile_t* p_tile = &game_board[i][j];

                if (g_desktop_lmb_pressed &&
                    g_desktop_mouse_pos_x > p_tile->grid_x * tile_size && g_desktop_mouse_pos_x < p_tile->grid_x * tile_size + tile_size &&
                    g_desktop_mouse_pos_y > p_tile->grid_y * tile_size && g_desktop_mouse_pos_y < p_tile->grid_y * tile_size + tile_size)
                    game_board[i][j].is_revealed = true;

                draw_tile(&buff, p_tile, 0, 0);
            }
        }

        desktop_draw_cursor(&buff, g_desktop_mouse_pos_x, g_desktop_mouse_pos_y);

        vga_gm_render();

        g_last_tick = now;
    }

    return true;
}

extern "C" void kernel_entry(void* p_multiboot_struct, void* p_kpml4) {
    // validate multiboot
    if (!mb_has_valid_magic((multiboot_t*)p_multiboot_struct))
        kernel_fatal(KERNEL_FATAL_MULTIBOOT_MAGIC_VALIDATE, "multiboot magic was not valid");

    // initialize the gdt / tss
    gdt_init();

    // initialze vga text mode
    vga_tm_init_buffer(&g_vga_tm_buffer, (void*)VGA_TM_BUFFER_ADDR, VGA_TM_NUM_COLS, VGA_TM_NUM_ROWS);
    vga_tm_clear_buffer(&g_vga_tm_buffer);

    // initialze the debug out stream
    debug_init();

    // initialze virtual memory
    if (!vmem_init(p_multiboot_struct, p_kpml4))
        kernel_fatal(KERNEL_FATAL_VMEM_INIT, "vmem failed to initialize");
    
    vmem_identity_map(p_kpml4);
    set_pml4(p_kpml4);

    // initialze the global heap
    heap_t heap {};
    if (!heap_init(&heap, p_kpml4, (void*)VMEM_HEAP_START_ADDR, HEAP_START_SIZE))
        kernel_fatal(KERNEL_FATAL_HEAP_INIT, "kernel heap fail to initialze");
    
    set_global_heap(&heap);

    // initialze the interrupt line(s)
    set_interrupt_callback(interrupt_t::HARDWARE_PIT, pit_handle_interrupt);
    set_interrupt_callback(interrupt_t::HARDWARE_KEYBOARD, ps2_keyboard_handle_interrupt);
    set_interrupt_callback(interrupt_t::HARDWARE_PS2_MOUSE, ps2_mouse_handle_interrupt);
    set_interrupt_callback(interrupt_t::SOFTWARE_SCHEDULER, vthread_handle_interrupt);

    interrupt_set_handler(handle_interrupt);
    ps2_mouse_init();
    pit_init(PIT_TIMER_INTERVAL);
    interrupt_init(gdt_get_kernel_code_selector());

    if (ps2_port_test_device(ps2_device_type_t::KEYBOARD)) {
        printf(DBG, "[+] ps2/keyboard\n");
    }

    if (ps2_port_test_device(ps2_device_type_t::MOUSE)) {
        printf(DBG, "[+] ps2/mouse\n");
    }

    if (vthread_start_and_setup_main() == VTHREAD_HANDLE_INVALID)
        kernel_fatal(KERNEL_FATAL_VHREAD_INIT, "virtual threads failed to intialize");

    pit_add_interrupt_function(vthread_handle_interrupt);

    linked_list<pci_device_t> pci_devices {};
    pci_enumerate_devices(&pci_devices);

    for (auto& device : pci_devices) {
        const char* cd = pci_get_class_description(&device);
        printf(DBG, "[+] %s\n", cd);
    }

    // ide device
    pci_class_info_t ide_device_class_info {
        .revision_id = (uint8_t)PCI_UNKNOWN,
        .prog_if = (uint8_t)PCI_UNKNOWN,
        .sub_class = (uint8_t)1,
        .class_code = (uint8_t)1
    };
    const pci_device_t* ide_controller = pci_find_device(&pci_devices, &ide_device_class_info);

    linked_list<ide_device_t> ide_devices {};
    ide_init(ide_controller, &ide_devices);
    
    size_t ide_device_index = 0;
    for (auto& drive : ide_devices) {
        char buffer[20];
        sprintf(buffer, 20, "ide/disk%i", ide_device_index++);
        printf(DBG, "[+] %s\n", buffer);
    }

    // TODO @since 05/08/2025 -- 01:18
    // detect file system
    iso9660_fs_data_t mounted_iso9660_fs_instance {};
    iso9660_drive_init(&ide_devices[0], &mounted_iso9660_fs_instance);

    // ahci device
    pci_class_info_t ahci_device_class_info {
        .revision_id = (uint8_t)PCI_UNKNOWN,
        .prog_if = (uint8_t)1,
        .sub_class = (uint8_t)6,
        .class_code = (uint8_t)1
    };
    const pci_device_t* ahci_controller = pci_find_device(&pci_devices, &ahci_device_class_info);
    // TODO @since 14/07/2025 -- 21:52
    
    // network device
    pci_class_info_t network_device_class_info {
        .revision_id = (uint8_t)PCI_UNKNOWN,
        .prog_if = (uint8_t)PCI_UNKNOWN,
        .sub_class = (uint8_t)0,
        .class_code = (uint8_t)2
    };
    const pci_device_t* network_controller = pci_find_device(&pci_devices, &network_device_class_info);
    
    e1000_t e1000 {};
    const auto e1000_init_result = e1000_init_device(network_controller, &e1000);
    printf(DBG, "e1000_init_result: %i\n", e1000_init_result);
    if (e1000_init_result == 0) {
        printf(DBG, "MAC: %uh:%uh:%uh:%uh:%uh:%uh\n", e1000.mac[0], e1000.mac[1], e1000.mac[2], e1000.mac[3], e1000.mac[4], e1000.mac[5]);

        network_interface_device_t e1000_nid {};
        e1000_nid.name = "eth0 (Intel e1000)";
        // TODO @since 27/08/2025 -- 03:49
        // dhcp :)
        e1000_nid.ip4 = TO_IP(10, 0, 2, 15);
        e1000_nid.is_up = true;
        e1000_nid.device_data = &e1000;
        memcpy(e1000_nid.mac, e1000.mac, sizeof(e1000_nid.mac));
        e1000_nid.send_packet = e1000_nidm_send_packet;
        nidm_register_device(e1000_nid);
    }

    virtual_file_system vfs {};
    vfs.create_directory("/mnt");

    auto disk_storage = ptr::make_unique<vfs_disk_storage>(&mounted_iso9660_fs_instance);
    vfs.mount("/mnt/disk0", move(disk_storage));
    vfs.create_file_cache("/mnt/disk0/INetDrivers.sys");
    vfs.create_file_cache("/mnt/disk0/.env");
    vfs.create_file_cache("/mnt/disk0/media/eva-title-0.bmp");
    vfs.create_file_cache("/mnt/disk0/media/eva-title-0.png");

    // dynamic_array<uint8_t> inet_driver_file {};
    // auto inet_driver_file_handle = vfs.open_file("/mnt/disk0/INetDrivers.sys");
    // vfs.read_file(inet_driver_file_handle, &inet_driver_file);

    // auto handle = driver_load("INetDrivers", inet_driver_file.get_data());
    // driver_start(handle);
    // void* check_driver_function = driver_get_function(handle, "check_driver");
    // typedef bool(*check_driver_function_t)(const char*);
    // auto result = ((check_driver_function_t)check_driver_function)("e1000");
    // printf(DBG, "has e1000 driver: %s\n", result ? "true" : "false");

    // ps2_keyboard_event_subscribe(on_key_down);
    // ps2_mouse_event_subscribe(on_mouse);

    // kernel finished
    printf(STD, "> SYSTEM READY\n");
    printf(DBG, "Kernel finished initializing\n");

    // vthread_create(desktop_init);
    desktop_init();

    // we shoudn t reach this point since the kernel should never stop
    // incase we do just hang here so we dont break anything
    while (true);
}