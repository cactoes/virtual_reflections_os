#include "gui/desktop.hpp"
#include "drivers/vga.hpp"
#include "drivers/ps2/mouse.hpp"
#include "std/array.hpp"
#include "std/map.hpp"
#include "utils/event.hpp"
#include "std/random.hpp"
#include "time/clock.hpp"
#include "virtual_thread.hpp"
#include "gui/games/minesweeper.hpp"

static vga_buffer_t* g_desktop_back_buffer = nullptr;
static bool g_desktop_ready = false;
static int g_desktop_mouse_pos[2] { 0, 0 };
static std::linear_map<uint64_t, event_manager_t<void*>> g_desktop_event_container {};
static linked_list<desktop_render_target_t> g_render_targets {};
constexpr uint64_t k_desktop_target_fps_ms = 1000 / 60;

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
    const uint64_t hash = hash_fnv1a_64(p_name);

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

vga_gm_color_index_t rgb_to_vga(const desktop_render_color_t& c) {
    // *taken from the internet

    static constexpr uint8_t s_palette[16][3] = {
        {0x00, 0x00, 0x00}, {0x00, 0x00, 0xAA}, {0x00, 0xAA, 0x00}, {0x00, 0xAA, 0xAA},
        {0xAA, 0x00, 0x00}, {0xAA, 0x00, 0xAA}, {0xAA, 0x55, 0x00}, {0xAA, 0xAA, 0xAA},
        {0x55, 0x55, 0x55}, {0x55, 0x55, 0xFF}, {0x55, 0xFF, 0x55}, {0x55, 0xFF, 0xFF},
        {0xFF, 0x55, 0x55}, {0xFF, 0x55, 0xFF}, {0xFF, 0xFF, 0x55}, {0xFF, 0xFF, 0xFF}
    };

    int best_dist = MAX_INT32;
    vga_gm_color_index_t best = vga_gm_color_index_t::BLACK;

    for (int i = 0; i < 16; ++i) {
        int dr = int(c.r) - int(s_palette[i][0]);
        int dg = int(c.g) - int(s_palette[i][1]);
        int db = int(c.b) - int(s_palette[i][2]);
        int dist = dr*dr + dg*dg + db*db;

        if (dist < best_dist) {
            best_dist = dist;
            best = (vga_gm_color_index_t)i;
            if (dist == 0)
                break;
        }
    }
    return best;
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

bool desktop_render_pixel(int x, int y, uint8_t vga_color_index) {
    return vga_gm_draw::pixel(desktop_render_get_buffer(), x, y, (vga_gm_color_index_t)vga_color_index);
}

bool desktop_render_pixel(int x, int y, const desktop_render_color_t& color) {
    return desktop_render_pixel(x, y, (uint8_t)rgb_to_vga(color));
}

bool desktop_render_linev(int x, int y, size_t l, const desktop_render_color_t& color) {
    return vga_gm_draw::linev(desktop_render_get_buffer(), x, y, l, rgb_to_vga(color));
}

bool desktop_render_lineh(int x, int y, size_t l,const desktop_render_color_t& color) {
    return vga_gm_draw::lineh(desktop_render_get_buffer(), x, y, l, rgb_to_vga(color));
}

bool desktop_render_square(int x, int y, size_t w, size_t h, const desktop_render_color_t& color) {
    return vga_gm_draw::square(desktop_render_get_buffer(), x, y, w, h, rgb_to_vga(color));
}

void desktop_render_draw_cursor() {
    const uint64_t x = g_desktop_mouse_pos[0];
    const uint64_t y = g_desktop_mouse_pos[1];

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
    desktop_render_square(0, VGA_GM_BUFFER_HEIGHT - 20, VGA_GM_BUFFER_WIDTH, 20, { 50, 50, 50 });
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

    g_desktop_ready = true;

    // main render loop
    uint64_t g_last_tick = 0;
    while (true) {
        // wait for new frame
        const uint64_t now = clock_get_time_since_boot();
        const uint64_t dt = now - g_last_tick;

        if (dt < k_desktop_target_fps_ms) {
            vthread_sleep(k_desktop_target_fps_ms - (dt));
            continue;
        }

        desktop_render_clear_buffer();

        // render targets
        for (auto& target : g_render_targets) {
            target.callback(dt, target.x, target.y + 10);
            desktop_render_window(&target);
        }

        // render ui
        desktop_render_task_bar();
        desktop_render_draw_cursor();

        desktop_render_end();
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