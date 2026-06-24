#include "gui/display_driver.hpp"
#include "drivers/graphics/graphics_driver.hpp"
#include "interrupt_manager.hpp"
#include "time/clock.hpp"
#include "virtual_thread.hpp"
#include "memory/vmem.hpp"

#include "arch/amd64/vmem.hpp"

static void* dd_remote_buffer = nullptr;
static u64 dd_target_fps_ms = 1000 / 60;

static window_handle_t last_handle = 0;
static window_manager_t* global_window_manager = nullptr;

void dd_set_active_buffer(void* buffer) {
    dd_remote_buffer = buffer;
}

int dd_buffer_render_loop() {
    graphics_driver_t* gd = get_global_graphics_driver();
    if (!gd)
        return 1;

    u64 last_tick = 0;
    while (true) {
        const u64 now = clock_get_time_since_boot();
        const u64 dt = now - last_tick;

        if (dt < dd_target_fps_ms) {
            vthread_sleep(dd_target_fps_ms - (dt));
            continue;
        }

        disable_interrupts();

        graphics_driver_render(gd);

        enable_interrupts();
    }

    return 0;
}

bool wm_init(window_manager_t* wm) {
    if (!wm)
        return false;

    wm->should_render = true;

    return true;
}

void set_global_window_manager(window_manager_t* wm) {
    global_window_manager = wm;
}

window_manager_t* get_global_window_manager() {
    return global_window_manager;
}

window_t* wm_create_window(int w, int h, int x, int y) {
    // TODO @since 25/06/2026 -- 01:00
    // update w / h types everywhere

    window_t* win = new window_t {};
    win->width = w;
    win->height = h;
    win->x = x;
    win->y = y;
    win->handle = last_handle++;
    return win;
}

window_handle_t wm_allocate_window(window_manager_t* wm, int w, int h, event_hook_t hook) {
    auto win = wm_create_window(w, h, 20, 20);
    win->parent_process = get_current_process();
    win->buffer = heap_alloc(&win->parent_process->heap, (w * h) * sizeof(u32));
    win->event_hook = hook;
    wm->windows.insert_back(win);
    return win->handle;
}

void* wm_window_get_buffer(window_manager_t* wm, window_handle_t handle) {
    for (auto& w : wm->windows)
        if (w->handle == handle)
            return w->buffer;

    return nullptr;
}

bool wm_window_poll_event(window_manager_t* wm, window_handle_t handle, window_event_t* event, event_hook_t* hook) {
    for (auto& w : wm->windows) {
        if (w->handle == handle) {
            if (hook)
                *hook = w->event_hook;

            return w->event_queue.get(*event);
        }
    }

    return false;
}

bool wm_window_resize(window_manager_t* wm, window_handle_t handle, u32 width, u32 height) {
    size_t x, y;
    graphics_driver_get_size(get_global_graphics_driver(), &x, &y);

    if (width > x || height > y)
        return false;

    for (auto& w : wm->windows) {
        if (w->handle == handle) {

            heap_free(&w->parent_process->heap, w->buffer);
            w->buffer = heap_alloc(&w->parent_process->heap, (width * height) * sizeof(u32));
            if (!w->buffer)
                return false;
            w->width = width;
            w->height = height;

            return true;
        }
    }

    return false;
}

void wm_handle_mouse_event(window_manager_t* wm, int x, int y, kdc_action_t action) {
    size_t max_x, max_y;
    graphics_driver_get_size(get_global_graphics_driver(), &max_x, &max_y);

    switch (action) {
        case kdc_action_t::MMOVE: {
            if (x == 0 && y == 0)
                break;

            for (auto& window : wm->windows) {
                if (window->is_dragging) {
                    window->x += x;
                    window->y += y;
                    break;
                }
            }

            wm->cursor.x += x;
            wm->cursor.y += y;
            wm->should_render = true;
            break;
        }
        case kdc_action_t::MDOWNL: {
            for (auto& window : wm->windows) {
                if (wm->cursor.x > window->x && wm->cursor.x < window->x + window->width &&
                    wm->cursor.y > window->y - 15 && wm->cursor.y < window->y + window->height) {
                    window->is_dragging = true;
                    window->active = true;
                    window->event_queue.insert(window_event_t { .type = WE_MBL_DOWN, .mouse = { .x = (i32)(wm->cursor.x - window->x), .y = (i32)(wm->cursor.y - window->y) } });
                    break;
                }

                window->active = false;
            }
            break;
        }
        case kdc_action_t::MUPL: {
            for (auto& window : wm->windows) {
                if (wm->cursor.x > window->x && wm->cursor.x < window->x + window->width &&
                    wm->cursor.y > window->y - 15 && wm->cursor.y < window->y + window->height) {
                    window->is_dragging = false;
                    window->event_queue.insert(window_event_t { .type = WE_MBL_UP, .mouse = { .x = (i32)(wm->cursor.x - window->x), .y = (i32)(wm->cursor.y - window->y) } });
                    break;
                }
            }
            break;
        }
        case kdc_action_t::MDOWNR: {
            for (auto& window :wm-> windows) {
                if (wm->cursor.x > window->x && wm->cursor.x < window->x + window->width &&
                    wm->cursor.y > window->y && wm->cursor.y < window->y + window->height) {
                    window->event_queue.insert(window_event_t { .type = WE_MBR_DOWN, .mouse = { .x = (i32)(wm->cursor.x - window->x), .y = (i32)(wm->cursor.y - window->y) } });
                    break;
                }
            }
            break;
        }
        case kdc_action_t::MUPR: {
            for (auto& window : wm->windows) {
                if (wm->cursor.x > window->x && wm->cursor.x < window->x + window->width &&
                    wm->cursor.y > window->y && wm->cursor.y < window->y + window->height) {
                    window->event_queue.insert(window_event_t { .type = WE_MBR_UP, .mouse = { .x = (i32)(wm->cursor.x - window->x), .y = (i32)(wm->cursor.y - window->y) } });
                    break;
                }
            }
            break;
        }
        default:
            break;
    }
}

int wm_render_loop() {
    window_manager_t* wm = get_global_window_manager();

    while (true) {
        if (!wm->should_render)
            vthread_yield();

        wm->should_render = false;

        disable_interrupts();

        graphics_driver_t* __gd = get_global_graphics_driver();

        size_t max_x, max_y;
        graphics_driver_get_size(__gd, &max_x, &max_y);

        // graphics_driver_draw_square(__gd, 0, 0, max_x, max_y, { 0, 0, 0 });
        memset(__gd->framebuffer->back_buffer, 0, __gd->framebuffer->size);

        for (const auto& w : wm->windows) {
            graphics_driver_draw_square(__gd, w->x - 1, w->y - 15, w->width + 2, w->height + 2 + 15, { 255, 255, 255 });
            void* page_table_current = amd64_get_page_table();
            amd64_set_page_table(vmem_virtual_to_physical(w->parent_process->page_table));
            framebuffer_copy_remote_square(__gd->framebuffer, w->buffer, w->x, w->y, w->width, w->height, 0, 0);
            amd64_set_page_table(page_table_current);
        }

        graphics_driver_draw_linev(__gd, wm->cursor.x, wm->cursor.y, 10, { 0, 0, 0 });
        graphics_driver_draw_pixel(__gd, wm->cursor.x + 1, wm->cursor.y + 1, { 0, 0, 0 });
        graphics_driver_draw_pixel(__gd, wm->cursor.x + 2, wm->cursor.y + 2, { 0, 0, 0 });
        graphics_driver_draw_pixel(__gd, wm->cursor.x + 3, wm->cursor.y + 3, { 0, 0, 0 });
        graphics_driver_draw_pixel(__gd, wm->cursor.x + 4, wm->cursor.y + 4, { 0, 0, 0 });
        graphics_driver_draw_pixel(__gd, wm->cursor.x + 5, wm->cursor.y + 5, { 0, 0, 0 });
        graphics_driver_draw_pixel(__gd, wm->cursor.x + 6, wm->cursor.y + 6, { 0, 0, 0 });
        graphics_driver_draw_lineh(__gd, wm->cursor.x + 4, wm->cursor.y + 7, 3, { 0, 0, 0 });
        graphics_driver_draw_pixel(__gd, wm->cursor.x + 1, wm->cursor.y + 9, { 0, 0, 0 });
        graphics_driver_draw_pixel(__gd, wm->cursor.x + 2, wm->cursor.y + 8, { 0, 0, 0 });
        graphics_driver_draw_pixel(__gd, wm->cursor.x + 3, wm->cursor.y + 7, { 0, 0, 0 });

        // inline
        graphics_driver_draw_linev(__gd, wm->cursor.x + 1, wm->cursor.y + 2, 7, { 255, 255, 255 });
        graphics_driver_draw_linev(__gd, wm->cursor.x + 2, wm->cursor.y + 3, 5, { 255, 255, 255 });
        graphics_driver_draw_linev(__gd, wm->cursor.x + 3, wm->cursor.y + 4, 3, { 255, 255, 255 });
        graphics_driver_draw_linev(__gd, wm->cursor.x + 4, wm->cursor.y + 5, 2, { 255, 255, 255 });
        graphics_driver_draw_linev(__gd, wm->cursor.x + 5, wm->cursor.y + 6, 1, { 255, 255, 255 });

        enable_interrupts();
    }
    
    return 0;
}