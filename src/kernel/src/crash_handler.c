#include "crash_handler.h"
#include "io/debug.h"
#include "processor/cpu_direct.h"
#include "graphics/framebuffer.h"
#include "libstring.h"
#include "libarray.h"

#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
#include "processor/amd64_crash_handler.h"
#endif

static
void crash_draw_centered(u64 screen_width, i64 y, const char* text, u32 color) {
    const u64 text_width = strlen(text) * 8;
    const i64 x = text_width < screen_width ? (i64)((screen_width - text_width) / 2) : 0;
    framebuffer_draw_text8x16(x, y, text, color, 0, false);
}

static
void crash_draw_centered_lines(u64 screen_width, i64 y, const char* const* lines, u64 line_count, u32 color) {
    u64 block_width = 0;
    for (u64 i = 0; i < line_count; i++) {
        const u64 line_width = strlen(lines[i]) * 8;
        if (line_width > block_width)
            block_width = line_width;
    }

    const i64 x = block_width < screen_width ? (i64)((screen_width - block_width) / 2) : 0;
    for (u64 i = 0; i < line_count; i++)
        framebuffer_draw_text8x16(x, y + (i * 16), lines[i], color, 0, false);
}

static
void crash_draw_register_dump(const void* frame, u64 screen_width, i64 y, u32 color) {
    if (!frame) {
        crash_draw_centered(screen_width, y, "Register context unavailable.", color);
        return;
    }

#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    static_array_t registers = amd64_get_crash_registers(frame);
#else
    static_array_t registers;
    static_array_init(&registers, 0, sizeof(crash_register_entry_t));
#endif

    const u64 count = registers.capacity;
    const u64 rows_per_column = (count + 1) / 2;
    const i64 column_width = 25 * 8;
    const i64 column_gap = 16;
    const i64 block_width = column_width * 2 + column_gap;
    const i64 start_x = screen_width > (u64)block_width ? (i64)((screen_width - (u64)block_width) / 2) : 0;

    for (u64 i = 0; i < count; i++) {
        const u64 column = i / rows_per_column;
        const u64 row = i % rows_per_column;
        const i64 x = start_x + (i64)column * (column_width + column_gap);
        char line[26];

        crash_register_entry_t* entry = static_array_get(&registers, i);
        sprintf(line, sizeof(line), "%s=%llx", entry->name, entry->value);
        framebuffer_draw_text8x16(x, y + (i64)row * 16, line, color, 0, false);
    }

    static_array_free(&registers);
}

__noreturn
void crash_handler_entry(void* frame, const char* message, u64 core) {
    (void)frame;
    if (!message)
        message = "No crash details available.";

    kdbg_uprintf("crash_handler_entry: %s (core %llu)\n", message, core);
    cpu_kill_all_cores_except(core);

    if (!framebuffer_is_ready()) {
        kdbg_uprintf("no framebuffer available\n");
        cpu_hang();
    }

    const u64 screen_width = framebuffer_width();
    const u64 screen_height = framebuffer_height();
    const u32 black = framebuffer_color(0, 0, 0);
    const u32 white = framebuffer_color(255, 255, 255);
    const u32 red = framebuffer_color(255, 72, 72);
    const u32 dim = framebuffer_color(190, 190, 190);

    framebuffer_clear(black);

    const i64 top = screen_height > 128 ? (i64)((screen_height - 128) / 2) : 0;

    crash_draw_centered(screen_width, top + (16 * 0), "Unrecoverable Exception", red);
    crash_draw_centered(screen_width, top + (16 * 1), message, dim);

    static const char* const recovery_lines[] = {
        "The system encountered an unrecoverable situation.",
        "No automatic recovery is possible.",
        "",
        "A system reboot is required."
    };

    crash_draw_centered_lines(screen_width, top + (16 * 3), recovery_lines, 4, white);

    crash_draw_register_dump(frame, screen_width, top + 152, dim);

    framebuffer_present();

    cpu_hang();
}