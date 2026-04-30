#include "gui/programs/webbrowser.hpp"
#include "gui/desktop.hpp"
#include "memory/heap.hpp"
#include "subsystem_interface.hpp"
#include "subsystems/dns/interface.hpp"
#include "network/tcp.hpp"
#include "io.hpp"
#include "utils/debug.hpp"

void data_handler(const uint8_t* data, size_t size) {
    char* str = (char*)malloc(size + 1);
    memzero(str, size + 1);
    memcpy(str, data, size);
    debug_puts(str);
    debug_puts("\n");
    free(str);
}

void webbrowser_init() {
    desktop_render_target_t target {};
    target.callback = webbrowser_render_target;
    target.x = 1;
    target.y = 1;
    target.w = 200;
    target.h = 150;
    target.name = "Browser";
    desktop_register_target(target);

    const auto subsys_dns_client = subsys_get<subsys_dns_client_t>(SUBSYS_DNS_CLIENT);
    uint32_t ip = subsys_dns_client->resolve("httpforever.com");

    if (ip == (uint32_t)-1) {
        kprintf("failed to resolve ip");
        return;
    }

    // auto connection = tcp_connect(ip, 80, data_handler);

    // const char* http_request = 
    //     "GET / HTTP/1.1\r\n"
    //     "Host: cactoes.xyz\r\n"
    //     "Connection: close\r\n"
    //     "User-Agent: virtual reflections e0\r\n"
    //     "\r\n";

    // tcp_send_packet((uint8_t*)http_request, strlen(http_request), TCP_FLAG_ACK | TCP_FLAG_PSH, connection);
}

void webbrowser_render_target(uint64_t dt, uint64_t x, uint64_t y) {

}