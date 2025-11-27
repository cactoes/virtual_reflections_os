#include "io.hpp"
#include "virtual_thread.hpp"
#include "utils/bitmap.hpp"
#include "drivers/vga.hpp"

static uint64_t global_io_bitmap[1];

bool write_stream(io_stream_t stream, const char* str) {
    file_descriptor_t stream_fd = FILE_DESCRIPTOR_INVALID;
    
    if (stream == io_stream_t::STD_OUT || stream == io_stream_t::STD_ERR || stream == io_stream_t::STD_WRN)
        stream_fd = __thread_tls->out_streams[ABS((int)stream) - 1];

    if (stream == io_stream_t::STD_DBG)
        stream_fd = vfs_open_file(get_global_vfs(), "/dev/dbg/stream");

    if (stream_fd == FILE_DESCRIPTOR_INVALID)
        return false;

    std::dynamic_array<uint8_t> data {};
    data.assign((const uint8_t*)str, strlen(str));
    data.insert_back('\0');
    return vfs_write_file(get_global_vfs(), stream_fd, &data);
}

void printf(const char* str, ...) {
    char buffer[IO_PRINT_BUFFER_SIZE] = { 0 };

    va_list args;
    va_start(args, str);
    sprintf(buffer, (unsigned long int)sizeof(buffer), str, args);
    va_end(args);

    write_stream(io_stream_t::STD_OUT, buffer);
}

void kprintf(const char* str, ...) {
    char buffer[IO_PRINT_BUFFER_SIZE] = { 0 };

    va_list args;
    va_start(args, str);
    sprintf(buffer, (unsigned long int)sizeof(buffer), str, args);
    va_end(args);

    if (io_flag_get(io_flag::KPRINT_BYPASS_VFS))
        debug_puts(buffer);
    else
        write_stream(io_stream_t::STD_DBG, buffer);
}

void io_flag_set(io_flag flag, bool state) {
    bitmap_set(global_io_bitmap, (size_t)flag, state);
}

bool io_flag_get(io_flag flag) {
    return bitmap_get(global_io_bitmap, (size_t)flag);
}