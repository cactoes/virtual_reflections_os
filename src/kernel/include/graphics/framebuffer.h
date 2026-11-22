//==========================================
/// @file       framebuffer.h
/// @brief      
//==========================================

#pragma once

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "libtype.h"
#include "kbootinfo.h"

typedef struct __framebuffer_state {
    u8* front;
    u8* draw;
    u64  width, height, pitch;
    u8  r_shift, g_shift, b_shift, a_shift;
    bool buffered;
    u64 dirty_x0, dirty_y0, dirty_x1, dirty_y1;
} framebuffer_state_t;

bool framebuffer_init(framebuffer_t* framebuffer, void* backbuffer);
void framebuffer_clear(u32 color);

void framebuffer_put_pixel(i64 x, i64 y, u32 color);
void framebuffer_line(i64 x0, i64 y0, i64 x1, i64 y1, u32 color);
void framebuffer_rect(i64 x, i64 y, i64 w, i64 h, u32 color);
void framebuffer_fill_rect(i64 x, i64 y, i64 w, i64 h, u32 color);

void framebuffer_blit(i64 x, i64 y, i64 w, i64 h, const u32* src, u64 src_stride);
bool framebuffer_copy_rect(i64 sx, i64 sy, i64 w, i64 h, i64 dx, i64 dy);
void framebuffer_draw_bitmap_1bpp(i64 x, i64 y, i64 w, i64 h, const u8* bits, u64 stride_bytes, u32 fg, u32 bg, bool opaque_bg);

void framebuffer_invalidate();
void framebuffer_present();

#endif // FRAMEBUFFER_H