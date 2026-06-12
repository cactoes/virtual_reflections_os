#include "common.hpp"
#include "std/random.hpp"
#include "vrosapi/memory.hpp"
#include "vrosapi/window.hpp"
#include "vrosapi/file.hpp"

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

    sprite_t tile_unrevealed;
    sprite_t tile_empty;
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

    for (u32 i = 0; i < config.size.width; i++)
        for (u32 j = 0; j < config.size.height; j++)
            callback(game, &game->game_board[i + config.size.width * j]);
}

void loop_neighbour_tiles(game_t* game, tile_t* tile, tile_loop_neigbour_function_t callback) {
    const game_config_t& config = game_configs[game->active_game_config];

    for (i32 i = -1; i < 2; i++) {
        for (i32 j = -1; j < 2; j++) {
            if (!is_valid_grid_pos(game, tile->x + i, tile->y + j) || (i == 0 && j == 0))
                continue;

            tile_t* tile_other = &game->game_board[(tile->y + i) + config.size.width * (tile->x + j)];
            callback(game, tile_other, tile);
        }
    }
}

tile_t* get_tile_from_pos(game_t* game, int x, int y) {
    const game_config_t& config = game_configs[game->active_game_config];

    if (x > config.size.width * tile_size ||
        y > config.size.height * tile_size ||
        x < 0 || y < 0)
        return nullptr;

    return &game->game_board[(x / tile_size) + config.size.width * (y / tile_size)];
}

tile_t* get_tile_at(game_t* game, u32 x, u32 y) {
    const game_config_t& config = game_configs[game->active_game_config];
    return &game->game_board[x + config.size.width * y];
}

void tile_reveal(game_t* game, tile_t* tile) {
    if (tile->is_revealed || tile->is_marked)
        return;

    tile->is_revealed = true;

    if (tile->is_bomb) {
        // TODO @since 11/06/2026 -- 10:24
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
    if (game->game_board == nullptr)
        return false;

    const game_config_t& config = game_configs[game->active_game_config];

    game->game_board = (tile_t*)syscall_malloc(config.size.height * config.size.width * sizeof(tile_t));
    if (game->game_board == nullptr)
        return false;

    for (u32 i = 0; i < config.size.height * config.size.width; i++) {
        (game->game_board)[i].x = i % config.size.width;
        (game->game_board)[i].y = i / config.size.height;
    }

    u32 bomb_positions_count = config.size.height * config.size.width;
    u32* bomb_positions = (u32*)syscall_malloc(bomb_positions_count * sizeof(u32) * 2);
    for (u32 i = 0; i < bomb_positions_count; i++) {
        u32 x = i % config.size.width;
        u32 y = i / config.size.height;
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

    syscall_free(bomb_positions);

    return true;
}

void minesweeper_handle_lmb_down(i32 x, i32 y) {
    // game_t* active_game = get_active_game();
    // if (!active_game)
    //     return;

    // tile_t* tile = get_tile_from_pos(active_game, x, y);
    // if (!tile)
    //     return;

    // tile->has_clicked = true;
    // tile_reveal(active_game, tile);
}

void event_hook(window_handle_t handle, window_event_t event) {
    switch (event.type) {
        case WE_MBL_DOWN:
            minesweeper_handle_lmb_down(event.mouse.x, event.mouse.y);
            break;
        case WE_MBL_UP:
            break;
        default:
            break;
    }
}

struct bmp_file_header_t {
    u8 signature[2];
    u32 file_size;
    u16 unused[2];
    u32 image_data_offset;
} PACKED;

struct bmp_info_header_t {
    u32 header_size;
    int width;
    int height;
    u16 planes;
    u16 bits_per_pixel;
    u32 compression;
    u32 uncompressed_size;
    int pixels_per_m_x;
    int pixels_per_m_y;
    u32 number_of_colors;
    u32 number_of_importand_colors;
} PACKED;

struct bmp_color_t {
    u8 r;
    u8 g;
    u8 b;
} PACKED;

struct bmp_image_t {
    u8* file_data;
    u64 file_size;

    i32 width;
    i32 height;
    bool bottom_up;

    bmp_color_t colors[256];
};

bool is_bmp(u8* data, size_t size) {
    if (!data)
        return false;

    if (size < sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t) + sizeof(bmp_color_t))
        return false;

    return data[0] == 'B' && data[1] == 'M';
}

bool bmp_load_image(u8* file_data, u64 file_size, bmp_image_t* image) {
    if (!file_data || file_size == 0 || !image)
        return false;

    if (!is_bmp(file_data, file_size))
        return false;

    image->file_data = file_data;
    image->file_size = file_size;

    bmp_file_header_t* file_header = (bmp_file_header_t*)file_data;
    bmp_info_header_t* info_header = (bmp_info_header_t*)(file_data + sizeof(bmp_file_header_t));

    if (info_header->bits_per_pixel != 8)
        return false;

    u8* color_pallet = (u8*)(file_data + sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t));

    for (int i = 0; i < info_header->number_of_colors; i++) {
        image->colors[i].b = *(color_pallet + (i * 4) + 0);
        image->colors[i].g = *(color_pallet + (i * 4) + 1);
        image->colors[i].r = *(color_pallet + (i * 4) + 2);
    }

    image->width = info_header->width;
    image->height = info_header->height;
    image->bottom_up = true;

    if (image->height < 0) {
        image->height = -image->height;
        image->bottom_up = false;
    }

    return true;
}

bool bmp_load_sprite(bmp_image_t* image, sprite_t* sprite, u32 tile_size, i32 src_x, i32 src_y) {
    if (!image || !sprite)
        return false;

    sprite->sprite_size = tile_size;
    sprite->data = (u32*)syscall_malloc(tile_size * tile_size * sizeof(u32));

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

    for (int y = 0; y < tile_size; y++) {
        u32* target = &((u32*)buffer)[(dst_x + window_width * (dst_y + y))];
        u32* src = &sprite->data[(0 + tile_size * y)];
        memcpy(target, src, tile_size * sizeof(u32));
    }

    return true;
}

void render_gameboard(game_t* game) {
    void* buffer = syscall_get_window_buffer(game->window_handle);
    memzero(buffer, (window_width * window_height) * sizeof(u32));

    const game_config_t& config = game_configs[game->active_game_config];

    tile_t* tile = game->game_board;

    u64 file_handle = syscall_open_file("harddisk0/minesweeper/sprites.bmp");
    
    u8* data = nullptr;
    u64 size = 0;
    bool result = syscall_read_file(file_handle, &data, &size);

    bmp_image_t spritesheet {};
    bmp_load_image(data, size, &spritesheet);

    bmp_load_sprite(&spritesheet, &game->tile_unrevealed, tile_size, 0, 51);
    bmp_load_sprite(&spritesheet, &game->tile_empty, tile_size, 17, 51);

    for (u32 x = 0; x < config.size.width; x++) {
        for (u32 y = 0; y < config.size.height; y++) {
            sprite_render(buffer, &game->tile_unrevealed, x * tile_size, y * tile_size);
        }
    }

    syscall_render_window(game->window_handle);
}

int main() {
    window_desc_t wnd_desc {};
    memzero(&wnd_desc, sizeof(window_desc_t));
    wnd_desc.rect.h = window_height;
    wnd_desc.rect.w = window_width;
    wnd_desc.event_hook = event_hook;

    u64 handle = syscall_create_window(&wnd_desc);
    void* buffer = syscall_get_window_buffer(handle);
    memzero(buffer, (window_width * window_height) * sizeof(u32));

    syscall_render_window(handle);

    game_t* active_game = get_active_game();

    active_game->active_game_config = 0;
    active_game->game_board = nullptr;
    active_game->window_handle = handle;

    game_board_create(active_game);

    render_gameboard(active_game);

    while (true) {
        window_event_t event {};
        while (syscall_poll_event(handle, &event, nullptr))
            event_hook(handle, event);
    }

    return 0;
}