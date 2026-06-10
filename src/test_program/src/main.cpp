#include "common.hpp"
#include "vrosapi/memory.hpp"
#include "vrosapi/window.hpp"

void event_hook(window_handle_t handle, window_event_t event) {
    switch (event.type) {
        case WE_MBL_DOWN:
            syscall_malloc(sizeof(i64));
            break;
        case WE_MBL_UP:
            break;
        default:
            break;
    }
}

int main() {
    // create inter process memory space
    // register screen buffer to kernel
    // render dekstop
    // handle mouse events
    // window management

    u64 window_width = 400;
    u64 window_height = 400;

    window_desc_t wnd_desc {};
    memzero(&wnd_desc, sizeof(window_desc_t));
    wnd_desc.rect.h = window_height;
    wnd_desc.rect.w = window_width;
    wnd_desc.event_hook = event_hook;

    u64 handle = syscall_create_window(&wnd_desc);
    void* buffer = syscall_get_window_buffer(handle);
    memzero(buffer, (window_width * window_height) * sizeof(u32));

    syscall_render_window(handle);

    while (true) {
        window_event_t event {};
        if (syscall_poll_event(handle, &event, nullptr))
            event_hook(handle, event);
    }

    return 0;
}