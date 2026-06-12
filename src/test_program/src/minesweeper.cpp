#include "minesweeper.hpp"
#include "std/random.hpp"

bool is_valid_grid_pos(game_t* game, i32 x, i32 y) {
    if (x < 0 || y < 0)
        return false;

    const game_config_t& config = game_configs[game->active_game_config];

    return x < config.size.width && y < config.size.height;
}

void loop_game_board(game_t* game, tile_loop_function_t callback) {
    const game_config_t& config = game_configs[game->active_game_config];

    for (u32 x = 0; x < config.size.width; x++)
        for (u32 y = 0; y < config.size.height; y++)
            callback(game, &game->game_board[x + config.size.width * y]);
}

void loop_neighbour_tiles(game_t* game, tile_t* tile, tile_loop_neigbour_function_t callback) {
    const game_config_t& config = game_configs[game->active_game_config];

    for (i32 i = -1; i < 2; i++) {
        for (i32 j = -1; j < 2; j++) {
            if (!is_valid_grid_pos(game, tile->x + i, tile->y + j) || (i == 0 && j == 0))
                continue;

            tile_t* tile_other = &game->game_board[(tile->x + i) + config.size.width * (tile->y + j)];
            callback(game, tile_other, tile);
        }
    }
}

tile_t* get_tile_from_pos(game_t* game, int x, int y) {
    const game_config_t& config = game_configs[game->active_game_config];

    const u32 tile_size = game->tile_empty.sprite_size;

    if (x >= config.size.width * tile_size ||
        y >= config.size.height * tile_size ||
        x < 0 || y < 0)
        return nullptr;

    return &game->game_board[(x / tile_size) + config.size.width * (y / tile_size)];
}

tile_t* get_tile_at(game_t* game, u32 x, u32 y) {
    const game_config_t& config = game_configs[game->active_game_config];
    return &game->game_board[x + config.size.width * y];
}

void minesweeper_end_game(game_t* game, bool won) {
    game->is_running = false;
    game->has_won = won;

    loop_game_board(game, [](game_t* game, tile_t* tile) {
        if (tile->is_bomb && !tile->is_marked)
            tile->is_revealed = true;
    });
}

void tile_reveal(game_t* game, tile_t* tile) {
    if (tile->is_revealed || tile->is_marked)
        return;

    tile->is_revealed = true;

    if (tile->is_bomb) {
        minesweeper_end_game(game, false);
        return;
    }

    if (tile->bomb_count >= 1)
        return;

    loop_neighbour_tiles(game, tile, [](game_t* game, tile_t* current_tile, tile_t*) {
        if (!current_tile->is_bomb && !current_tile->is_marked)
            tile_reveal(game, current_tile);
    });
}

bool game_board_create(game_t* game) {
    if (game->game_board != nullptr)
        return false;

    const game_config_t& config = game_configs[game->active_game_config];

    game->game_board = (tile_t*)malloc(config.size.height * config.size.width * sizeof(tile_t));
    if (game->game_board == nullptr)
        return false;

    memzero(game->game_board, config.size.height * config.size.width * sizeof(tile_t));

    for (u32 i = 0; i < config.size.height * config.size.width; i++) {
        (game->game_board)[i].x = i % config.size.width;
        (game->game_board)[i].y = i / config.size.width;
    }

    u32 bomb_positions_count = config.size.height * config.size.width;
    u32* bomb_positions = (u32*)malloc(bomb_positions_count * sizeof(u32) * 2);
    for (u32 i = 0; i < bomb_positions_count; i++) {
        u32 x = i % config.size.width;
        u32 y = i / config.size.width;
        bomb_positions[i * 2 + 0] = x;
        bomb_positions[i * 2 + 1] = y;
    }

    for (u32 n = 0; n < config.bomb_count; n++) {
        u32 index = random_number() % bomb_positions_count;
        u32 x = bomb_positions[index * 2 + 0];
        u32 y = bomb_positions[index * 2 + 1];

        bomb_positions_count--;

        bomb_positions[index * 2 + 0] = bomb_positions[bomb_positions_count * 2 + 0];
        bomb_positions[index * 2 + 1] = bomb_positions[bomb_positions_count * 2 + 1];

        (game->game_board)[x + config.size.width * y].is_bomb = true;
    }

    loop_game_board(game, [](game_t* game, tile_t* tile) {
        if (tile->is_bomb)
            return;

        loop_neighbour_tiles(game, tile, [](game_t* game, tile_t* current_tile, tile_t* origin_tile) {
            if (current_tile->is_bomb)
                origin_tile->bomb_count++;
        });
    });

    free(bomb_positions);

    game->is_running = false;

    return true;
}

void tile_mark(tile_t* tile) {
    if (tile->is_revealed)
        return;

    tile->is_marked = !tile->is_marked;
}

void tile_peek_nearby(game_t* game, tile_t* tile) {
    loop_neighbour_tiles(game, tile, [](game_t* game, tile_t* current_tile, tile_t* origin_tile) {
        if (!current_tile->is_revealed && !current_tile->is_marked)
            current_tile->is_peeking = true;
    });
}