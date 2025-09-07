#include "drivers/network/nidm.hpp"
#include "utils/vector.hpp"
#include "utils/map.hpp"
#include "drivers/network/ethernet.hpp"

enum print_mode_t {
    STD,
    DBG
};

extern void printf(print_mode_t mode, const char* p_str, ...);

static dynamic_array<network_interface_device_t> g_nid_list {};
static linear_map<uint64_t, network_callback_t> g_udpnetcallback_map {};

int nidm_packet_recieve(network_interface_device_t* p_device, const void* p_data, size_t size) {
    // nidm_network_packet_t packet {};

    // packet.data = ptr::unique<uint8_t>((uint8_t*)heap_alloc(get_global_heap(), size));
    // memcpy(packet.data.get(), p_data, size);
    // packet.size = size;

    // printf(DBG, "got packet: [length=%i] [ ", size);
    // for (size_t i = 0; i < size; i++) {
    //     printf(DBG, "%c", packet.data[i]);
    // }
    // printf(DBG, "]\n");

    // TODO @since 24/08/2025 -- 18:29
    // update network stack etc
    ethernet_receive(p_device, (uint8_t*)p_data, size);

    return 0;
}

int nidm_send_data(const string& name, const void* p_data, size_t size) {
    for (auto& nid : g_nid_list) {
        if (!nid.is_up || !(nid.name == name))
            continue;

        return nidm_send_data(&nid, p_data, size);
    }

    return 1;
}

int nidm_send_data(network_interface_device_t* p_device, const void* p_data, size_t size) {
    return p_device->send_packet(p_device, p_data, size);
}

void nidm_register_device(network_interface_device_t device) {
    g_nid_list.insert_back(device);
}

network_interface_device_t* ndim_get_device(const string& name) {
    for (auto& device : g_nid_list) {
        if (device.name == name)
            return &device;
    }

    return nullptr;
}

int nidm_udp_bind(uint64_t port, network_callback_t p_callback) {
    if (port >= MAX_UINT16)
        return 1;

    if (g_udpnetcallback_map.contains(port))
        return 2;

    g_udpnetcallback_map[port] = p_callback;
    return 0;
}

int ndim_udp_send_to_handler(uint64_t port, uint8_t* p_packet, size_t length) {
    if (port >= MAX_UINT16)
        return 1;

    auto it = g_udpnetcallback_map.get(port);
    if (it == g_udpnetcallback_map.end())
        return 2;

    it->value(p_packet, length);
    return 0;
}