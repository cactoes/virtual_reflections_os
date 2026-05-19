#include "drivers/graphics/graphics_driver.hpp"
#include "memory/heap.hpp"
#include "memory/vmem.hpp"

static graphics_driver_t* global_graphics_driver = nullptr;

const u8 font8x8[95][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // space (32)
    { 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00 }, // ! (33)
    { 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // " (34)
    { 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00 }, // # (35)
    { 0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00 }, // $ (36)
    { 0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00 }, // % (37)
    { 0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00 }, // & (38)
    { 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00 }, // ' (39)
    { 0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00 }, // ( (40)
    { 0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00 }, // ) (41)
    { 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00 }, // * (42)
    { 0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00 }, // + (43)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06 }, // , (44)
    { 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00 }, // - (45)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00 }, // . (46)
    { 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00 }, // / (47)
    { 0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00 }, // 0 (48)
    { 0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00 }, // 1 (49)
    { 0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00 }, // 2 (50)
    { 0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00 }, // 3 (51)
    { 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00 }, // 4 (52)
    { 0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00 }, // 5 (53)
    { 0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00 }, // 6 (54)
    { 0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00 }, // 7 (55)
    { 0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00 }, // 8 (56)
    { 0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00 }, // 9 (57)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00 }, // : (58)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06 }, // ; (59)
    { 0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00 }, // < (60)
    { 0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00 }, // = (61)
    { 0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00 }, // > (62)
    { 0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00 }, // ? (63)
    { 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00 }, // @ (64)
    { 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00 }, // A (65)
    { 0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00 }, // B (66)
    { 0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00 }, // C (67)
    { 0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00 }, // D (68)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00 }, // E (69)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00 }, // F (70)
    { 0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00 }, // G (71)
    { 0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00 }, // H (72)
    { 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 }, // I (73)
    { 0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00 }, // J (74)
    { 0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00 }, // K (75)
    { 0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00 }, // L (76)
    { 0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00 }, // M (77)
    { 0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00 }, // N (78)
    { 0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00 }, // O (79)
    { 0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00 }, // P (80)
    { 0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00 }, // Q (81)
    { 0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00 }, // R (82)
    { 0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00 }, // S (83)
    { 0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 }, // T (84)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00 }, // U (85)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00 }, // V (86)
    { 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00 }, // W (87)
    { 0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00 }, // X (88)
    { 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00 }, // Y (89)
    { 0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00 }, // Z (90)
    { 0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00 }, // [ (91)
    { 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00 }, // \ (92)
    { 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00 }, // ] (93)
    { 0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00 }, // ^ (94)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF }, // _ (95)
    { 0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00 }, // ` (96)
    { 0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00 }, // a (97)
    { 0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00 }, // b (98)
    { 0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00 }, // c (99)
    { 0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6E, 0x00 }, // d (100)
    { 0x00, 0x00, 0x1E, 0x33, 0x3f, 0x03, 0x1E, 0x00 }, // e (101)
    { 0x1C, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0F, 0x00 }, // f (102)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F }, // g (103)
    { 0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00 }, // h (104)
    { 0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 }, // i (105)
    { 0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E }, // j (106)
    { 0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00 }, // k (107)
    { 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 }, // l (108)
    { 0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00 }, // m (109)
    { 0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00 }, // n (110)
    { 0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00 }, // o (111)
    { 0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F }, // p (112)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78 }, // q (113)
    { 0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00 }, // r (114)
    { 0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00 }, // s (115)
    { 0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00 }, // t (116)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00 }, // u (117)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00 }, // v (118)
    { 0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00 }, // w (119)
    { 0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00 }, // x (120)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F }, // y (121)
    { 0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00 }, // z (122)
    { 0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00 }, // { (123)
    { 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00 }, // | (124)
    { 0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00 }, // } (125)
    { 0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // ~ (126)
};

const u8 font8x8_fallback[8] = {
    0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF
};

graphics_driver_t* get_global_graphics_driver() {
    return global_graphics_driver;
}

void set_global_graphics_driver(graphics_driver_t* graphics_driver) {
    global_graphics_driver = graphics_driver;
}

framebuffer_color_format_t get_framebuffer_format(multiboot2_tag_framebuffer_t* tag) {
    if (tag->framebuffer_type != MULTIBOOT2_FRAMEBUFFER_TYPE_RGB ||
        tag->framebuffer_bpp != MULTIBOOT2_FRAMEBUFFER_BPP_32BIT)
        return framebuffer_color_format_t::UNKNOWN;

    if (tag->rgb.framebuffer_red_field_position == 16 &&
        tag->rgb.framebuffer_green_field_position == 8 &&
        tag->rgb.framebuffer_blue_field_position == 0)
        return framebuffer_color_format_t::ARGB888;

    if (tag->rgb.framebuffer_red_field_position == 24 &&
        tag->rgb.framebuffer_green_field_position == 16 &&
        tag->rgb.framebuffer_blue_field_position == 8)
        return framebuffer_color_format_t::RGBA888;

    if (tag->rgb.framebuffer_red_field_position == 8 &&
        tag->rgb.framebuffer_green_field_position == 16 &&
        tag->rgb.framebuffer_blue_field_position == 24)
        return framebuffer_color_format_t::BGRA888;

    if (tag->rgb.framebuffer_red_field_position == 0 &&
        tag->rgb.framebuffer_green_field_position == 8 &&
        tag->rgb.framebuffer_blue_field_position == 16)
        return framebuffer_color_format_t::ABGR888;

    return framebuffer_color_format_t::UNKNOWN;
}

bool graphics_driver_init_framebuffer(graphics_driver_t* graphics_driver, multiboot2_info_t* multiboot_struct) {
    graphics_driver->type = graphics_driver_type_t::FRAMEBUFFER;
    
    multiboot2_tag_framebuffer_t* framebuffer_tag = mb2_get_framebuffer(multiboot_struct);
    if (!framebuffer_tag)
        return false;

    framebuffer_color_format_t format = get_framebuffer_format(framebuffer_tag);

    if (format == framebuffer_color_format_t::UNKNOWN)
        return false;

    const size_t framebuffer_size = (framebuffer_tag->framebuffer_pitch * framebuffer_tag->framebuffer_height);
    void* mapped_framebuffer = (void*)malloc(align_up(framebuffer_size, PAGE_SIZE_LARGE));
    for (size_t i = 0; i < align_up(framebuffer_size, PAGE_SIZE_LARGE); i += PAGE_SIZE_LARGE)
        vmem_map_2mb(nullptr, (void*)((u64)mapped_framebuffer + i), (void*)(framebuffer_tag->framebuffer_addr + i));

    graphics_driver->framebuffer = (framebuffer_t*)malloc(sizeof(framebuffer_t));

    if (!framebuffer_init(graphics_driver->framebuffer,
        format,
        (u32*)mapped_framebuffer,
        framebuffer_size,
        framebuffer_tag->framebuffer_width,
        framebuffer_tag->framebuffer_height,
        framebuffer_tag->framebuffer_pitch))
        return false;

    return true;
}

bool graphics_driver_init_vga(graphics_driver_t* graphics_driver, multiboot2_info_t* multiboot_struct) {
    graphics_driver->vgabuffer = (vga_buffer_t*)malloc(sizeof(vga_buffer_t));

    graphics_driver->type = graphics_driver_type_t::VGA;

    if (!vga_gm_buffer_create(graphics_driver->vgabuffer))
        return false;

    vga_gm_startup(graphics_driver->vgabuffer);

    if (!vga_gm_draw::clear(graphics_driver->vgabuffer, vga_gm_color_index_t::BLACK))
        return false;

    return vga_gm_render();
}

bool graphics_driver_init(graphics_driver_t* graphics_driver, multiboot2_info_t* multiboot_struct) {
    if (!graphics_driver || !multiboot_struct)
        return false;

    if (graphics_driver_init_framebuffer(graphics_driver, multiboot_struct))
        return true;

    if (graphics_driver->framebuffer)
        free(graphics_driver->framebuffer);

    if (graphics_driver_init_vga(graphics_driver, multiboot_struct))
        return true;

    if (graphics_driver->vgabuffer)
        free(graphics_driver->vgabuffer);

    return false;
}

vga_gm_color_index_t rgb_to_vga(const color_t& c) {
    // *taken from the internet

    static constexpr u8 s_palette[16][3] = {
        {0x00, 0x00, 0x00}, {0x00, 0x00, 0xAA}, {0x00, 0xAA, 0x00}, {0x00, 0xAA, 0xAA},
        {0xAA, 0x00, 0x00}, {0xAA, 0x00, 0xAA}, {0xAA, 0x55, 0x00}, {0xAA, 0xAA, 0xAA},
        {0x55, 0x55, 0x55}, {0x55, 0x55, 0xFF}, {0x55, 0xFF, 0x55}, {0x55, 0xFF, 0xFF},
        {0xFF, 0x55, 0x55}, {0xFF, 0x55, 0xFF}, {0xFF, 0xFF, 0x55}, {0xFF, 0xFF, 0xFF}
    };

    int best_dist = MAX_INT32;
    vga_gm_color_index_t best = vga_gm_color_index_t::BLACK;

    for (int i = 0; i < 16; i++) {
        int dr = int(c.r) - int(s_palette[i][0]);
        int dg = int(c.g) - int(s_palette[i][1]);
        int db = int(c.b) - int(s_palette[i][2]);
        int dist = dr*dr + dg*dg + db*db;

        if (dist < best_dist) {
            best_dist = dist;
            best = (vga_gm_color_index_t)i;
            if (dist == 0)
                break;
        }
    }
    return best;
}

bool graphics_driver_draw_pixel(graphics_driver_t* graphics_driver, size_t x, size_t y, const color_t& color) {
    switch (graphics_driver->type) {
        case graphics_driver_type_t::FRAMEBUFFER:
            return framebuffer_write_pixel(graphics_driver->framebuffer, x, y, color.r, color.g, color.b, color.a);
        case graphics_driver_type_t::VGA: 
            return vga_gm_draw::pixel(graphics_driver->vgabuffer, x, y, rgb_to_vga(color));
        default:
            return false;
    }

    return false;
}

void graphics_driver_draw_pixel_raw(graphics_driver_t* graphics_driver, size_t x, size_t y, const color_t& color) {
    if (graphics_driver->type == graphics_driver_type_t::FRAMEBUFFER)
        framebuffer_write_pixel_raw(graphics_driver->framebuffer, x, y, framebuffer_format_color(graphics_driver->framebuffer, color.r, color.g, color.b, color.a));
    else
        vga_gm_draw::pixel(graphics_driver->vgabuffer, x, y, rgb_to_vga(color));
}

bool graphics_driver_draw_lineh(graphics_driver_t* graphics_driver, size_t x, size_t y, size_t len, const color_t& color) {
        switch (graphics_driver->type) {
        case graphics_driver_type_t::FRAMEBUFFER:
            return framebuffer_write_lineh(graphics_driver->framebuffer, x, y, len, color.r, color.g, color.b, color.a);
        case graphics_driver_type_t::VGA: 
            return vga_gm_draw::lineh(graphics_driver->vgabuffer, x, y, len, rgb_to_vga(color));
        default:
            return false;
    }

    return false;
}

bool graphics_driver_draw_linev(graphics_driver_t* graphics_driver, size_t x, size_t y, size_t len, const color_t& color) {
    switch (graphics_driver->type) {
        case graphics_driver_type_t::FRAMEBUFFER:
            return framebuffer_write_linev(graphics_driver->framebuffer, x, y, len, color.r, color.g, color.b, color.a);
        case graphics_driver_type_t::VGA: 
            return vga_gm_draw::linev(graphics_driver->vgabuffer, x, y, len, rgb_to_vga(color));
        default:
            return false;
    }

    return false;
}

bool graphics_driver_draw_square(graphics_driver_t* graphics_driver, size_t x, size_t y, size_t w, size_t h, const color_t& color) {
    switch (graphics_driver->type) {
        case graphics_driver_type_t::FRAMEBUFFER:
            return framebuffer_write_square(graphics_driver->framebuffer, x, y, w, h, color.r, color.g, color.b, color.a);
        case graphics_driver_type_t::VGA: 
            return vga_gm_draw::square(graphics_driver->vgabuffer, x, y, w, h, rgb_to_vga(color));
        default:
            return false;
    }

    return false;
}

bool graphics_driver_draw_character(graphics_driver_t* graphics_driver, size_t x, size_t y, char c, const color_t& color, float scale) {
    if (!graphics_driver)
        return false;

    if (scale <= 0)
        scale = 1;

    const u8* glyph = c < 32 || c > 126 ? font8x8_fallback : font8x8[c - 32];
    
    size_t scaled_width = (size_t)(8 * scale);
    size_t scaled_height = (size_t)(8 * scale);
    
    for (size_t py = 0; py < scaled_height; py++) {
        for (size_t px = 0; px < scaled_width; px++) {
            float src_x = px / scale;
            float src_y = py / scale;
            
            size_t col = (size_t)src_x;
            size_t row = (size_t)src_y;

            if (row < 8 && col < 8) {
                if (glyph[row] & (1 << col)) {
                    graphics_driver_draw_pixel(graphics_driver, x + px, y + py, color);
                } else {
                    graphics_driver_draw_pixel(graphics_driver, x + px, y + py, { 0, 0, 0 });
                }
            }
        }
    }

    return true;
}

bool graphics_driver_draw_text(graphics_driver_t* graphics_driver, size_t x, size_t y, const char* text, const color_t& color, float scale) {
    if (!graphics_driver)
        return false;

    size_t offset = 0;
    while (*text) {
        graphics_driver_draw_character(graphics_driver, x + offset, y, *text, color, scale);
        offset += (size_t)(8 * scale);
        text++;
    }

    return true;
}

bool graphics_driver_move_square(graphics_driver_t* graphics_driver, size_t x, size_t y, size_t w, size_t h, size_t nx, size_t ny) {
    switch (graphics_driver->type) {
        case graphics_driver_type_t::FRAMEBUFFER:
            return framebuffer_move_square(graphics_driver->framebuffer, x, y, w, h, nx, ny);
        case graphics_driver_type_t::VGA: 
            return vga_gm_draw::move_square(graphics_driver->vgabuffer, x, y, w, h, nx, ny);
        default:
            return false;
    }

    return false;
}

bool graphics_driver_get_size(graphics_driver_t* graphics_driver, size_t* w, size_t* h) {
    switch (graphics_driver->type) {
        case graphics_driver_type_t::FRAMEBUFFER:
            *w = graphics_driver->framebuffer->width;
            *h = graphics_driver->framebuffer->height;
            return true;
        case graphics_driver_type_t::VGA: 
            *w = graphics_driver->vgabuffer->size.width;
            *h = graphics_driver->vgabuffer->size.height;
            return true;
        default:
            return false;
    }

    return false;
}

bool graphics_driver_render(graphics_driver_t* graphics_driver) {
    switch (graphics_driver->type) {
        case graphics_driver_type_t::FRAMEBUFFER:
            return framebuffer_render(graphics_driver->framebuffer);
        case graphics_driver_type_t::VGA:
            return vga_gm_render();
        default:
            return false;
    }

    return false;
}