#include "bmp.hpp"

bool is_bmp(u8* data, size_t size) {
    if (!data)
        return false;

    if (size < sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t) + sizeof(bmp_color_t))
        return false;

    return data[0] == 'B' && data[1] == 'M';
}

bool bmp_load_image(u8* file_data, u64 file_size, bmp_image_t* image) {
    if (!file_data || file_size == 0 || !image)
        return false;

    if (!is_bmp(file_data, file_size))
        return false;

    image->file_data = file_data;
    image->file_size = file_size;

    bmp_file_header_t* file_header = (bmp_file_header_t*)file_data;
    bmp_info_header_t* info_header = (bmp_info_header_t*)(file_data + sizeof(bmp_file_header_t));

    if (info_header->bits_per_pixel != 8)
        return false;

    u8* color_pallet = (u8*)(file_data + sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t));

    for (int i = 0; i < info_header->number_of_colors; i++) {
        image->colors[i].b = *(color_pallet + (i * 4) + 0);
        image->colors[i].g = *(color_pallet + (i * 4) + 1);
        image->colors[i].r = *(color_pallet + (i * 4) + 2);
    }

    image->width = info_header->width;
    image->height = info_header->height;
    image->bottom_up = true;

    if (image->height < 0) {
        image->height = -image->height;
        image->bottom_up = false;
    }

    return true;
}