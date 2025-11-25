#include "subsystems/dhcp/dhcp_client_driver.hpp"
#include "drivers/driver.hpp"
#include "drivers/network/udp.hpp"

void dhcp_send_packet_wrapper(uint32_t dst_ip, uint16_t dst_port, uint16_t src_port, uint8_t* data, size_t size) {
    udp_send(dst_ip, src_port, dst_port, data, size);
}

bool subsystem_dhcp_client_driver_t::init() {
    system_driver_handle_t driver_handle = driver_manager_get_driver_handle(get_global_driver_manager(), "INetDrivers");
    driver_client_init = (decltype(DHCPClientInit)*)driver_get_function(get_global_driver_manager(), driver_handle, "DHCPClientInit");
    driver_client_shutdown = (decltype(DHCPClientShutdown)*)driver_get_function(get_global_driver_manager(), driver_handle, "DHCPClientShutdown");
    driver_client_start = (decltype(DHCPClientStart)*)driver_get_function(get_global_driver_manager(), driver_handle, "DHCPClientStart");
    driver_client_handle_packet = (decltype(DHCPClientHandlePacket)*)driver_get_function(get_global_driver_manager(), driver_handle, "DHCPClientHandlePacket");
    driver_client_lease_extend = (decltype(DHCPClientLeaseExtend)*)driver_get_function(get_global_driver_manager(), driver_handle, "DHCPClientLeaseExtend");

    return nidm_udp_bind(get_global_nidm(), DHCP_PORT_CLIENT, { this, &subsystem_dhcp_client_driver_t::network_callback }) == 0;
}

void subsystem_dhcp_client_driver_t::shutdown() {
    mutex_lock_guard guard(&mutex);

    if (state)
        driver_client_shutdown(state);
}

void subsystem_dhcp_client_driver_t::network_callback(uint8_t* packet, size_t size) {
    // mutex_lock_guard guard(&mutex);

    if (size != sizeof(DHCPPacket))
        return;

    // TODO @since 16/10/2025 -- 17:15
    // deternine when to send
    // driver_client_lease_extend(state, dhcp_send_packet);

    int result = driver_client_handle_packet(state, dhcp_send_packet_wrapper, (DHCPPacket*)packet);

    switch (result) {
        case DHCP_CLIENT_RECIEVE_ACK: {
            current_device->ip.raw = state->m_nOfferdIP;
            current_device->subnet_mask.raw = state->m_nSubnetMask;
            current_device->gateway.raw = state->m_nGateway;
            current_device->is_configured = true;
            break;
        }
        default:
            break;
    }
}

void subsystem_dhcp_client_driver_t::configure(network_interface_device_t* device) {
    mutex_lock_guard guard(&mutex);

    if (state)
        driver_client_shutdown(state);

    state = driver_client_init(hostname.c_str(), device->mac);
    current_device = device;
    driver_client_start(state, dhcp_send_packet_wrapper);
}