#include "drivers/graphics/framebuffer.hpp"

u32 framebuffer_format_color(framebuffer_t* framebuffer, u8 r, u8 g, u8 b, u8 a) {
    if (!framebuffer)
        return 0;

    switch (framebuffer->format) {
        case framebuffer_color_format_t::ARGB8888:
            return a << 24 | r << 16 | g << 8 | b;
        case framebuffer_color_format_t::RGBA8888:
            return r << 24 | g << 16 | b << 8 | a;
        case framebuffer_color_format_t::BGRA8888:
            return b << 24 | g << 16 | r << 8 | a;
        case framebuffer_color_format_t::ABGR8888:
            return a << 24 | b << 16 | g << 8 | r;
        default:
            return 0;
    }

    return 0;
}

bool framebuffer_init(framebuffer_t* framebuffer, framebuffer_color_format_t format, u32* address, size_t size, size_t width, size_t height, size_t pitch) {
    if (!framebuffer || !address || size == 0)
        return false;

    framebuffer->address = address;
    framebuffer->back_buffer = (u32*)malloc(size);
    framebuffer->size = size;
    framebuffer->width = width;
    framebuffer->height = height;
    framebuffer->pitch = pitch;
    framebuffer->format = format;
    framebuffer->caluclated_pitch = (pitch / sizeof(u32));

    return true;
}

void* framebuffer_create_buffer(framebuffer_t* framebuffer) {
    if (!framebuffer)
        return nullptr;

    return (u32*)malloc(framebuffer->size);
}

bool framebuffer_write_pixel(framebuffer_t* framebuffer, size_t x, size_t y, u8 r, u8 g, u8 b, u8 a) {
    return framebuffer_write_pixel(framebuffer, x, y, framebuffer_format_color(framebuffer, r, g, b, a));
}

bool framebuffer_write_pixel(framebuffer_t* framebuffer, size_t x, size_t y, u32 color) {
    if (!framebuffer || !framebuffer->back_buffer)
        return false;

    if (x >= framebuffer->width || y >= framebuffer->height)
        return false;

    framebuffer->back_buffer[y * framebuffer->caluclated_pitch + x] = color;

    return true;
}

void framebuffer_write_pixel_raw(framebuffer_t* framebuffer, size_t x, size_t y, u32 color) {
    framebuffer->back_buffer[y * framebuffer->caluclated_pitch + x] = color;
}

bool framebuffer_write_lineh(framebuffer_t* framebuffer, size_t x, size_t y, size_t len, u8 r, u8 g, u8 b, u8 a) {
    return framebuffer_write_lineh(framebuffer, x, y, len, framebuffer_format_color(framebuffer, r, g, b, a));
}

bool framebuffer_write_lineh(framebuffer_t* framebuffer, size_t x, size_t y, size_t len, u32 color) {
    if (!framebuffer || !framebuffer->back_buffer)
        return false;

    if (x >= framebuffer->width || y >= framebuffer->height)
        return false;

    for (size_t i = x; i < x + len && i < framebuffer->width; i++)
        framebuffer->back_buffer[y * framebuffer->caluclated_pitch + i] = color;

    return true;
}

bool framebuffer_write_linev(framebuffer_t* framebuffer, size_t x, size_t y, size_t len, u8 r, u8 g, u8 b, u8 a) {
    return framebuffer_write_linev(framebuffer, x, y, len, framebuffer_format_color(framebuffer, r, g, b, a));
}

bool framebuffer_write_linev(framebuffer_t* framebuffer, size_t x, size_t y, size_t len, u32 color) {
    if (!framebuffer || !framebuffer->back_buffer)
        return false;

    if (x >= framebuffer->width || y >= framebuffer->height)
        return false;

    for (size_t i = y; i < y + len && i < framebuffer->width; i++)
        framebuffer->back_buffer[i * framebuffer->caluclated_pitch + x] = color;

    return true;
}

bool framebuffer_write_square(framebuffer_t* framebuffer, size_t x, size_t y, size_t w, size_t h, u8 r, u8 g, u8 b, u8 a) {
    return framebuffer_write_square(framebuffer, x, y, w, h, framebuffer_format_color(framebuffer, r, g, b, a));
}

bool framebuffer_write_square(framebuffer_t* framebuffer, size_t x, size_t y, size_t w, size_t h, u32 color) {
    if (!framebuffer || !framebuffer->back_buffer)
        return false;

    if (x >= framebuffer->width || y >= framebuffer->height)
        return false;

    size_t max_x = x + w;
    size_t max_y = y + h;

    if (max_x > framebuffer->width)
        max_x = framebuffer->width;

    if (max_y > framebuffer->height)
        max_y = framebuffer->height;

    for (size_t j = y; j < max_y; j++)
        for (size_t i = x; i < max_x; i++)
            framebuffer->back_buffer[j * framebuffer->caluclated_pitch + i] = color;

    return true;
}

bool framebuffer_move_square(framebuffer_t* framebuffer, size_t x, size_t y, size_t w, size_t h, size_t nx, size_t ny) {
    if (!framebuffer || !framebuffer->back_buffer)
        return false;

    if (x + w > framebuffer->width ||
        y + h > framebuffer->height ||
        nx + w > framebuffer->width ||
        ny + h > framebuffer->height)
        return false;

    u32* temp = (u32*)malloc(w * h * sizeof(u32));
    if (!temp)
        return false;

    for (size_t row = 0; row < h; row++) {
        memcpy(
            &temp[row * w],
            &framebuffer->back_buffer[(y + row) * framebuffer->caluclated_pitch + x],
            w * sizeof(u32)
        );
    }

    for (size_t row = 0; row < h; row++) {
        memset(
            &framebuffer->back_buffer[(y + row) * framebuffer->caluclated_pitch + x],
            0,
            w * sizeof(u32)
        );
    }

    for (size_t row = 0; row < h; row++) {
        memcpy(
            &framebuffer->back_buffer[(ny + row) * framebuffer->caluclated_pitch + nx],
            &temp[row * w],
            w * sizeof(u32)
        );
    }

    free(temp);
    return true;
}

bool framebuffer_copy_remote_square(framebuffer_t* framebuffer, void* buffer, u64 tx, u64 ty, u64 w, u64 h, u64 sx, u64 sy) {
    if (!framebuffer || !framebuffer->back_buffer)
        return false;

    u8* src = (u8*)buffer;

    for (size_t row = 0; row < h; row++) {
        memcpy(
            &framebuffer->back_buffer[(ty + row) * framebuffer->caluclated_pitch + tx],
            &src[(sy + row) * w * sizeof(u32) + sx * sizeof(u32)],
            w * sizeof(u32)
        );
    }

    return true;
}

bool framebuffer_swap_buffer(framebuffer_t* framebuffer, void** old_buffer, void* new_buffer) {
    if (!framebuffer || !old_buffer || !new_buffer)
        return false;

    *old_buffer = framebuffer->back_buffer;
    framebuffer->back_buffer = (u32*)new_buffer;

    return true;
}

bool framebuffer_copy_buffer(framebuffer_t* framebuffer, void* buffer) {
    if (!framebuffer || !buffer)
        return false;

    memcpy(framebuffer->back_buffer, buffer, framebuffer->size);

    return true;
}

bool framebuffer_render(framebuffer_t* framebuffer) {
    if (!framebuffer)
        return false;

    void* dst = framebuffer->address;
    void* src = framebuffer->back_buffer;

    // memcpy(framebuffer->address, framebuffer->back_buffer, framebuffer->size);

    asm volatile (
        "movq %[size], %%rcx\n"
        "shrq $6, %%rcx\n"
        "1:\n"
        "movdqa (%[src]), %%xmm0\n"
        "movdqa 16(%[src]), %%xmm1\n"
        "movdqa 32(%[src]), %%xmm2\n"
        "movdqa 48(%[src]), %%xmm3\n"
        "movntdq %%xmm0, (%[dst])\n"
        "movntdq %%xmm1, 16(%[dst])\n"
        "movntdq %%xmm2, 32(%[dst])\n"
        "movntdq %%xmm3, 48(%[dst])\n"
        "addq $64, %[src]\n"
        "addq $64, %[dst]\n"
        "decq %%rcx\n"
        "jnz 1b\n"
        "sfence\n"
        : [src] "+r" (src), [dst] "+r" (dst)
        : [size] "r" (framebuffer->size)
        : "rcx", "xmm0", "xmm1", "xmm2", "xmm3", "memory"
    );

    return true;
}