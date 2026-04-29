#include "subsystems/dns/dns_client_driver.hpp"
#include "drivers/driver.hpp"
#include "network/udp.hpp"
#include "std/random.hpp"
#include "virtual_thread.hpp"

int dns_send_packet_wrapper(uint32_t dst_ip, uint16_t dst_port, uint16_t src_port, uint8_t* data, size_t size) {
    return udp_send(dst_ip, src_port, dst_port, data, size);
}

bool subsys_dns_client_driver_t::init() {
    system_driver_handle_t driver_handle = driver_manager_get_driver_handle(get_global_driver_manager(), "INetDrivers");
    driver_client_init = (decltype(DNSClientInit)*)driver_get_function(get_global_driver_manager(), driver_handle, "DNSClientInit");
    driver_client_shutdown = (decltype(DNSClientShutdown)*)driver_get_function(get_global_driver_manager(), driver_handle, "DNSClientShutdown");
    driver_client_resolve = (decltype(DNSClientResolve)*)driver_get_function(get_global_driver_manager(), driver_handle, "DNSClientResolve");
    driver_client_get_record = (decltype(DNSClientGetRecord)*)driver_get_function(get_global_driver_manager(), driver_handle, "DNSClientGetRecord");
    driver_client_handle_packet = (decltype(DNSClientHandlePacket)*)driver_get_function(get_global_driver_manager(), driver_handle, "DNSClientHandlePacket");

    state = driver_client_init(random_number(49152, 65535), TO_IP(8, 8, 8, 8));

    if (nidm_udp_bind(get_global_nidm(), state->m_nPort, { this, &subsys_dns_client_driver_t::network_callback }) != 0)
        return false;

    state->m_bIsConfigured = true;

    return true;
}

void subsys_dns_client_driver_t::shutdown() {
    mutex_lock_guard guard(&mutex);

    if (state)
        driver_client_shutdown(state);
}

uint32_t subsys_dns_client_driver_t::resolve(const char* hostname) {
    // mutex_lock_guard guard(&mutex);
    
    if (auto record_ptr = driver_client_get_record(state, hostname))
        return record_ptr->m_nIP;

    return driver_client_resolve(state, hostname, dns_send_packet_wrapper, 1000);
}

bool subsys_dns_client_driver_t::is_configured() {
    return state->m_bIsConfigured;
}

uint32_t subsys_dns_client_driver_t::get_dns_server() {
    return state->m_nDNSServerIP;
}

void subsys_dns_client_driver_t::network_callback(uint8_t* packet, size_t size) {
    driver_client_handle_packet(state, packet, size);
}