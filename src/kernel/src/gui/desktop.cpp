#include "gui/desktop.hpp"
#include "drivers/vga.hpp"
#include "drivers/ps2/mouse.hpp"
#include "utils/vector.hpp"
#include "utils/map.hpp"
#include "utils/event.hpp"
#include "random.hpp"
#include "time/clock.hpp"
#include "virtual_thread.hpp"

enum print_mode_t {
    STD,
    DBG
};

extern void printf(print_mode_t mode, const char* p_str, ...);

// static int64_t g_desktop_mouse_pos_x = 0;
// static int64_t g_desktop_mouse_pos_y = 0;
// static int64_t g_desktop_mouse_scroll = 1;
// static bool g_desktop_lmb_pressed = false;
// static bool g_desktop_rmb_pressed = false;
// static bool g_prev_lmb_pressed = false;
// static bool g_prev_rmb_pressed = false;
// static bool g_lmb_just_pressed = false;
// static bool g_rmb_just_pressed = false;

// void desktop_handle_mouse_input(const ps2_mouse_state_t* p_state) {
//     // printf(DBG, "L: %i, M: %i, R: %i, S: %i\n", p_state->buttons.left, p_state->buttons.middle, p_state->buttons.right, p_state->ds);

//     g_desktop_mouse_pos_x += p_state->dx;
//     g_desktop_mouse_pos_y += p_state->dy;
//     g_desktop_mouse_scroll += p_state->ds;

//     g_lmb_just_pressed = (p_state->buttons.left && !g_prev_lmb_pressed);
//     g_rmb_just_pressed = (p_state->buttons.right && !g_prev_rmb_pressed);

//     g_desktop_lmb_pressed = p_state->buttons.left;
//     g_desktop_rmb_pressed = p_state->buttons.right;

//     // update prev for next input packet
//     g_prev_lmb_pressed = g_desktop_lmb_pressed;
//     g_prev_rmb_pressed = g_desktop_rmb_pressed;

//     g_desktop_mouse_pos_x = CLAMP(g_desktop_mouse_pos_x, 0, VGA_GM_BUFFER_WIDTH - 1);
//     g_desktop_mouse_pos_y = CLAMP(g_desktop_mouse_pos_y, 0, VGA_GM_BUFFER_HEIGHT - 1);
//     g_desktop_mouse_scroll = CLAMP(g_desktop_mouse_scroll, 1, 20);
// }

// void desktop_draw_cursor(vga_buffer_t* p_buffer, uint64_t x, uint64_t y) {
//     vga_gm_draw::linev(p_buffer, x, y, 10, vga_gm_color_index_t::BLACK);
//     vga_gm_draw::pixel(p_buffer, x + 1, y + 1, vga_gm_color_index_t::BLACK);
//     vga_gm_draw::pixel(p_buffer, x + 2, y + 2, vga_gm_color_index_t::BLACK);
//     vga_gm_draw::pixel(p_buffer, x + 3, y + 3, vga_gm_color_index_t::BLACK);
//     vga_gm_draw::pixel(p_buffer, x + 4, y + 4, vga_gm_color_index_t::BLACK);
//     vga_gm_draw::pixel(p_buffer, x + 5, y + 5, vga_gm_color_index_t::BLACK);
//     vga_gm_draw::pixel(p_buffer, x + 6, y + 6, vga_gm_color_index_t::BLACK);
    
//     vga_gm_draw::lineh(p_buffer, x + 4, y + 7, 3, vga_gm_color_index_t::BLACK);

//     vga_gm_draw::pixel(p_buffer, x + 1, y + 9, vga_gm_color_index_t::BLACK);
//     vga_gm_draw::pixel(p_buffer, x + 2, y + 8, vga_gm_color_index_t::BLACK);
//     vga_gm_draw::pixel(p_buffer, x + 3, y + 7, vga_gm_color_index_t::BLACK);
// }

// static uint64_t g_last_tick = 0;
// constexpr uint64_t game_board_width = 16;
// constexpr uint64_t game_board_height = 16;
// constexpr uint64_t tile_size = 10;
// constexpr uint64_t bomb_count = 40;

// struct tile_t {
//     int bomb_count;
//     int mark_count;

//     bool is_bomb;
//     bool is_marked;
//     bool is_revealed;
//     bool has_clicked_bomb;
    
//     uint64_t grid_x;
//     uint64_t grid_y;
// };

// const uint8_t g_tile_empty[10][10] {
//     8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
// };

// const uint8_t g_tile_bomb1[10][10] { // 9u
//     8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 9u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 9u, 9u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 9u, 7u, 9u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 9u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 9u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 9u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 9u, 9u, 9u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
// };

// const uint8_t g_tile_bomb2[10][10] { // 2u
//     8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 2u, 2u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 2u, 7u, 7u, 2u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 2u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 2u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 2u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 2u, 7u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 2u, 2u, 2u, 2u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
// };

// const uint8_t g_tile_bomb3[10][10] { // 4u
//     8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 4u, 4u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 4u, 7u, 7u, 4u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 4u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 4u, 4u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 4u, 7u, 7u, 7u,
//     8u, 7u, 7u, 4u, 7u, 7u, 4u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 4u, 4u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
// };

// const uint8_t g_tile_bomb4[10][10] { // 1u
//     8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 1u, 7u, 7u, 1u, 7u, 7u, 7u,
//     8u, 7u, 7u, 1u, 7u, 7u, 1u, 7u, 7u, 7u,
//     8u, 7u, 7u, 1u, 7u, 7u, 1u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 1u, 1u, 1u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 1u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 1u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 1u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
// };

// const uint8_t g_tile_bomb5[10][10] { // 6u
//     8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 6u, 6u, 6u, 6u, 7u, 7u, 7u,
//     8u, 7u, 7u, 6u, 7u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 6u, 6u, 6u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 6u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 6u, 7u, 7u, 7u,
//     8u, 7u, 7u, 6u, 7u, 7u, 6u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 6u, 6u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
// };

// const uint8_t g_tile_bomb6[10][10] { // 3u
//     8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 3u, 3u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 3u, 7u, 7u, 3u, 7u, 7u, 7u,
//     8u, 7u, 7u, 3u, 7u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 3u, 3u, 3u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 3u, 7u, 7u, 3u, 7u, 7u, 7u,
//     8u, 7u, 7u, 3u, 7u, 7u, 3u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 3u, 3u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
// };

// const uint8_t g_tile_bomb7[10][10] { // 0u
//     8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 0u, 0u, 0u, 0u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 0u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 0u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 0u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 0u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 0u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 0u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
// };

// const uint8_t g_tile_bomb8[10][10] { // 8u
//     8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 8u, 8u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 8u, 7u, 7u, 8u, 7u, 7u, 7u,
//     8u, 7u, 7u, 8u, 7u, 7u, 8u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 8u, 8u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 8u, 7u, 7u, 8u, 7u, 7u, 7u,
//     8u, 7u, 7u, 8u, 7u, 7u, 8u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 8u, 8u, 7u, 7u, 7u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
// };

// const uint8_t g_tile_bomb[10][10] {
//     8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
//     8u, 7u, 0u, 7u, 7u, 0u, 7u, 7u, 0u, 7u,
//     8u, 7u, 7u, 7u, 0u, 0u, 0u, 7u, 7u, 7u,
//     8u, 7u, 7u, 0u, 15, 0u, 0u, 0u, 7u, 7u,
//     8u, 7u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 7u,
//     8u, 7u, 7u, 0u, 0u, 0u, 0u, 0u, 7u, 7u,
//     8u, 7u, 7u, 7u, 0u, 0u, 0u, 7u, 7u, 7u,
//     0u, 7u, 0u, 7u, 7u, 0u, 7u, 7u, 0u, 7u,
//     8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
// };

// const uint8_t g_tile_bomb_exploded[10][10] {
//     8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
//     8u, 12, 12, 12, 12, 12, 12, 12, 12, 12,
//     8u, 12, 0u, 12, 12, 0u, 12, 12, 0u, 12,
//     8u, 12, 12, 12, 0u, 0u, 0u, 12, 12, 12,
//     8u, 12, 12, 0u, 15, 0u, 0u, 0u, 12, 12,
//     8u, 12, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 12,
//     8u, 12, 12, 0u, 0u, 0u, 0u, 0u, 12, 12,
//     8u, 12, 12, 12, 0u, 0u, 0u, 12, 12, 12,
//     0u, 12, 0u, 12, 12, 0u, 12, 12, 0u, 12,
//     8u, 12, 12, 12, 12, 12, 12, 12, 12, 12,
// };

// const uint8_t g_tile_unrevealed[10][10] {
//     15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
//     15, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 8u,
//     15, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 8u,
//     15, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 8u,
//     15, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 8u,
//     15, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 8u,
//     15, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 8u,
//     15, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 8u,
//     15, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 8u,
//     8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
// };

// const uint8_t g_tile_marked[10][10] {
//     15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
//     15, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 8u,
//     15, 7u, 7u, 7u, 4u, 4u, 7u, 7u, 7u, 8u,
//     15, 7u, 4u, 4u, 4u, 4u, 7u, 7u, 7u, 8u,
//     15, 7u, 7u, 4u, 4u, 0u, 7u, 7u, 7u, 8u,
//     15, 7u, 7u, 7u, 7u, 0u, 7u, 7u, 7u, 8u,
//     15, 7u, 7u, 7u, 0u, 0u, 0u, 7u, 7u, 8u,
//     15, 7u, 7u, 0u, 0u, 0u, 0u, 0u, 7u, 8u,
//     15, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 8u,
//     8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
// };

// void draw_tile(vga_buffer_t* p_buffer, tile_t* p_tile, uint64_t offset_x, uint64_t offset_y) {
//     // not revealed
//     if (!p_tile->is_revealed) {
//         // marked
//         if (p_tile->is_marked) {
//             for (size_t x = 0; x < tile_size; x++) {
//                 for (size_t y = 0; y < tile_size; y++) {
//                     vga_gm_draw::pixel(p_buffer, tile_size * p_tile->grid_x + offset_x + x, tile_size * p_tile->grid_y + offset_y + y, (vga_gm_color_index_t)g_tile_marked[y][x]);
//                 }
//             }
//             return;
//         }

//         // nothing
//         for (size_t x = 0; x < tile_size; x++) {
//             for (size_t y = 0; y < tile_size; y++) {
//                 vga_gm_draw::pixel(p_buffer, tile_size * p_tile->grid_x + offset_x + x, tile_size * p_tile->grid_y + offset_y + y, (vga_gm_color_index_t)g_tile_unrevealed[y][x]);
//             }
//         }

//         return;
//     }

//     // revealed

//     // is bomb
//     if (p_tile->is_bomb) {
//         for (size_t x = 0; x < tile_size; x++) {
//             for (size_t y = 0; y < tile_size; y++) {
//                 vga_gm_draw::pixel(p_buffer, tile_size * p_tile->grid_x + offset_x + x, tile_size * p_tile->grid_y + offset_y + y, (vga_gm_color_index_t)g_tile_bomb_exploded[y][x]);
//             }
//         }

//         return;
//     }

//     // is nothing
//     for (size_t x = 0; x < tile_size; x++) {
//         for (size_t y = 0; y < tile_size; y++) {
//             switch (p_tile->bomb_count) {
//             case 1:
//                 vga_gm_draw::pixel(p_buffer, tile_size * p_tile->grid_x + offset_x + x, tile_size * p_tile->grid_y + offset_y + y, (vga_gm_color_index_t)g_tile_bomb1[y][x]);
//                 break;
//             case 2:
//                 vga_gm_draw::pixel(p_buffer, tile_size * p_tile->grid_x + offset_x + x, tile_size * p_tile->grid_y + offset_y + y, (vga_gm_color_index_t)g_tile_bomb2[y][x]);
//                 break;
//             case 3:
//                 vga_gm_draw::pixel(p_buffer, tile_size * p_tile->grid_x + offset_x + x, tile_size * p_tile->grid_y + offset_y + y, (vga_gm_color_index_t)g_tile_bomb3[y][x]);
//                 break;
//             case 4:
//                 vga_gm_draw::pixel(p_buffer, tile_size * p_tile->grid_x + offset_x + x, tile_size * p_tile->grid_y + offset_y + y, (vga_gm_color_index_t)g_tile_bomb4[y][x]);
//                 break;
//             case 5:
//                 vga_gm_draw::pixel(p_buffer, tile_size * p_tile->grid_x + offset_x + x, tile_size * p_tile->grid_y + offset_y + y, (vga_gm_color_index_t)g_tile_bomb5[y][x]);
//                 break;
//             case 6:
//                 vga_gm_draw::pixel(p_buffer, tile_size * p_tile->grid_x + offset_x + x, tile_size * p_tile->grid_y + offset_y + y, (vga_gm_color_index_t)g_tile_bomb6[y][x]);
//                 break;
//             case 7:
//                 vga_gm_draw::pixel(p_buffer, tile_size * p_tile->grid_x + offset_x + x, tile_size * p_tile->grid_y + offset_y + y, (vga_gm_color_index_t)g_tile_bomb7[y][x]);
//                 break;
//             case 8:
//                 vga_gm_draw::pixel(p_buffer, tile_size * p_tile->grid_x + offset_x + x, tile_size * p_tile->grid_y + offset_y + y, (vga_gm_color_index_t)g_tile_bomb8[y][x]);
//                 break;
//             case 0:
//             default:
//                 vga_gm_draw::pixel(p_buffer, tile_size * p_tile->grid_x + offset_x + x, tile_size * p_tile->grid_y + offset_y + y, (vga_gm_color_index_t)g_tile_empty[y][x]);
//                 break;
//             }
//         }
//     }
// }

// bool is_pos_valid(int x, int y) {
//     if (x < 0 || y < 0)
//         return false;

//     return x < game_board_width && y < game_board_height;
// }

// void tile_reveal(tile_t p_board[16][16], tile_t* p_tile) {
//     if (p_tile->is_revealed || p_tile->is_marked)
//         return;

//     if (p_tile->is_bomb)
//         p_tile->has_clicked_bomb = true;

//     p_tile->is_revealed = true;

//     if (p_tile->bomb_count >= 1)
//         return;

//     for (int i = -1; i < 2; i++) {
//         for (int j = -1; j < 2; j++) {
//             if (!is_pos_valid(p_tile->grid_x + i, p_tile->grid_y + j) || (i == 0 && j == 0))
//                 continue;

//             tile_t* p_tile_other = &p_board[p_tile->grid_x + i][p_tile->grid_y + j];
//             if (!p_tile_other->is_bomb && !p_tile_other->is_marked)
//                 tile_reveal(p_board, p_tile_other);
//         }
//     }
// }

// void tile_mark(tile_t* p_tile) {
//     if (p_tile->is_revealed)
//         return;

//     p_tile->is_marked = !p_tile->is_marked;
// }

// int desktop_init() {
//     g_last_tick = clock_get_time_since_boot();
//     vga_buffer_t buff {};
//     vga_gm_buffer_create(&buff);
//     vga_gm_startup(&buff);
//     vga_gm_draw::clear(&buff, vga_gm_color_index_t::WHITE);
    
//     seed_random(clock_get_time_since_boot());

//     ps2_mouse_event_subscribe(desktop_handle_mouse_input);

//     constexpr uint64_t frame_time_ms = 1000 / 60;

//     tile_t game_board[game_board_width][game_board_height] {};

//     for (int i = 0; i < game_board_width; i++) {
//         for (int j = 0; j < game_board_height; j++) {
//             auto& tile = game_board[i][j];
//             tile.grid_x = i;
//             tile.grid_y = j;
//         }
//     }

//     dynamic_array<dynamic_array<int>> bomb_spots {};
//     for (int i = 0; i < game_board_width; i++) {
//         for (int j = 0; j < game_board_height; j++) {
//             bomb_spots.insert_back( dynamic_array<int>{ i, j } );
//         }
//     }

//     for (int n = 0; n < bomb_count; n++) {
//         int index = random_number() % bomb_spots.length();
//         int x = bomb_spots[index][0];
//         int y = bomb_spots[index][1];
//         bomb_spots.delete_at(index);
//         game_board[x][y].is_bomb = true;
//     }

//     for (int x = 0; x < game_board_width; x++) {
//         for (int y = 0; y < game_board_height; y++) {
//             tile_t* p_tile = &game_board[x][y];

//             for (int i = -1; i < 2; i++) {
//                 for (int j = -1; j < 2; j++) {
//                     if (!is_pos_valid(x + i, y + j) || (i == 0 && j == 0))
//                         continue;

//                     tile_t* p_tile_other = &game_board[x + i][y + j];
//                     if (p_tile_other->is_bomb)
//                         p_tile->bomb_count++;
//                 }
//             }

//         }
//     }

//     while (true) {      
//         uint64_t now = clock_get_time_since_boot();

//         if (now - g_last_tick < frame_time_ms) {
//             // vthread_sleep(frame_time_ms - (now - g_last_tick));
//             continue;
//         }

//         vga_gm_draw::clear(&buff, vga_gm_color_index_t::WHITE);

//         for (int i = 0; i < game_board_width; i++) {
//             for (int j = 0; j < game_board_height; j++) {
//                 tile_t* p_tile = &game_board[i][j];

//                 if (g_desktop_lmb_pressed && g_lmb_just_pressed &&
//                     g_desktop_mouse_pos_x > p_tile->grid_x * tile_size && g_desktop_mouse_pos_x < p_tile->grid_x * tile_size + tile_size &&
//                     g_desktop_mouse_pos_y > p_tile->grid_y * tile_size && g_desktop_mouse_pos_y < p_tile->grid_y * tile_size + tile_size)
//                     tile_reveal(game_board, p_tile);

//                 if (g_desktop_rmb_pressed && g_rmb_just_pressed &&
//                     g_desktop_mouse_pos_x > p_tile->grid_x * tile_size && g_desktop_mouse_pos_x < p_tile->grid_x * tile_size + tile_size &&
//                     g_desktop_mouse_pos_y > p_tile->grid_y * tile_size && g_desktop_mouse_pos_y < p_tile->grid_y * tile_size + tile_size)
//                     tile_mark(p_tile);

//                 draw_tile(&buff, p_tile, 0, 0);
//             }
//         }

//         desktop_draw_cursor(&buff, g_desktop_mouse_pos_x, g_desktop_mouse_pos_y);

//         vga_gm_render();

//         g_last_tick = now;
//     }

//     return 0;
// }

static vga_buffer_t* g_desktop_back_buffer = nullptr;
static int g_desktop_mouse_pos[2] { 0, 0 };
static linear_map<uint64_t, event_manager_t<void*>> g_desktop_event_container {};
constexpr uint64_t k_desktop_target_fps_ms = 1000 / 60;

struct desktop_event_on_mouse_move_t {
    int x;
    int y;
};

struct desktop_event_on_mouse_scroll_t {
    int d;
};

enum class desktop_event_mouse_button_type_t {
    LEFT,
    MIDDLE,
    RIGHT
};

struct desktop_event_on_mouse_button_t {
    desktop_event_mouse_button_type_t key;
};

vga_buffer_t* desktop_render_get_buffer() {
    return g_desktop_back_buffer;
}

void desktop_handle_ps2_mouse_input(const ps2_mouse_state_t* p_state) {
    // on mouse move
    if (p_state->dx != 0 || p_state->dy != 0) {
        // update mouse pos
        g_desktop_mouse_pos[0] += p_state->dx;
        g_desktop_mouse_pos[0] = CLAMP(g_desktop_mouse_pos[0], 0, VGA_GM_BUFFER_WIDTH - 1);
        g_desktop_mouse_pos[1] += p_state->dy;
        g_desktop_mouse_pos[1] = CLAMP(g_desktop_mouse_pos[1], 0, VGA_GM_BUFFER_HEIGHT - 1);

        // call event
        desktop_event_on_mouse_move_t event {};
        event.x = g_desktop_mouse_pos[0];
        event.y = g_desktop_mouse_pos[1];
        g_desktop_event_container[hash_fnv1a_64("on_mouse_move")].fire_event((void*)&event);
    }
    
    // on mouse button press & release
    if (static bool s_lmb_last = false; s_lmb_last != p_state->buttons.left) {
        // call event
        desktop_event_on_mouse_button_t event {};
        event.key = desktop_event_mouse_button_type_t::LEFT;

        if (p_state->buttons.left)
            g_desktop_event_container[hash_fnv1a_64("on_mouse_pressed")].fire_event((void*)&event);
        else
            g_desktop_event_container[hash_fnv1a_64("on_mouse_released")].fire_event((void*)&event);

        s_lmb_last = p_state->buttons.left;
    }

    if (static bool s_rmb_last = false; s_rmb_last != p_state->buttons.right) {
        // call event
        desktop_event_on_mouse_button_t event {};
        event.key = desktop_event_mouse_button_type_t::RIGHT;

        if (p_state->buttons.right)
            g_desktop_event_container[hash_fnv1a_64("on_mouse_pressed")].fire_event((void*)&event);
        else
            g_desktop_event_container[hash_fnv1a_64("on_mouse_released")].fire_event((void*)&event);

        s_rmb_last = p_state->buttons.right;
    }

    if (static bool s_mmb_last = false; s_mmb_last != p_state->buttons.middle) {
        // call event
        desktop_event_on_mouse_button_t event {};
        event.key = desktop_event_mouse_button_type_t::RIGHT;

        if (p_state->buttons.middle)
            g_desktop_event_container[hash_fnv1a_64("on_mouse_pressed")].fire_event((void*)&event);
        else
            g_desktop_event_container[hash_fnv1a_64("on_mouse_released")].fire_event((void*)&event);

        s_mmb_last = p_state->buttons.middle;
    }

    // on scroll
    if (p_state->ds != 0) {
        // call event
        desktop_event_on_mouse_scroll_t event {};
        event.d = p_state->ds;
        g_desktop_event_container[hash_fnv1a_64("on_mouse_scroll")].fire_event((void*)&event);
    }
}

template <typename T>
bool desktop_event_subscribe(const char* p_name, void(*p_callback)(T*)) {
    const uint64_t hash = hash_fnv1a_64(p_name);

    auto it = g_desktop_event_container.get(hash);
    if (it == g_desktop_event_container.end())
        return false;

    g_desktop_event_container[hash].subscribe((void(*)(void*))p_callback);
    return true;
}

void desktop_init_hardware_handlers() {
    // setup events
    g_desktop_event_container.insert(hash_fnv1a_64("on_mouse_move"), event_manager_t<void*>{});
    g_desktop_event_container.insert(hash_fnv1a_64("on_mouse_scroll"), event_manager_t<void*>{});
    g_desktop_event_container.insert(hash_fnv1a_64("on_mouse_pressed"), event_manager_t<void*>{});
    g_desktop_event_container.insert(hash_fnv1a_64("on_mouse_release"), event_manager_t<void*>{});

    // start with mouse in the middle
    g_desktop_mouse_pos[0] = VGA_GM_BUFFER_WIDTH / 2;
    g_desktop_mouse_pos[1] = VGA_GM_BUFFER_HEIGHT / 2;

    // add ps2 mouse input support
    ps2_mouse_event_subscribe(desktop_handle_ps2_mouse_input);
}

void desktop_render_init() {
    // initialize render buffer
    g_desktop_back_buffer = (vga_buffer_t*)heap_alloc(get_global_heap(), sizeof(vga_buffer_t));
    vga_gm_buffer_create(desktop_render_get_buffer());

    // initialize vga -> our render target
    vga_gm_startup(desktop_render_get_buffer());

    // start with someting clean
    vga_gm_draw::clear(desktop_render_get_buffer(), vga_gm_color_index_t::BLACK);
    vga_gm_render();
}

void desktop_render_clear_buffer() {
    vga_gm_draw::clear(desktop_render_get_buffer(), vga_gm_color_index_t::BLACK);
}

void desktop_render_end() {
    vga_gm_render();
}

void desktop_render_draw_cursor() {
    const uint64_t x = g_desktop_mouse_pos[0];
    const uint64_t y = g_desktop_mouse_pos[1];

    // outline
    vga_gm_draw::linev(desktop_render_get_buffer(), x, y, 10, vga_gm_color_index_t::BLACK);
    vga_gm_draw::pixel(desktop_render_get_buffer(), x + 1, y + 1, vga_gm_color_index_t::BLACK);
    vga_gm_draw::pixel(desktop_render_get_buffer(), x + 2, y + 2, vga_gm_color_index_t::BLACK);
    vga_gm_draw::pixel(desktop_render_get_buffer(), x + 3, y + 3, vga_gm_color_index_t::BLACK);
    vga_gm_draw::pixel(desktop_render_get_buffer(), x + 4, y + 4, vga_gm_color_index_t::BLACK);
    vga_gm_draw::pixel(desktop_render_get_buffer(), x + 5, y + 5, vga_gm_color_index_t::BLACK);
    vga_gm_draw::pixel(desktop_render_get_buffer(), x + 6, y + 6, vga_gm_color_index_t::BLACK);
    vga_gm_draw::lineh(desktop_render_get_buffer(), x + 4, y + 7, 3, vga_gm_color_index_t::BLACK);
    vga_gm_draw::pixel(desktop_render_get_buffer(), x + 1, y + 9, vga_gm_color_index_t::BLACK);
    vga_gm_draw::pixel(desktop_render_get_buffer(), x + 2, y + 8, vga_gm_color_index_t::BLACK);
    vga_gm_draw::pixel(desktop_render_get_buffer(), x + 3, y + 7, vga_gm_color_index_t::BLACK);

    // inline
    vga_gm_draw::linev(desktop_render_get_buffer(), x + 1, y + 2, 7, vga_gm_color_index_t::WHITE);
    vga_gm_draw::linev(desktop_render_get_buffer(), x + 2, y + 3, 5, vga_gm_color_index_t::WHITE);
    vga_gm_draw::linev(desktop_render_get_buffer(), x + 3, y + 4, 3, vga_gm_color_index_t::WHITE);
    vga_gm_draw::linev(desktop_render_get_buffer(), x + 4, y + 5, 2, vga_gm_color_index_t::WHITE);
    vga_gm_draw::linev(desktop_render_get_buffer(), x + 5, y + 6, 1, vga_gm_color_index_t::WHITE);
}

int desktop_init() {
    // setup renderer
    desktop_render_init();

    // setup input devices
    desktop_init_hardware_handlers();

    // main render loop
    uint64_t g_last_tick = 0;
    while (true) {
        // wait for new frame
        uint64_t now = clock_get_time_since_boot();

        if (now - g_last_tick < k_desktop_target_fps_ms) {
            vthread_sleep(k_desktop_target_fps_ms - (now - g_last_tick));
            continue;
        }

        desktop_render_clear_buffer();

        // TODO @since 09/09/2025 -- 22:00
        // drawing here

        desktop_render_draw_cursor();

        desktop_render_end();
        g_last_tick = now;
    }

    return 0;
}