#include "gui/desktop.hpp"
#include "drivers/vga.hpp"
#include "drivers/ps2/mouse.hpp"
#include "std/array.hpp"
#include "std/map.hpp"
#include "utils/event.hpp"
#include "std/random.hpp"
#include "time/clock.hpp"
#include "virtual_thread.hpp"
#include "gui/programs/minesweeper.hpp"
#include "gui/programs/webbrowser.hpp"
#include "gui/font8x8.hpp"
#include "filesystems/vfs.hpp"
#include "interrupt_manager.hpp"

static vga_buffer_t* g_desktop_back_buffer = nullptr;
static bool g_desktop_ready = false;
static int g_desktop_mouse_pos[2] { 0, 0 };
static std::linear_map<u64, event_manager_t<void*>> g_desktop_event_container {};
static linked_list<desktop_render_target_t> g_render_targets {};
constexpr u64 k_desktop_target_fps_ms = 1000 / 60;

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
        g_desktop_event_container[hash_fnv1a_64(DESKTOP_EVENT_MOUSE_MOVE)].fire_event((void*)&event);
    }
    
    // on mouse button press & release
    if (static bool s_lmb_last = false; s_lmb_last != p_state->buttons.left) {
        // call event
        desktop_event_on_mouse_button_t event {};
        event.key = desktop_event_mouse_button_type_t::LEFT;

        if (p_state->buttons.left)
            g_desktop_event_container[hash_fnv1a_64(DESKTOP_EVENT_MOUSE_PRESSED)].fire_event((void*)&event);
        else
            g_desktop_event_container[hash_fnv1a_64(DESKTOP_EVENT_MOUSE_RELEASED)].fire_event((void*)&event);

        s_lmb_last = p_state->buttons.left;
    }

    if (static bool s_rmb_last = false; s_rmb_last != p_state->buttons.right) {
        // call event
        desktop_event_on_mouse_button_t event {};
        event.key = desktop_event_mouse_button_type_t::RIGHT;

        if (p_state->buttons.right)
            g_desktop_event_container[hash_fnv1a_64(DESKTOP_EVENT_MOUSE_PRESSED)].fire_event((void*)&event);
        else
            g_desktop_event_container[hash_fnv1a_64(DESKTOP_EVENT_MOUSE_RELEASED)].fire_event((void*)&event);

        s_rmb_last = p_state->buttons.right;
    }

    if (static bool s_mmb_last = false; s_mmb_last != p_state->buttons.middle) {
        // call event
        desktop_event_on_mouse_button_t event {};
        event.key = desktop_event_mouse_button_type_t::RIGHT;

        if (p_state->buttons.middle)
            g_desktop_event_container[hash_fnv1a_64(DESKTOP_EVENT_MOUSE_PRESSED)].fire_event((void*)&event);
        else
            g_desktop_event_container[hash_fnv1a_64(DESKTOP_EVENT_MOUSE_RELEASED)].fire_event((void*)&event);

        s_mmb_last = p_state->buttons.middle;
    }

    // on scroll
    if (p_state->ds != 0) {
        // call event
        desktop_event_on_mouse_scroll_t event {};
        event.d = p_state->ds;
        g_desktop_event_container[hash_fnv1a_64(DESKTOP_EVENT_MOUSE_SCROLL)].fire_event((void*)&event);
    }
}

bool desktop_event_subscribe(const char* p_name, void(*p_callback)(void*)) {
    const u64 hash = hash_fnv1a_64(p_name);

    auto it = g_desktop_event_container.get(hash);
    if (it == g_desktop_event_container.end())
        return false;

    g_desktop_event_container[hash].subscribe(p_callback);
    return true;
}

void desktop_init_hardware_handlers() {
    // setup events
    g_desktop_event_container.insert(hash_fnv1a_64(DESKTOP_EVENT_MOUSE_MOVE), event_manager_t<void*>{});
    g_desktop_event_container.insert(hash_fnv1a_64(DESKTOP_EVENT_MOUSE_SCROLL), event_manager_t<void*>{});
    g_desktop_event_container.insert(hash_fnv1a_64(DESKTOP_EVENT_MOUSE_PRESSED), event_manager_t<void*>{});
    g_desktop_event_container.insert(hash_fnv1a_64(DESKTOP_EVENT_MOUSE_RELEASED), event_manager_t<void*>{});

    // start with mouse in the middle
    g_desktop_mouse_pos[0] = VGA_GM_BUFFER_WIDTH / 2;
    g_desktop_mouse_pos[1] = VGA_GM_BUFFER_HEIGHT / 2;

    // add ps2 mouse input support
    ps2_mouse_event_subscribe(desktop_handle_ps2_mouse_input);
}

void desktop_render_clear_buffer() {
    if (get_global_graphics_driver()->type == graphics_driver_type_t::FRAMEBUFFER) {
        auto fb = get_global_graphics_driver()->framebuffer;
    
        memset(fb->back_buffer, 0, fb->size);
    } else {
        auto fb = get_global_graphics_driver()->vgabuffer;
    
        memset(fb->buffer, 0, fb->size.width * fb->size.height);
    }
}

void desktop_render_init() {
    desktop_render_clear_buffer();
}

void desktop_render_end() {
    graphics_driver_render(get_global_graphics_driver());
}

bool desktop_render_pixel(int x, int y, const color_t& color) {
    graphics_driver_draw_pixel_raw(get_global_graphics_driver(), x, y, color);
    return true;
}

bool desktop_render_linev(int x, int y, size_t l, const color_t& color) {
    return graphics_driver_draw_linev(get_global_graphics_driver(), x, y, l, color);
}

bool desktop_render_lineh(int x, int y, size_t l,const color_t& color) {
    return graphics_driver_draw_lineh(get_global_graphics_driver(), x, y, l, color);
}

bool desktop_render_square(int x, int y, size_t w, size_t h, const color_t& color) {
    return graphics_driver_draw_square(get_global_graphics_driver(), x, y, w, h, color);
}

bool desktop_render_char(int x, int y, char ch, const color_t& color) {
    if (ch < 0 || ch >= 128)
        ch = 128;
    
    for (int row = 0; row < 8; row++) {
        u8 line = font8x8[(int)ch][row];
        for (int col = 0; col < 8; col++) {
            if (line & (128 >> col))
                desktop_render_pixel(x + col, y + row, color);
        }
    }

    return true;
}

bool desktop_render_text(int x, int y, const char* str, const color_t& color) {
    if (str == nullptr || !*str)
        return false;

    int offset = 0;
    while (*str) {
        desktop_render_char(x + offset, y, *str, color);
        offset += 8;
        str++;
    }

    return true;
}

void desktop_render_draw_cursor() {
    const u64 x = g_desktop_mouse_pos[0];
    const u64 y = g_desktop_mouse_pos[1];

    // outline
    desktop_render_linev(x, y, 10, { 0, 0, 0 });
    desktop_render_pixel(x + 1, y + 1, { 0, 0, 0 });
    desktop_render_pixel(x + 2, y + 2, { 0, 0, 0 });
    desktop_render_pixel(x + 3, y + 3, { 0, 0, 0 });
    desktop_render_pixel(x + 4, y + 4, { 0, 0, 0 });
    desktop_render_pixel(x + 5, y + 5, { 0, 0, 0 });
    desktop_render_pixel(x + 6, y + 6, { 0, 0, 0 });
    desktop_render_lineh(x + 4, y + 7, 3, { 0, 0, 0 });
    desktop_render_pixel(x + 1, y + 9, { 0, 0, 0 });
    desktop_render_pixel(x + 2, y + 8, { 0, 0, 0 });
    desktop_render_pixel(x + 3, y + 7, { 0, 0, 0 });

    // inline
    desktop_render_linev(x + 1, y + 2, 7, { 255, 255, 255 });
    desktop_render_linev(x + 2, y + 3, 5, { 255, 255, 255 });
    desktop_render_linev(x + 3, y + 4, 3, { 255, 255, 255 });
    desktop_render_linev(x + 4, y + 5, 2, { 255, 255, 255 });
    desktop_render_linev(x + 5, y + 6, 1, { 255, 255, 255 });
}

void desktop_render_task_bar() {
    size_t w, h;
    graphics_driver_get_size(get_global_graphics_driver(), &w, &h);
    desktop_render_square(0, h - 20, w, 20, { 50, 50, 50 });
}

bool desktop_register_target(desktop_render_target_t p_target) {
    return g_render_targets.insert_back(p_target);
}

void desktop_on_mouse_release(const desktop_event_on_mouse_button_t* event) {
    if (event->key != desktop_event_mouse_button_type_t::LEFT)
        return;

    const auto x = g_desktop_mouse_pos[0];
    const auto y = g_desktop_mouse_pos[1];

    for (auto& target : g_render_targets)
        target.dragging = false;
}

void desktop_on_mouse_pressed(const desktop_event_on_mouse_button_t* event) {
    if (event->key != desktop_event_mouse_button_type_t::LEFT)
        return;

    const auto x = g_desktop_mouse_pos[0];
    const auto y = g_desktop_mouse_pos[1];

    for (auto& target : g_render_targets) {
        if (x > target.x && x < target.x + target.w &&
            y > target.y && y < target.y + 10) {
                target.dragging = true;
                break;
            }

        target.dragging = false;
    }
}

void desktop_on_mouse_move(const desktop_event_on_mouse_move_t* event) {
    static int x, y;

    const int dx = event->x - x;
    const int dy = event->y - y;

    for (auto& target : g_render_targets) {
        if (target.dragging) {
            target.x += dx;
            target.y += dy;
            break;
        }
    }

    x = event->x;
    y = event->y;
}

void desktop_render_window(const desktop_render_target_t* target) {
    desktop_render_square(target->x - 1, target->y, target->w + 2, 10, { 255, 255, 255 });
    desktop_render_linev(target->x - 1, target->y + 10, target->h, { 255, 255, 255 });
    desktop_render_lineh(target->x - 1, target->y + 10 + target->h, target->w + 2, { 255, 255, 255 });
    desktop_render_linev(target->x + target->w, target->y + 10, target->h, { 255, 255, 255 });

    desktop_render_text(target->x + 1, target->y + 2, target->name.c_str(), { 0, 0, 0 });
}

#include "io.hpp"
#include "arch/generic.hpp"

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

bool is_bmp(u8* data, size_t size) {
    if (!data)
        return false;

    if (size < sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t) + sizeof(bmp_color_t))
        return false;

    return data[0] == 'B' && data[1] == 'M';
}

void load_background() {
    fd_t fd = vfs_open_file(get_global_vfs(), "harddisk0/media/logo.bmp");
    if (fd == FILE_DESCRIPTOR_INVALID)
        return kprintf("failed to open logo.bmp\n");

    u8* data;
    size_t size;
    if (!vfs_read_file(get_global_vfs(), fd, &data, &size))
        return kprintf("failed to read logo.bmp");
    
    if (!is_bmp(data, size))
        return kprintf("not bmp!\n");

    bmp_file_header_t* file_header = (bmp_file_header_t*)data;
    bmp_info_header_t* info_header = (bmp_info_header_t*)(data + sizeof(bmp_file_header_t));

    if (info_header->bits_per_pixel != 8)
        return kprintf("not 8-bit image\n");

    u8* color_pallet = (u8*)(data + sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t));

    bmp_color_t colors[256] {};
    for (int i = 0; i < info_header->number_of_colors; i++) {
        colors[i].b = *(color_pallet + (i * 4) + 0);
        colors[i].g = *(color_pallet + (i * 4) + 1);
        colors[i].r = *(color_pallet + (i * 4) + 2);
    }

    u8* image_data = (u8*)(data + file_header->image_data_offset);

    int width  = info_header->width;
    int height = info_header->height;
    bool bottom_up = true;

    if (height < 0) {
        height = -height;
        bottom_up = false;
    }

    /* Each row is padded to 4 bytes */
    int row_stride = (width + 3) & ~3;

    for (int y = 0; y < height; y++) {
        int bmp_y = bottom_up ? (height - 1 - y) : y;

        for (int x = 0; x < width; x++) {
            u8 index = image_data[bmp_y * row_stride + x];
            bmp_color_t c = colors[index];
            desktop_render_pixel(
                x,
                y,
                { .r = c.r, .g = c.g, .b = c.b }
            );
        }
    }

    // free(data);
    // vfs_close_file(get_global_vfs(), fd);
}

int desktop_init() {
    // setup renderer
    desktop_render_init();

    // setup input devices
    desktop_init_hardware_handlers();

    desktop_event_subscribe(DESKTOP_EVENT_MOUSE_RELEASED, desktop_on_mouse_release);
    desktop_event_subscribe(DESKTOP_EVENT_MOUSE_PRESSED, desktop_on_mouse_pressed);
    desktop_event_subscribe(DESKTOP_EVENT_MOUSE_MOVE, desktop_on_mouse_move);

    minesweeper_init();
    // webbrowser_init();

    // load_background();

    // desktop_render_end();

    // while (1)
    // {
    // }

    g_desktop_ready = true;

    // main render loop
    u64 g_last_tick = 0;
    while (true) {
        // wait for new frame
        const u64 now = clock_get_time_since_boot();
        const u64 dt = now - g_last_tick;

        if (dt < k_desktop_target_fps_ms) {
            vthread_sleep(k_desktop_target_fps_ms - (dt));
            continue;
        }

        disable_interrupts();

        desktop_render_clear_buffer();

        // render targets
        for (size_t i = 0; i < g_render_targets.length(); i++) {
            auto& target = g_render_targets[i];
            target.callback(dt, target.x, target.y + 10);
            desktop_render_window(&target);
        }

        // render ui
        desktop_render_task_bar();
        desktop_render_draw_cursor();

        desktop_render_end();

        enable_interrupts();

        g_last_tick = now;
    }

    return 0;
}

void desktop_get_cursor_pos(int* p_x, int* p_y) {
    *p_x = g_desktop_mouse_pos[0];
    *p_y = g_desktop_mouse_pos[1];
}

bool is_desktop_ready() {
    return g_desktop_ready;
}