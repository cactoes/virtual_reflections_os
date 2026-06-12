#include "common.hpp"
#include "std/random.hpp"
#include "vrosapi/memory.hpp"
#include "vrosapi/window.hpp"
#include "vrosapi/file.hpp"
#include "vrosapi/time.hpp"
#include "bmp.hpp"

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

static u64 window_width = 400;
static u64 window_height = 400;
static const u32 tile_size = 16;

static game_t game {};

game_t* get_active_game() {
    return &game;
}

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

void minesweeper_end_game(game_t* game) {
    game->is_running = false;

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
        minesweeper_end_game(game);
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

void render_gameboard(game_t* game);

void minesweeper_handle_lmb_down(i32 x, i32 y) {
    game_t* active_game = get_active_game();
    if (!active_game)
        return;

    if (!active_game->is_running)
        return;

    const game_config_t& config = game_configs[active_game->active_game_config];

    tile_t* tile = get_tile_from_pos(active_game, x - active_game->board_offsetx, y - active_game->board_offsety);
    if (!tile)
        return;

    if (tile->is_revealed)
        tile_peek_nearby(active_game, tile);
    else
        tile->is_peeking = true;

    render_gameboard(active_game);
}

void minesweeper_handle_lmb_up(i32 x, i32 y) {
    game_t* active_game = get_active_game();
    if (!active_game)
        return;

    if (y < active_game->board_offsety) {
        if (x >= active_game->smileyx && x < active_game->smileyx + active_game->smiley_happy.sprite_size &&
            y >= active_game->smileyy && y < active_game->smileyy + active_game->smiley_happy.sprite_size) {

            free(active_game->game_board);

            active_game->game_board = nullptr;

            game_board_create(active_game);

            active_game->is_running = true;

            render_gameboard(active_game);
        }

        return;
    }

    if (!active_game->is_running)
        return;

    const game_config_t& config = game_configs[active_game->active_game_config];

    tile_t* tile = get_tile_from_pos(active_game, x - active_game->board_offsetx, y - active_game->board_offsety);
    if (!tile)
        return;

    tile->has_clicked = true;
    tile_reveal(active_game, tile);

    int total_unrevealed = 0;
    for (u32 i = 0; i < config.size.width * config.size.height; i++)
        if (!active_game->game_board[i].is_revealed)
            total_unrevealed++;

    if (total_unrevealed == config.bomb_count)
        minesweeper_end_game(active_game);

    // remove peaking
    loop_game_board(active_game, [](game_t* game, tile_t* tile) {
        if (tile->is_bomb)
            return;

        loop_neighbour_tiles(game, tile, [](game_t* game, tile_t* current_tile, tile_t* origin_tile) {
            if (current_tile->is_peeking)
                current_tile->is_peeking = false;
        });
    });

    render_gameboard(active_game);
}

void minesweeper_handle_rmb_up(i32 x, i32 y) {
    game_t* active_game = get_active_game();
    if (!active_game)
        return;

    if (!active_game->is_running)
        return;

    const game_config_t& config = game_configs[active_game->active_game_config];

    tile_t* tile = get_tile_from_pos(active_game, x - active_game->board_offsetx, y - active_game->board_offsety);
    if (!tile)
        return;

    tile_mark(tile);

    render_gameboard(active_game);
}

void event_hook(window_handle_t handle, window_event_t event) {
    switch (event.type) {
        case WE_MBL_DOWN:
            minesweeper_handle_lmb_down(event.mouse.x, event.mouse.y);
            break;
        case WE_MBL_UP:
            minesweeper_handle_lmb_up(event.mouse.x, event.mouse.y);
            break;
        case WE_MBR_UP:
            minesweeper_handle_rmb_up(event.mouse.x, event.mouse.y);
            break;
        default:
            break;
    }
}

bool bmp_load_sprite(bmp_image_t* image, sprite_t* sprite, u32 tile_size, i32 src_x, i32 src_y) {
    if (!image || !sprite)
        return false;

    sprite->sprite_size = tile_size;
    sprite->data = (u32*)malloc(tile_size * tile_size * sizeof(u32));

    bmp_file_header_t* file_header = (bmp_file_header_t*)image->file_data;
    u8* image_data = (u8*)(image->file_data + file_header->image_data_offset);

    int row_stride = (image->width + 3) & ~3;

    for (int y = 0; y < tile_size; y++) {
        int bmp_y = image->bottom_up ? (image->height - 1 - (y + src_y)) : (y + src_y);

        for (int x = 0; x < tile_size; x++) {
            u8 index = image_data[bmp_y * row_stride + (x + src_x)];
            bmp_color_t c = image->colors[index];

            u8* pixel = (u8*)sprite->data + (x + tile_size * y) * 4;
            pixel[0] = c.b;
            pixel[1] = c.g;
            pixel[2] = c.r;
            pixel[3] = 0xFF;
        }
    }

    return true;
}

bool sprite_render(void* buffer, sprite_t* sprite, i32 dst_x, i32 dst_y) {
    if (!buffer || !sprite)
        return false;

    for (int y = 0; y < sprite->sprite_size; y++) {
        u32* target = &((u32*)buffer)[(dst_x + window_width * (dst_y + y))];
        u32* src = &sprite->data[(0 + sprite->sprite_size * y)];
        memcpy(target, src, sprite->sprite_size * sizeof(u32));
    }

    return true;
}

sprite_t* tile_get_sprite(game_t* game, tile_t* tile) {
    if (!tile || !game)
        return nullptr;

    if (tile->is_peeking)
        return &game->tile_empty;

    // not revealed
    if (!tile->is_revealed) {
        // wrong marked eog
        if (!game->is_running && tile->is_marked && !tile->is_bomb)
            return &game->tile_marked_wrong;

        // marked
        if (tile->is_marked)
            return &game->tile_marked;

        // nothing
        return &game->tile_unrevealed;
    }

    // revealed

    // is bomb
    if (tile->is_bomb)
        return tile->has_clicked ? &game->tile_bomb_exploded : &game->tile_bomb;

    // is nothing
    switch (tile->bomb_count) {
        case 1: return &game->tile_bomb1;
        case 2: return &game->tile_bomb2;
        case 3: return &game->tile_bomb3;
        case 4: return &game->tile_bomb4;
        case 5: return &game->tile_bomb5;
        case 6: return &game->tile_bomb6;
        case 7: return &game->tile_bomb7;
        case 8: return &game->tile_bomb8;
        case 0:
        default: return &game->tile_empty;
    }

    return nullptr;
}

bool render_box(game_t* game, void* buffer, u32 x, u32 y, u32 w, u32 h, u32 color) {
    const game_config_t& config = game_configs[game->active_game_config];
    if (x + w > window_width || y + h > window_height)
        return false;

    u32* row = (u32*)buffer + y * window_width + x;

    for (u32 j = 0; j < h; j++) {
        for (u32 i = 0; i < w; i++)
            row[i] = color;

        row += window_width;
    }

    return true;
}

void render_gameboard(game_t* game) {
    void* buffer = syscall_get_window_buffer(game->window_handle);
    memzero(buffer, (window_width * window_height) * sizeof(u32));

    const game_config_t& config = game_configs[game->active_game_config];

    render_box(game, buffer, 0, 0, tile_size * config.size.width, 30, 0xFFC0C0C0);

    for (u32 x = 0; x < config.size.width; x++) {
        for (u32 y = 0; y < config.size.height; y++) {
            sprite_t* sprite = tile_get_sprite(game, get_tile_at(game, x, y));
            sprite_render(buffer, sprite, x * tile_size + game->board_offsetx, y * tile_size + game->board_offsety);
        }
    }

    sprite_render(buffer, &game->smiley_happy, game->smileyx, game->smileyy);

    syscall_render_window(game->window_handle);
}

int main() {
    window_width = tile_size * game_configs[0].size.width;
    window_height = tile_size * game_configs[0].size.height + 30;

    seed_random(syscall_time_since_boot());

    window_desc_t wnd_desc {};
    memzero(&wnd_desc, sizeof(window_desc_t));
    wnd_desc.rect.h = window_height;
    wnd_desc.rect.w = window_width;
    wnd_desc.event_hook = event_hook;

    u64 handle = syscall_create_window(&wnd_desc);
    void* buffer = syscall_get_window_buffer(handle);
    memzero(buffer, (window_width * window_height) * sizeof(u32));

    game_t* active_game = get_active_game();
    memzero(active_game, sizeof(game_t));

    active_game->active_game_config = 0;
    active_game->game_board = nullptr;
    active_game->window_handle = handle;

    active_game->board_offsetx = 0;
    active_game->board_offsety = 30;

    game_board_create(active_game);

    u64 file_handle = syscall_open_file("harddisk0/minesweeper/sprites.bmp");
    
    u8* data = nullptr;
    u64 size = 0;
    bool result = syscall_read_file(file_handle, &data, &size);

    bmp_image_t spritesheet {};
    bmp_load_image(data, size, &spritesheet);

    bmp_load_sprite(&spritesheet, &active_game->tile_bomb1, tile_size, 0, 68);
    bmp_load_sprite(&spritesheet, &active_game->tile_bomb2, tile_size, 17, 68);
    bmp_load_sprite(&spritesheet, &active_game->tile_bomb3, tile_size, 34, 68);
    bmp_load_sprite(&spritesheet, &active_game->tile_bomb4, tile_size, 51, 68);
    bmp_load_sprite(&spritesheet, &active_game->tile_bomb5, tile_size, 68, 68);
    bmp_load_sprite(&spritesheet, &active_game->tile_bomb6, tile_size, 85, 68);
    bmp_load_sprite(&spritesheet, &active_game->tile_bomb7, tile_size, 102, 68);
    bmp_load_sprite(&spritesheet, &active_game->tile_bomb8, tile_size, 119, 68);
    bmp_load_sprite(&spritesheet, &active_game->tile_bomb, tile_size, 85, 51);
    bmp_load_sprite(&spritesheet, &active_game->tile_bomb_exploded, tile_size, 102, 51);
    bmp_load_sprite(&spritesheet, &active_game->tile_marked, tile_size, 34, 51);
    bmp_load_sprite(&spritesheet, &active_game->tile_marked_wrong, tile_size, 51, 51);
    bmp_load_sprite(&spritesheet, &active_game->tile_unrevealed, tile_size, 0, 51);
    bmp_load_sprite(&spritesheet, &active_game->tile_empty, tile_size, 17, 51);
    bmp_load_sprite(&spritesheet, &active_game->smiley_happy, 26, 0, 24);
    bmp_load_sprite(&spritesheet, &active_game->smiley_happy_pressed, 26, 27, 24);
    bmp_load_sprite(&spritesheet, &active_game->smiley_dead, 26, 81, 24);
    bmp_load_sprite(&spritesheet, &active_game->smiley_cool, 26, 108, 24);

    const game_config_t& config = game_configs[active_game->active_game_config];
    active_game->smileyx = ((config.size.width * tile_size) / 2) - (active_game->smiley_happy.sprite_size / 2);
    active_game->smileyy = 15 - (active_game->smiley_happy.sprite_size / 2);

    active_game->is_running = true;

    render_gameboard(active_game);

    while (true) {
        window_event_t event {};
        while (syscall_poll_event(handle, &event, nullptr))
            event_hook(handle, event);
    }

    return 0;
}