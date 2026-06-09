#include "gui/display_driver.hpp"
#include "drivers/graphics/graphics_driver.hpp"
#include "interrupt_manager.hpp"
#include "time/clock.hpp"
#include "virtual_thread.hpp"

static void* dd_remote_buffer = nullptr;
static u64 dd_target_fps_ms = 1000 / 60;

void dd_set_active_buffer(void* buffer) {
    dd_remote_buffer = buffer;
}

bool dd_buffer_render_loop() {
    graphics_driver_t* gd = get_global_graphics_driver();
    if (!gd)
        return false;

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

    return true;
}