//==========================================
/// @file       bmp.hpp
/// @brief      
//==========================================

#pragma once

#ifndef BMP_HPP
#define BMP_HPP

#include "common.hpp"

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

struct bmp_image_t {
    u8* file_data;
    u64 file_size;

    i32 width;
    i32 height;
    bool bottom_up;

    bmp_color_t colors[256];
};

bool is_bmp(u8* data, size_t size);
bool bmp_load_image(u8* file_data, u64 file_size, bmp_image_t* image);

#endif // BMP_HPP