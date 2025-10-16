#include "drivers/network/dhcp.hpp"
#include "drivers/network/udp.hpp"

dhcp_context_t* global_dhcp_context = nullptr;

dhcp_context_t* dhcp_context_create() {
    return (dhcp_context_t*)malloc(sizeof(dhcp_context_t));
}

void dhcp_context_destroy(dhcp_context_t* dhcp_context) {
    free(dhcp_context);
}

void set_global_dhcp_context(dhcp_context_t* dhcp_context) {
    global_dhcp_context = dhcp_context;
}

dhcp_context_t* get_global_dhcp_context() {
    return global_dhcp_context;
}

void dhcp_send_packet(uint32_t dst_ip, uint16_t dst_port, uint16_t src_port, uint8_t* data, size_t size) {
    udp_send(dst_ip, src_port, dst_port, data, size);
}

void dhcp_net_callback(uint8_t* packet, size_t size) {
    if (size != sizeof(DHCPPacket))
        return;

    dhcp_context_t* ctx = get_global_dhcp_context();
    auto dhcp_client_handle_packet_fn = (decltype(DHCPClientHandlePacket)*)driver_get_function(get_global_driver_manager(), ctx->driver_handle, "DHCPClientHandlePacket");
    // TODO @since 16/10/2025 -- 17:15
    // deternine when to send
    // auto dhcp_client_lease_extend_fn = (decltype(DHCPClientLeaseExtend)*)driver_get_function(get_global_driver_manager(), ctx->driver_handle, "DHCPClientLeaseExtend");
    // dhcp_client_lease_extend_fn(ctx->state, dhcp_send_packet);

    int result = dhcp_client_handle_packet_fn(ctx->state, dhcp_send_packet, (DHCPPacket*)packet);

    switch (result) {
        case DHCP_CLIENT_RECIEVE_ACK: {
            auto device = nidm_get_prefered_device(get_global_nidm());
            device->ip.raw = ctx->state->m_nOfferdIP;
            device->subnet_mask.raw = ctx->state->m_nSubnetMask;
            device->gateway.raw = ctx->state->m_nGateway;
            break;
        }
        default:
            break;
    }
}

void dhcp_client_start(dhcp_context_t* dhcp_context, network_interface_device_t* device) {
    system_driver_handle_t driver_handle = driver_manager_get_driver_handle(get_global_driver_manager(), "INetDrivers");
    auto dhcp_client_init_fn = (decltype(DHCPClientInit)*)driver_get_function(get_global_driver_manager(), driver_handle, "DHCPClientInit");
    auto dhcp_client_start_fn = (decltype(DHCPClientStart)*)driver_get_function(get_global_driver_manager(), driver_handle, "DHCPClientStart");

    dhcp_context->driver_handle = driver_handle;
    // TODO @since 15/10/2025 -- 14:32
    // setup hostname
    dhcp_context->state = dhcp_client_init_fn("hostname", device->mac);
    set_global_dhcp_context(dhcp_context);

    nidm_udp_bind(get_global_nidm(), DHCP_PORT_CLIENT, dhcp_net_callback);
    dhcp_client_start_fn(dhcp_context->state, dhcp_send_packet);
}