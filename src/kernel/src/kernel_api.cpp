#include "kernel_api.hpp"
#include "memory/heap.hpp"
#include "time/clock.hpp"
#include "drivers/network/nidm.hpp"
#include "drivers/network/udp.hpp"

enum print_mode_t {
    STD,
    DBG
};

extern void printf(print_mode_t mode, const char* p_str, ...);

void kprint(const char* str) {
    printf(DBG, "[DRIVER] %s\n", str);
}

uint64_t ktime_since_boot() {
    return clock_get_time_since_boot();
}

void knet_udp_send(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, uint8_t* packet, size_t size) {
    udp_send(dst_ip, src_port, dst_port, packet, size);
}