//==========================================
/// @file       minesweeper.hpp
/// @brief      
//==========================================

#pragma once

#ifndef MINESWEEPER_HPP
#define MINESWEEPER_HPP

#include "common.hpp"

struct tile_t {
    i32 bomb_count;
    i32 mark_count;

    i32 x, y;

    bool is_bomb;
    bool is_marked;
    bool is_revealed;
    bool is_peeking;

    bool has_clicked;
};

struct game_config_t {
    struct {
        u32 width;
        u32 height;
    } size;

    i32 bomb_count;
};

struct sprite_t {
    u32 sprite_size;
    u32* data;
};

struct game_t {
    u32 active_game_config;
    tile_t* game_board;
    u64 window_handle;
    bool is_running;

    u32 board_offsetx;
    u32 board_offsety;

    sprite_t tile_empty;
    sprite_t tile_bomb1;
    sprite_t tile_bomb2;
    sprite_t tile_bomb3;
    sprite_t tile_bomb4;
    sprite_t tile_bomb5;
    sprite_t tile_bomb6;
    sprite_t tile_bomb7;
    sprite_t tile_bomb8;
    sprite_t tile_bomb;
    sprite_t tile_bomb_exploded;
    sprite_t tile_unrevealed;
    sprite_t tile_marked;
    sprite_t tile_marked_wrong;

    sprite_t smiley_happy;
    sprite_t smiley_happy_pressed;
    sprite_t smiley_dead;
    sprite_t smiley_cool;

    u32 smileyx, smileyy;
};

typedef void(*tile_loop_function_t)(game_t* game, tile_t* tile);
typedef void(*tile_loop_neigbour_function_t)(game_t* game, tile_t* current_tile, tile_t* origin_tile);

static const game_config_t game_configs[3] {
    { .size = { 9, 9 }, .bomb_count = 10 },
    { .size = { 16, 16 }, .bomb_count = 40 },
    { .size = { 30, 16 }, .bomb_count = 99 }
};

bool is_valid_grid_pos(game_t* game, i32 x, i32 y);

void loop_game_board(game_t* game, tile_loop_function_t callback);

void loop_neighbour_tiles(game_t* game, tile_t* tile, tile_loop_neigbour_function_t callback);

tile_t* get_tile_from_pos(game_t* game, int x, int y);

tile_t* get_tile_at(game_t* game, u32 x, u32 y);

void minesweeper_end_game(game_t* game);

void tile_reveal(game_t* game, tile_t* tile);

bool game_board_create(game_t* game);

void tile_mark(tile_t* tile);

void tile_peek_nearby(game_t* game, tile_t* tile);

#endif // MINESWEEPER_HPP