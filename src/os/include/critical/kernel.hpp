#pragma once

#ifndef __KERNEL_HPP__
#define __KERNEL_HPP__

#include "common.hpp"

#define NUM_COLS (uint64_t)80
#define NUM_ROWS (uint64_t)25

// #define VGA_PORT_COMMAND 0x3D4
// #define VGA_PORT_DATA    0x3D5

#define VGA_MISC_PORT 0x03C2
#define VGA_SEQ_INDEX 0x03C4
#define VGA_SEQ_DATA 0x03C5
#define VGA_GC_INDEX 0x03CE
#define VGA_GC_DATA 0x03CF
#define VGA_CRTC_INDEX 0x03D4
#define VGA_CRTC_DATA 0x03D5

#define KMEM_VGA_BUFFER             0x000B8000

#define KFATAL_KERNEL_ASSERTION_FAILED              0xF00000000
#define KFATAL_UNHANDLED_INTERRUPT                  0xF00000001
#define KFATAL_PAGE_TABLE_KERNEL_LIMIT_REACHED      0xF00000002
#define KFATAL_PAGE_TABLE_USER_LIMIT_REACHED        0xF00000003
#define KFATAL_PAGE_TABLE_INCORRECT_SIZE            0xF00000004
#define KFATAL_MEMORY_ALIGNMENT_INCORRECT           0xF00000005
#define KFATAL_MEMORY_OUT_OF_BOUNDS                 0xF00000006

enum vga_color_t : uint8_t {
    VGAC_BLACK = 0,
    VGAC_BLUE = 1,
    VGAC_GREEN = 2,
    VGAC_CYAN = 3,
    VGAC_RED = 4,
    VGAC_MAGENTA = 5,
    VGAC_BROWN = 6,
    VGAC_LIGHT_GRAY = 7,
    VGAC_DARK_GRAY = 8,
    VGAC_LIGHT_BLUE = 9,
    VGAC_LIGHT_GREEN = 10,
    VGAC_LIGHT_CYAN = 11,
    VGAC_LIGHT_RED = 12,
    VGAC_PINK = 13,
    VGAC_YELLOW = 14,
    VGAC_WHITE = 15,
};

void kernel_set_print_color(vga_color_t fg, vga_color_t bg);
void kernel_clear_row(uint64_t row);
void kernel_clear_screen();
void kernel_print_new_line();
void kernel_move_cursor(uint64_t row, uint64_t col);
void kernel_print(char ch);
void kernel_print(const char* string, ...);
[[noreturn]] void kernel_fatal(uint64_t code, uint64_t extra_code = 0);
void kernel_set_cursor(uint64_t y, uint64_t x);

namespace kernel {

/// @brief namespace for cpu generic asm functions
namespace cpu {

enum port_type_t : uint64_t {
    PT_B = 0,
    PT_W,
    PT_L
};

/// @brief          sends an out i/o command
/// @param type     value type / size
/// @param port     target port number
/// @param value    value that gets sent (get cast to correct type)
/// @return         KRESULT(0): success
///                 KRESULT(1): invalid port type
[[nodiscard]] kresult_t
out_port(
    port_type_t type,
    uint16_t port,
    uint32_t value);

/// @brief              sends an in i/o command
/// @param type         value type / size
/// @param port         target port number
/// @param[out] value   value gets dumped in here
///                     this value HAS to be a 32 bits or bigger
/// @return             KRESULT(0): success
///                     KRESULT(1): invalid port type
///                     KRESULT(2): value pointer was null
[[nodiscard]] kresult_t
in_port(
    port_type_t type,
    uint16_t port,
    uint32_t* value);

/// @brief disabled interrupts and halts the cpu
void
halt();

} // namespace cpu

} // namespace kernel

#endif // __KERNEL_HPP__