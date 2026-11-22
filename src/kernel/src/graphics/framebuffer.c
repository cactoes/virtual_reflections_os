#include "graphics/framebuffer.h"
#include "memory/generic.h"
#include "libmem.h"

static framebuffer_state_t global_framebuffer;

static inline
u32* row_ptr(u64 y) {
    return (u32*)(global_framebuffer.draw + y * global_framebuffer.pitch);
}

// CLEAN @since 22/11/2026 -- 23:10
// move this to actual memset32 function
static inline
void fill32(u32* dst, u32 value, u64 count) {
    asm volatile("rep stosl" : "+D"(dst), "+c"(count) : "a"(value) : "memory");
}

static
void mark_dirty(i64 x, i64 y, i64 w, i64 h) {
    if (!global_framebuffer.buffered)
        return;

    const u64 x0 = (u64)x;
    const u64 y0 = (u64)y;
    const u64 x1 = (u64)(x + w);
    const u64 y1 = (u64)(y + h);

    if (global_framebuffer.dirty_x0 >= global_framebuffer.dirty_x1) {
        global_framebuffer.dirty_x0 = x0;
        global_framebuffer.dirty_y0 = y0;
        global_framebuffer.dirty_x1 = x1;
        global_framebuffer.dirty_y1 = y1;
        return;
    }

    if (x0 < global_framebuffer.dirty_x0)
        global_framebuffer.dirty_x0 = x0;

    if (y0 < global_framebuffer.dirty_y0)
        global_framebuffer.dirty_y0 = y0;

    if (x1 > global_framebuffer.dirty_x1)
        global_framebuffer.dirty_x1 = x1;

    if (y1 > global_framebuffer.dirty_y1)
        global_framebuffer.dirty_y1 = y1;
}

static bool clip_rect(i64* x, i64* y, i64* w, i64* h) {
    if (*w <= 0 || *h <= 0)
        return false;

    i64 x0 = *x;
    i64 y0 = *y;
    i64 x1 = *x + *w;
    i64 y1 = *y + *h;

    if (x0 < 0)
        x0 = 0;

    if (y0 < 0)
        y0 = 0;

    if (x1 > (i64)global_framebuffer.width)
        x1 = (i64)global_framebuffer.width;

    if (y1 > (i64)global_framebuffer.height)
        y1 = (i64)global_framebuffer.height;

    if (x0 >= x1 || y0 >= y1)
        return false;

    *x = x0;
    *y = y0;
    *w = x1 - x0;
    *h = y1 - y0;
    return true;
}

bool framebuffer_init(framebuffer_t* framebuffer, void* backbuffer) {
   if (!framebuffer || !framebuffer->valid || !framebuffer->physical_address || !framebuffer->width || !framebuffer->height)
        return false;

    if (framebuffer->pitch < framebuffer->width * 4)
        return false;

    switch (framebuffer->format) {
        case FB_FORMAT_ARGB8888: global_framebuffer.a_shift = 24; global_framebuffer.r_shift = 16; global_framebuffer.g_shift = 8; global_framebuffer.b_shift = 0; break;
        case FB_FORMAT_RGBA8888: global_framebuffer.r_shift = 24; global_framebuffer.g_shift = 16; global_framebuffer.b_shift = 8; global_framebuffer.a_shift = 0; break;
        case FB_FORMAT_BGRA8888: global_framebuffer.b_shift = 24; global_framebuffer.g_shift = 16; global_framebuffer.r_shift = 8; global_framebuffer.a_shift = 0; break;
        case FB_FORMAT_ABGR8888: global_framebuffer.a_shift = 24; global_framebuffer.b_shift = 16; global_framebuffer.g_shift = 8; global_framebuffer.r_shift = 0; break;
        default: return false;
    }

    global_framebuffer.front = kmmio_map((void*)framebuffer->physical_address, framebuffer->pitch * framebuffer->height);
    if (!global_framebuffer.front)
        return false;

    global_framebuffer.width = framebuffer->width;
    global_framebuffer.height = framebuffer->height;
    global_framebuffer.pitch = framebuffer->pitch;

    global_framebuffer.buffered = backbuffer != nullptr;
    global_framebuffer.draw = global_framebuffer.buffered ? (u8*)backbuffer : global_framebuffer.front;

    global_framebuffer.dirty_x0 = global_framebuffer.dirty_y0 = global_framebuffer.dirty_x1 = global_framebuffer.dirty_y1 = 0;
    return true;
}

void framebuffer_fill_rect(i64 x, i64 y, i64 w, i64 h, u32 color) {
    if (!clip_rect(&x, &y, &w, &h))
        return;

    for (i64 j = 0; j < h; j++)
        fill32(row_ptr((u64)(y + j)) + x, color, (u64)w);

    mark_dirty(x, y, w, h);
}

void framebuffer_clear(u32 color) {
    framebuffer_fill_rect(0, 0, (i64)global_framebuffer.width, (i64)global_framebuffer.height, color);
}

void framebuffer_put_pixel(i64 x, i64 y, u32 color) {
    if ((u64)x >= global_framebuffer.width || (u64)y >= global_framebuffer.height)
        return;

    row_ptr((u64)y)[x] = color;
    mark_dirty(x, y, 1, 1);
}

void framebuffer_line(i64 x0, i64 y0, i64 x1, i64 y1, u32 color) {
    const i64 dx =  (x1 > x0) ? (x1 - x0) : (x0 - x1);
    const i64 dy = -((y1 > y0) ? (y1 - y0) : (y0 - y1));
    const i64 sx = x0 < x1 ? 1 : -1;
    const i64 sy = y0 < y1 ? 1 : -1;
    i64 err = dx + dy;

    for (;;) {
        framebuffer_put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;

        const i64 e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void framebuffer_rect(i64 x, i64 y, i64 w, i64 h, u32 color) {
    if (w <= 0 || h <= 0)
        return;

    framebuffer_fill_rect(x, y, w, 1, color);
    if (h > 1)
        framebuffer_fill_rect(x, y + h - 1, w, 1, color);

    if (h > 2) {
        framebuffer_fill_rect(x, y + 1, 1, h - 2, color);
        if (w > 1)
            framebuffer_fill_rect(x + w - 1, y + 1, 1, h - 2, color);
    }
}

void framebuffer_blit(i64 x, i64 y, i64 w, i64 h, const u32* src, u64 src_stride) {
    if (!src)
        return;

    const i64 ox = x;
    const i64 oy = y;
    if (!clip_rect(&x, &y, &w, &h))
        return;

    const u32* s = src + (u64)(y - oy) * src_stride + (u64)(x - ox);
    for (i64 j = 0; j < h; j++, s += src_stride)
        memcpy(row_ptr((u64)(y + j)) + x, s, (u64)w * 4);

    mark_dirty(x, y, w, h);
}

bool framebuffer_copy_rect(i64 sx, i64 sy, i64 w, i64 h, i64 dx, i64 dy) {
    i64 cx = sx;
    i64 cy = sy;
    i64 cw = w;
    i64 ch = h;
    if (!clip_rect(&cx, &cy, &cw, &ch))
        return false;

    dx += cx - sx;
    dy += cy - sy;
    sx = cx;
    sy = cy;
    w = cw;
    h = ch;

    i64 tx = dx;
    i64 ty = dy;
    i64 tw = w;
    i64 th = h;
    if (!clip_rect(&tx, &ty, &tw, &th))
        return false;

    sx += tx - dx;
    sy += ty - dy;
    dx = tx;
    dy = ty;
    w = tw;
    h = th;

    if (dy <= sy) {
        for (i64 j = 0; j < h; j++)
            memmove(row_ptr((u64)(dy + j)) + dx, row_ptr((u64)(sy + j)) + sx, (u64)w * 4);
    } else {
        for (i64 j = h - 1; j >= 0; j--)
            memmove(row_ptr((u64)(dy + j)) + dx, row_ptr((u64)(sy + j)) + sx, (u64)w * 4);
    }

    mark_dirty(dx, dy, w, h);
    return true;
}

void framebuffer_draw_bitmap_1bpp(i64 x, i64 y, i64 w, i64 h, const u8* bits, u64 stride_bytes, u32 fg, u32 bg, bool opaque_bg) {
    if (!bits)
        return;

    const i64 ox = x;
    const i64 oy = y;
    if (!clip_rect(&x, &y, &w, &h))
        return;

    for (i64 j = 0; j < h; j++) {
        const u8* src_row = bits + (u64)(y - oy + j) * stride_bytes;
        u32* dst_row = row_ptr((u64)(y + j));

        for (i64 i = 0; i < w; i++) {
            const u64 col = (u64)(x - ox + i);
            if (src_row[col >> 3] & (0x80 >> (col & 7)))
                dst_row[x + i] = fg;
            else if (opaque_bg)
                dst_row[x + i] = bg;
        }
    }

    mark_dirty(x, y, w, h);
}

void framebuffer_invalidate() {
    if (!global_framebuffer.buffered)
        return;

    global_framebuffer.dirty_x0 = 0;
    global_framebuffer.dirty_y0 = 0;
    global_framebuffer.dirty_x1 = global_framebuffer.width;
    global_framebuffer.dirty_y1 = global_framebuffer.height;
}

void framebuffer_present() {
   if (!global_framebuffer.buffered || global_framebuffer.dirty_x0 >= global_framebuffer.dirty_x1 || global_framebuffer.dirty_y0 >= global_framebuffer.dirty_y1)
        return;

    const u64 offset = global_framebuffer.dirty_x0 * 4;
    const u64 bytes = (global_framebuffer.dirty_x1 - global_framebuffer.dirty_x0) * 4;

    for (u64 y = global_framebuffer.dirty_y0; y < global_framebuffer.dirty_y1; y++)
        memcpy(global_framebuffer.front + y * global_framebuffer.pitch + offset, global_framebuffer.draw + y * global_framebuffer.pitch + offset, bytes);

    global_framebuffer.dirty_x0 = global_framebuffer.dirty_y0 = global_framebuffer.dirty_x1 = global_framebuffer.dirty_y1 = 0;
}

u64 framebuffer_width() {
    return global_framebuffer.width;
}

u64 framebuffer_height() {
    return global_framebuffer.height;
}

u32 framebuffer_color(u8 r, u8 g, u8 b) {
    return ((u32)r << global_framebuffer.r_shift) | ((u32)g << global_framebuffer.g_shift) | ((u32)b << global_framebuffer.b_shift) | ((u32)0xff << global_framebuffer.a_shift);
}