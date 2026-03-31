#include "kernel_include/virtual_reflections_exports.hpp"
#include "memory/heap.hpp"
#include "time/clock.hpp"
#include "drivers/network/nidm.hpp"
#include "drivers/network/udp.hpp"
#include "io.hpp"
#include "virtual_thread.hpp"

void kprint(const char* str) {
    kprintf("[DRIVER] %s\n", str);
}

uint64_t ktime_since_boot() {
    return clock_get_time_since_boot();
}

void knet_udp_send(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, uint8_t* packet, size_t size) {
    udp_send(dst_ip, src_port, dst_port, packet, size);
}

void ksleep(uint64_t ms) {
    vthread_sleep(ms);
}