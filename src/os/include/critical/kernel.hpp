#pragma once

#ifndef __KERNEL_HPP__
#define __KERNEL_HPP__

#include "common.hpp"
#include "driver/vga.hpp"

#define KFATAL_KERNEL_ASSERTION_FAILED              0xF00000000
#define KFATAL_UNHANDLED_INTERRUPT                  0xF00000001
#define KFATAL_PAGE_TABLE_KERNEL_LIMIT_REACHED      0xF00000002
#define KFATAL_PAGE_TABLE_USER_LIMIT_REACHED        0xF00000003
#define KFATAL_PAGE_TABLE_INCORRECT_SIZE            0xF00000004
#define KFATAL_MEMORY_ALIGNMENT_INCORRECT           0xF00000005
#define KFATAL_MEMORY_OUT_OF_BOUNDS                 0xF00000006

[[noreturn]] void kernel_fatal(uint64_t code, uint64_t extra_code = 0);

namespace kernel {

/// @brief namespace for kernel printing functions
namespace print {

/// @brief          prints string to the given kernel stdout
/// @param fmt      string with format chars to print
/// @param va_args  argument list
void
print(
    const char* fmt,
    ...);

/// @brief              prints string to the given kernel stdout
/// @param[in] color    print color for this text after it resets to before
/// @param fmt          string with format chars to print
/// @param va_args      argument list
void
print(
    const driver::vga::tm::vga_color_map_t* color,
    const char* fmt,
    ...);

/// @brief clears the stdio screen
void
clear_screen();

/// @brief      clears the text on the current row & resets the cursor
/// @param row  row index to clear
void
clear_row(
    uint32_t row);

/// @brief              sets the text color to the given color
/// @param[in] color    pointer to color struct
void
set_color(
    const driver::vga::tm::vga_color_map_t* color);

/// @brief          sets the cursor to destination
/// @param x        x pos
/// @param y        y pos
void
set_cusor(
    uint32_t x,
    uint32_t y);

} // namespace print

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

/// @brief disabled interrupts & halts the cpu
void
halt();

} // namespace cpu

} // namespace kernel

#endif // __KERNEL_HPP__