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

static constexpr game_config_t k_game_configs[3] {
    {
        .size = { 9, 9 },
        .bomb_count = 10
    },
    {
        .size = { 16, 16 },
        .bomb_count = 40
    },
    {
        .size = { 30, 16 },
        .bomb_count = 99
    }
};

constexpr int k_tile_size = 10;
constexpr int k_current_config = 1;
static tile_t g_game_board[k_game_configs[k_current_config].size.width][k_game_configs[k_current_config].size.height] {};

const uint8_t k_tile_empty[k_tile_size][k_tile_size] {
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
const uint8_t k_tile_bomb1[k_tile_size][k_tile_size] { // 9u
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
const uint8_t k_tile_bomb2[k_tile_size][k_tile_size] { // 2u
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
const uint8_t k_tile_bomb3[k_tile_size][k_tile_size] { // 4u
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
const uint8_t k_tile_bomb4[k_tile_size][k_tile_size] { // 1u
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
const uint8_t k_tile_bomb5[k_tile_size][k_tile_size] { // 6u
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
const uint8_t k_tile_bomb6[k_tile_size][k_tile_size] { // 3u
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
const uint8_t k_tile_bomb7[k_tile_size][k_tile_size] { // 0u
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
const uint8_t k_tile_bomb8[k_tile_size][k_tile_size] { // 8u
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
const uint8_t k_tile_bomb[k_tile_size][k_tile_size] {
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
const uint8_t k_tile_bomb_exploded[k_tile_size][k_tile_size] {
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
const uint8_t k_tile_unrevealed[k_tile_size][k_tile_size] {
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
const uint8_t k_tile_marked[k_tile_size][k_tile_size] {
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

template <typename func>
void loop_game_board(func&& callback) {
    constexpr auto& game_config = k_game_configs[k_current_config];

    for (int i = 0; i < game_config.size.width; i++)
        for (int j = 0; j < game_config.size.height; j++)
            callback(&g_game_board[i][j], i, j);
}

bool is_pos_valid(int x, int y) {
    constexpr auto& game_config = k_game_configs[k_current_config];
    
    if (x < 0 || y < 0)
        return false;

    return x < game_config.size.width && y < game_config.size.height;
}

bool is_in_tile_hitbox(tile_t* p_tile, int x, int y) {
    return x > p_tile->grid_x * k_tile_size && x < p_tile->grid_x * k_tile_size + k_tile_size &&
           y > p_tile->grid_y * k_tile_size && y < p_tile->grid_y * k_tile_size + k_tile_size;
}

void tile_reveal(tile_t* p_tile) {
    if (p_tile->is_revealed || p_tile->is_marked)
        return;

    p_tile->is_revealed = true;

    if (p_tile->bomb_count >= 1)
        return;

    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            if (!is_pos_valid(p_tile->grid_x + i, p_tile->grid_y + j) || (i == 0 && j == 0))
                continue;

            tile_t* p_tile_other = &g_game_board[p_tile->grid_x + i][p_tile->grid_y + j];
            if (!p_tile_other->is_bomb && !p_tile_other->is_marked)
                tile_reveal(p_tile_other);
        }
    }
}

void tile_mark(tile_t* p_tile) {
    if (p_tile->is_revealed)
        return;

    p_tile->is_marked = !p_tile->is_marked;
}

void minesweeper_on_mouse_down(const desktop_event_on_mouse_button_t* p_event) {
    int mouse_x, mouse_y;
    desktop_get_cursor_pos(&mouse_x, &mouse_y);

    switch (p_event->key) {
        case desktop_event_mouse_button_type_t::LEFT:
            loop_game_board([mouse_x, mouse_y](tile_t* p_tile, int x, int y) {
                if (is_in_tile_hitbox(p_tile, mouse_x, mouse_y))
                    tile_reveal(p_tile);
            });
            break;
        case desktop_event_mouse_button_type_t::RIGHT:
            loop_game_board([mouse_x, mouse_y](tile_t* p_tile, int x, int y) {
                if (is_in_tile_hitbox(p_tile, mouse_x, mouse_y))
                    tile_mark(p_tile);
            });
            break;
        default:
            break;
    }
}

void minesweeper_init() {
    constexpr auto& game_config = k_game_configs[k_current_config];
    
    // register desktop stuff
    desktop_event_subscribe(DESKTOP_EVENT_MOUSE_RELEASED, minesweeper_on_mouse_down);
    desktop_register_target(minesweeper_render_target);

    // random
    seed_random(1234);

    // setup game board
    // if (g_game_board)
    //     heap_free(get_global_heap(), g_game_board);
    
    // g_game_board = (tile_t**)heap_alloc(get_global_heap(), sizeof(tile_t) * game_config.size.width * game_config.size.height);
    // if (!g_game_board)
    //     return;

    // init the tiles
    loop_game_board([](tile_t* p_tile, int x, int y) {
        p_tile->grid_x = x;
        p_tile->grid_y = y;
    });

    // setup bombs
    dynamic_array<dynamic_array<int>> bomb_spots {};
    bomb_spots.resize(game_config.size.width * game_config.size.height);

    loop_game_board([&bomb_spots](tile_t* p_tile, int x, int y) { bomb_spots.insert_back({ x, y }); });

    for (int n = 0; n < game_config.bomb_count; n++) {
        int index = random_number() % bomb_spots.length();
        int x = bomb_spots[index][0];
        int y = bomb_spots[index][1];
        bomb_spots.delete_at(index);
        g_game_board[x][y].is_bomb = true;
    }

    // setup bomb counters
    loop_game_board([](tile_t* p_tile, int x, int y) {
        if (p_tile->is_bomb)
            return;

        for (int i = -1; i < 2; i++) {
            for (int j = -1; j < 2; j++) {
                if (!is_pos_valid(x + i, y + j) || (i == 0 && j == 0))
                    continue;

                tile_t& tile_other = g_game_board[x + i][y + j];
                if (tile_other.is_bomb)
                    p_tile->bomb_count++;
            }
        }
    });
}

void tile_render(tile_t* p_tile, uint64_t offset_x, uint64_t offset_y, const uint8_t p_sprite[k_tile_size][k_tile_size]) {
    for (size_t x = 0; x < k_tile_size; x++)
        for (size_t y = 0; y < k_tile_size; y++)
            desktop_render_pixel(k_tile_size * p_tile->grid_x + offset_x + x,
                k_tile_size * p_tile->grid_y + offset_y + y,
                p_sprite[y][x]);
}

void minesweeper_render_tile(tile_t* p_tile, uint64_t offset_x, uint64_t offset_y) {
    // not revealed
    if (!p_tile->is_revealed) {
        // marked
        if (p_tile->is_marked) {
            tile_render(p_tile, offset_x, offset_y, k_tile_marked);
            return;
        }

        // nothing
        tile_render(p_tile, offset_x, offset_y, k_tile_unrevealed);
        return;
    }

    // revealed

    // is bomb
    if (p_tile->is_bomb) {
        tile_render(p_tile, offset_x, offset_y, k_tile_bomb_exploded);
        return;
    }

    // is nothing
    switch (p_tile->bomb_count) {
        case 1: tile_render(p_tile, offset_x, offset_y, k_tile_bomb1); break;
        case 2: tile_render(p_tile, offset_x, offset_y, k_tile_bomb2); break;
        case 3: tile_render(p_tile, offset_x, offset_y, k_tile_bomb3); break;
        case 4: tile_render(p_tile, offset_x, offset_y, k_tile_bomb4); break;
        case 5: tile_render(p_tile, offset_x, offset_y, k_tile_bomb5); break;
        case 6: tile_render(p_tile, offset_x, offset_y, k_tile_bomb6); break;
        case 7: tile_render(p_tile, offset_x, offset_y, k_tile_bomb7); break;
        case 8: tile_render(p_tile, offset_x, offset_y, k_tile_bomb8); break;
        case 0:
        default: tile_render(p_tile, offset_x, offset_y, k_tile_empty); break;
    }
}

void minesweeper_render_target(uint64_t dt) {
    constexpr auto& game_config = k_game_configs[k_current_config];

    loop_game_board([](tile_t* p_tile, int x, int y) {
        minesweeper_render_tile(p_tile, 0, 0);
    });

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
}