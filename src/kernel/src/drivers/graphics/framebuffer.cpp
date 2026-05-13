#include "drivers/graphics/framebuffer.hpp"

uint32_t framebuffer_format_color(framebuffer_t* framebuffer, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!framebuffer)
        return 0;

    switch (framebuffer->format) {
        case framebuffer_color_format_t::ARGB888:
            return a << 24 | r << 16 | g << 8 | b;
        case framebuffer_color_format_t::RGBA888:
            return r << 24 | g << 16 | b << 8 | a;
        case framebuffer_color_format_t::BGRA888:
            return b << 24 | g << 16 | r << 8 | a;
        case framebuffer_color_format_t::ABGR888:
            return a << 24 | b << 16 | g << 8 | r;
        default:
            return 0;
    }

    return 0;
}

bool framebuffer_init(framebuffer_t* framebuffer, framebuffer_color_format_t format, uint32_t* address, size_t size, size_t width, size_t height, size_t pitch) {
    if (!framebuffer || !address || size == 0)
        return false;

    framebuffer->address = address;
    framebuffer->size = size;
    framebuffer->width = width;
    framebuffer->height = height;
    framebuffer->pitch = pitch;
    framebuffer->format = format;
    framebuffer->caluclated_pitch = (pitch / sizeof(uint32_t));

    return true;
}

bool framebuffer_write_pixel(framebuffer_t* framebuffer, size_t x, size_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return framebuffer_write_pixel(framebuffer, x, y, framebuffer_format_color(framebuffer, r, g, b, a));
}

bool framebuffer_write_pixel(framebuffer_t* framebuffer, size_t x, size_t y, uint32_t color) {
    if (!framebuffer || !framebuffer->address)
        return false;

    if (x >= framebuffer->width || y >= framebuffer->height)
        return false;

    framebuffer->address[y * framebuffer->caluclated_pitch + x] = color;

    return true;
}

bool framebuffer_write_lineh(framebuffer_t* framebuffer, size_t x, size_t y, size_t len, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return framebuffer_write_lineh(framebuffer, x, y, len, framebuffer_format_color(framebuffer, r, g, b, a));
}

bool framebuffer_write_lineh(framebuffer_t* framebuffer, size_t x, size_t y, size_t len, uint32_t color) {
    if (!framebuffer || !framebuffer->address)
        return false;

    if (x >= framebuffer->width || y >= framebuffer->height)
        return false;

    for (size_t i = x; i < x + len && i < framebuffer->width; i++)
        framebuffer->address[y * framebuffer->caluclated_pitch + i] = color;

    return true;
}

bool framebuffer_write_linev(framebuffer_t* framebuffer, size_t x, size_t y, size_t len, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return framebuffer_write_linev(framebuffer, x, y, len, framebuffer_format_color(framebuffer, r, g, b, a));
}

bool framebuffer_write_linev(framebuffer_t* framebuffer, size_t x, size_t y, size_t len, uint32_t color) {
    if (!framebuffer || !framebuffer->address)
        return false;

    if (x >= framebuffer->width || y >= framebuffer->height)
        return false;

    for (size_t i = y; i < y + len && i < framebuffer->width; i++)
        framebuffer->address[i * framebuffer->caluclated_pitch + x] = color;

    return true;
}

bool framebuffer_write_square(framebuffer_t* framebuffer, size_t x, size_t y, size_t w, size_t h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return framebuffer_write_square(framebuffer, x, y, w, h, framebuffer_format_color(framebuffer, r, g, b, a));
}

bool framebuffer_write_square(framebuffer_t* framebuffer, size_t x, size_t y, size_t w, size_t h, uint32_t color) {
    if (!framebuffer || !framebuffer->address)
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
            framebuffer->address[j * framebuffer->caluclated_pitch + i] = color;

    return true;
}

bool framebuffer_move_square(framebuffer_t* framebuffer, size_t x, size_t y, size_t w, size_t h, size_t nx, size_t ny) {
    if (!framebuffer || !framebuffer->address)
        return false;

    if (x + w > framebuffer->width ||
        y + h > framebuffer->height ||
        nx + w > framebuffer->width ||
        ny + h > framebuffer->height)
        return false;

    uint32_t* temp = (uint32_t*)malloc(w * h * sizeof(uint32_t));
    if (!temp)
        return false;

    for (size_t row = 0; row < h; row++) {
        memcpy(
            &temp[row * w],
            &framebuffer->address[(y + row) * framebuffer->caluclated_pitch + x],
            w * sizeof(uint32_t)
        );
    }

    for (size_t row = 0; row < h; row++) {
        memset(
            &framebuffer->address[(y + row) * framebuffer->caluclated_pitch + x],
            0,
            w * sizeof(uint32_t)
        );
    }

    for (size_t row = 0; row < h; row++) {
        memcpy(
            &framebuffer->address[(ny + row) * framebuffer->caluclated_pitch + nx],
            &temp[row * w],
            w * sizeof(uint32_t)
        );
    }

    free(temp);
    return true;
}