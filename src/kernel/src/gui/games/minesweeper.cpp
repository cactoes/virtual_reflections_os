#include "gui/games/minesweeper.hpp"
#include "gui/desktop.hpp"
#include "utils/vector.hpp"
#include "memory/heap.hpp"
#include "random.hpp"

struct tile_t {
    int bomb_count;
    int mark_count;

    bool is_bomb;
    bool is_marked;
    bool is_revealed;
    bool is_peeking;

    bool has_clicked;
    
    uint64_t grid_x;
    uint64_t grid_y;
};

struct game_config_t {
    struct {
        int width;
        int height;
    } size;
    
    int bomb_count;
};

static constexpr game_config_t game_configs[3] {
    { .size = { 9, 9 }, .bomb_count = 10 },
    { .size = { 16, 16 }, .bomb_count = 40 },
    { .size = { 30, 16 }, .bomb_count = 99 }
};

constexpr int tile_size = 10;
constexpr int current_config = 0;
constexpr auto game_config = game_configs[current_config];

static tile_t game_board[game_configs[current_config].size.width][game_configs[current_config].size.height] {};
static bool is_game_running = false;

const uint8_t tile_empty[tile_size][tile_size] {
    8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
};
const uint8_t tile_bomb1[tile_size][tile_size] { // 9u
    8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 9u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 9u, 9u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 9u, 7u, 9u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 9u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 9u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 9u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 9u, 9u, 9u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
};
const uint8_t tile_bomb2[tile_size][tile_size] { // 2u
    8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 2u, 2u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 2u, 7u, 7u, 2u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 2u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 2u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 2u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 2u, 7u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 2u, 2u, 2u, 2u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
};
const uint8_t tile_bomb3[tile_size][tile_size] { // 4u
    8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 4u, 4u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 4u, 7u, 7u, 4u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 4u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 4u, 4u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 4u, 7u, 7u, 7u,
    8u, 7u, 7u, 4u, 7u, 7u, 4u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 4u, 4u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
};
const uint8_t tile_bomb4[tile_size][tile_size] { // 1u
    8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 1u, 7u, 7u, 1u, 7u, 7u, 7u,
    8u, 7u, 7u, 1u, 7u, 7u, 1u, 7u, 7u, 7u,
    8u, 7u, 7u, 1u, 7u, 7u, 1u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 1u, 1u, 1u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 1u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 1u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 1u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
};
const uint8_t tile_bomb5[tile_size][tile_size] { // 6u
    8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 6u, 6u, 6u, 6u, 7u, 7u, 7u,
    8u, 7u, 7u, 6u, 7u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 6u, 6u, 6u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 6u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 6u, 7u, 7u, 7u,
    8u, 7u, 7u, 6u, 7u, 7u, 6u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 6u, 6u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
};
const uint8_t tile_bomb6[tile_size][tile_size] { // 3u
    8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 3u, 3u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 3u, 7u, 7u, 3u, 7u, 7u, 7u,
    8u, 7u, 7u, 3u, 7u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 3u, 3u, 3u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 3u, 7u, 7u, 3u, 7u, 7u, 7u,
    8u, 7u, 7u, 3u, 7u, 7u, 3u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 3u, 3u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
};
const uint8_t tile_bomb7[tile_size][tile_size] { // 0u
    8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 0u, 0u, 0u, 0u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 0u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 0u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 0u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 0u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 0u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 0u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
};
const uint8_t tile_bomb8[tile_size][tile_size] { // 8u
    8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 8u, 8u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 8u, 7u, 7u, 8u, 7u, 7u, 7u,
    8u, 7u, 7u, 8u, 7u, 7u, 8u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 8u, 8u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 8u, 7u, 7u, 8u, 7u, 7u, 7u,
    8u, 7u, 7u, 8u, 7u, 7u, 8u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 8u, 8u, 7u, 7u, 7u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
};
const uint8_t tile_bomb[tile_size][tile_size] {
    8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
    8u, 7u, 0u, 7u, 7u, 0u, 7u, 7u, 0u, 7u,
    8u, 7u, 7u, 7u, 0u, 0u, 0u, 7u, 7u, 7u,
    8u, 7u, 7u, 0u, 15, 0u, 0u, 0u, 7u, 7u,
    8u, 7u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 7u,
    8u, 7u, 7u, 0u, 0u, 0u, 0u, 0u, 7u, 7u,
    8u, 7u, 7u, 7u, 0u, 0u, 0u, 7u, 7u, 7u,
    0u, 7u, 0u, 7u, 7u, 0u, 7u, 7u, 0u, 7u,
    8u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
};
const uint8_t tile_bomb_exploded[tile_size][tile_size] {
    8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
    8u, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    8u, 12, 0u, 12, 12, 0u, 12, 12, 0u, 12,
    8u, 12, 12, 12, 0u, 0u, 0u, 12, 12, 12,
    8u, 12, 12, 0u, 15, 0u, 0u, 0u, 12, 12,
    8u, 12, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 12,
    8u, 12, 12, 0u, 0u, 0u, 0u, 0u, 12, 12,
    8u, 12, 12, 12, 0u, 0u, 0u, 12, 12, 12,
    0u, 12, 0u, 12, 12, 0u, 12, 12, 0u, 12,
    8u, 12, 12, 12, 12, 12, 12, 12, 12, 12,
};
const uint8_t tile_unrevealed[tile_size][tile_size] {
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 8u,
    15, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 8u,
    15, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 8u,
    15, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 8u,
    15, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 8u,
    15, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 8u,
    15, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 8u,
    15, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 8u,
    8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
};
const uint8_t tile_marked[tile_size][tile_size] {
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 8u,
    15, 7u, 7u, 7u, 4u, 4u, 7u, 7u, 7u, 8u,
    15, 7u, 4u, 4u, 4u, 4u, 7u, 7u, 7u, 8u,
    15, 7u, 7u, 4u, 4u, 0u, 7u, 7u, 7u, 8u,
    15, 7u, 7u, 7u, 7u, 0u, 7u, 7u, 7u, 8u,
    15, 7u, 7u, 7u, 0u, 0u, 0u, 7u, 7u, 8u,
    15, 7u, 7u, 0u, 0u, 0u, 0u, 0u, 7u, 8u,
    15, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 8u,
    8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
};
const uint8_t tile_marked_wrong[tile_size][tile_size] {
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 12, 12, 12, 12, 12, 12, 12, 12, 8u,
    15, 12, 12, 12, 4u, 4u, 12, 12, 12, 8u,
    15, 12, 4u, 4u, 4u, 4u, 12, 12, 12, 8u,
    15, 12, 12, 4u, 4u, 0u, 12, 12, 12, 8u,
    15, 12, 12, 12, 12, 0u, 12, 12, 12, 8u,
    15, 12, 12, 12, 0u, 0u, 0u, 12, 12, 8u,
    15, 12, 12, 0u, 0u, 0u, 0u, 0u, 12, 8u,
    15, 12, 12, 12, 12, 12, 12, 12, 12, 8u,
    8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,
};

bool is_valid_grid_pos(int x, int y) {
    if (x < 0 || y < 0)
        return false;

    return x < game_config.size.width && y < game_config.size.height;
}

template <typename func>
void loop_game_board(func&& callback) {
    for (int i = 0; i < game_config.size.width; i++)
        for (int j = 0; j < game_config.size.height; j++)
            callback(&game_board[i][j]);
}

template <typename func>
void loop_neighbour_tiles(tile_t* tile, func&& callback) {
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            if (!is_valid_grid_pos(tile->grid_x + i, tile->grid_y + j) || (i == 0 && j == 0))
                continue;

            tile_t* tile_other = &game_board[tile->grid_x + i][tile->grid_y + j];
            callback(tile_other);
        }
    }
}

bool is_in_tile_hitbox(tile_t* tile, int x, int y) {
    return x > tile->grid_x * tile_size && x < tile->grid_x * tile_size + tile_size &&
           y > tile->grid_y * tile_size && y < tile->grid_y * tile_size + tile_size;
}

tile_t* get_tile_from_pos(int x, int y) {
    if (x > game_config.size.width * tile_size ||
        y > game_config.size.height * tile_size ||
        x < 0 || y < 0)
        return nullptr;

    return &game_board[x / tile_size][y / tile_size];
}

void minesweeper_end_game() {
    is_game_running = false;
    loop_game_board([](tile_t* tile) {
        if (tile->is_bomb && !tile->is_marked)
            tile->is_revealed = true;
    });
}

void tile_reveal(tile_t* tile) {
    if (tile->is_revealed || tile->is_marked)
        return;

    tile->is_revealed = true;

    if (tile->is_bomb) {
        minesweeper_end_game();
        return;
    }

    if (tile->bomb_count >= 1)
        return;

    loop_neighbour_tiles(tile, [](tile_t* other) {
        if (!other->is_bomb && !other->is_marked)
            tile_reveal(other);
    });
}

void tile_mark(tile_t* tile) {
    if (tile->is_revealed)
        return;

    tile->is_marked = !tile->is_marked;
}

void tile_peek_nearby(tile_t* tile) {
    loop_neighbour_tiles(tile, [](tile_t* other) {
        if (!other->is_revealed && !other->is_marked)
            other->is_peeking = true;
    });
}

void minesweeper_on_mouse_up(const desktop_event_on_mouse_button_t* event) {
    if (!is_game_running)
        return;

    int mouse_x, mouse_y;
    desktop_get_cursor_pos(&mouse_x, &mouse_y);

    tile_t* tile = get_tile_from_pos(mouse_x, mouse_y);
    if (!tile)
        return;

    switch (event->key) {
        case desktop_event_mouse_button_type_t::LEFT:
            if (tile->is_revealed)
                tile_peek_nearby(tile);
            else
                tile->is_peeking = true;
            break;
    }
}

void minesweeper_on_mouse_down(const desktop_event_on_mouse_button_t* event) {
    if (!is_game_running)
        return;

    int mouse_x, mouse_y;
    desktop_get_cursor_pos(&mouse_x, &mouse_y);

    tile_t* tile = get_tile_from_pos(mouse_x, mouse_y);
    if (!tile)
        return;

    switch (event->key) {
        case desktop_event_mouse_button_type_t::LEFT:
            tile->has_clicked = true;
            tile_reveal(tile);
            break;
        case desktop_event_mouse_button_type_t::RIGHT:
            tile_mark(tile);
            break;
    }

    int total_unrevealed = 0;
    loop_game_board([&total_unrevealed](tile_t* tile) {
        if (!tile->is_revealed)
            total_unrevealed++;
    });

    if (total_unrevealed == game_config.bomb_count)
        minesweeper_end_game();

    // remove peeking
    loop_game_board([](tile_t* tile) {
        if (tile->is_bomb)
            return;

        loop_neighbour_tiles(tile, [](tile_t* other) {
            if (other->is_peeking)
                other->is_peeking = false;
        });
    });
}

void minesweeper_init() {
    // register desktop stuff
    desktop_event_subscribe(DESKTOP_EVENT_MOUSE_RELEASED, minesweeper_on_mouse_down);
    desktop_event_subscribe(DESKTOP_EVENT_MOUSE_PRESSED, minesweeper_on_mouse_up);
    desktop_register_target(minesweeper_render_target);

    // random
    seed_random(809429);

    // setup game board
    // if (g_game_board)
    //     heap_free(get_global_heap(), g_game_board);
    
    // g_game_board = (tile_t**)heap_alloc(get_global_heap(), sizeof(tile_t) * game_config.size.width * game_config.size.height);
    // if (!g_game_board)
    //     return;

    // init the tiles
    for (int i = 0; i < game_config.size.width; i++) {
        for (int j = 0; j < game_config.size.height; j++) {
            tile_t* tile = &game_board[i][j];
            tile->grid_x = i;
            tile->grid_y = j;
        }
    }

    // setup bombs
    dynamic_array<static_array<int, 2>> bomb_spots {};
    bomb_spots.resize(game_config.size.width * game_config.size.height);

    loop_game_board([&bomb_spots](tile_t* tile) { bomb_spots.insert_back({ tile->grid_x, tile->grid_y }); });

    for (int n = 0; n <= game_config.bomb_count; n++) {
        int index = random_number() % bomb_spots.length();
        int x = bomb_spots[index][0];
        int y = bomb_spots[index][1];
        bomb_spots.delete_at(index);
        game_board[x][y].is_bomb = true;
    }

    // setup bomb counters
    loop_game_board([](tile_t* tile) {
        if (tile->is_bomb)
            return;

        loop_neighbour_tiles(tile, [tile](tile_t* other) {
            if (other->is_bomb)
                tile->bomb_count++;
        });
    });

    is_game_running = true;
}

void tile_render(tile_t* tile, uint64_t offset_x, uint64_t offset_y, const uint8_t sprite[tile_size][tile_size]) {
    for (size_t x = 0; x < tile_size; x++)
        for (size_t y = 0; y < tile_size; y++)
            desktop_render_pixel(tile_size * tile->grid_x + offset_x + x,
                tile_size * tile->grid_y + offset_y + y,
                sprite[y][x]);
}

void minesweeper_render_tile(tile_t* tile, uint64_t offset_x, uint64_t offset_y) {
    if (tile->is_peeking) {
        tile_render(tile, offset_x, offset_y, tile_empty);
        return;
    }

    // not revealed
    if (!tile->is_revealed) {
        // wrong marked eog
        if (!is_game_running && tile->is_marked && !tile->is_bomb) {
            tile_render(tile, offset_x, offset_y, tile_marked_wrong);
            return;
        }

        // marked
        if (tile->is_marked) {
            tile_render(tile, offset_x, offset_y, tile_marked);
            return;
        }

        // nothing
        tile_render(tile, offset_x, offset_y, tile_unrevealed);
        return;
    }

    // revealed

    // is bomb
    if (tile->is_bomb) {
        if (tile->has_clicked)
            tile_render(tile, offset_x, offset_y, tile_bomb_exploded);
        else
            tile_render(tile, offset_x, offset_y, tile_bomb);
        return;
    }

    // is nothing
    switch (tile->bomb_count) {
        case 1: tile_render(tile, offset_x, offset_y, tile_bomb1); break;
        case 2: tile_render(tile, offset_x, offset_y, tile_bomb2); break;
        case 3: tile_render(tile, offset_x, offset_y, tile_bomb3); break;
        case 4: tile_render(tile, offset_x, offset_y, tile_bomb4); break;
        case 5: tile_render(tile, offset_x, offset_y, tile_bomb5); break;
        case 6: tile_render(tile, offset_x, offset_y, tile_bomb6); break;
        case 7: tile_render(tile, offset_x, offset_y, tile_bomb7); break;
        case 8: tile_render(tile, offset_x, offset_y, tile_bomb8); break;
        case 0:
        default: tile_render(tile, offset_x, offset_y, tile_empty); break;
    }
}

void minesweeper_render_target(uint64_t dt) {
    loop_game_board([](tile_t* tile) {
        minesweeper_render_tile(tile, 0, 0);
    });
}