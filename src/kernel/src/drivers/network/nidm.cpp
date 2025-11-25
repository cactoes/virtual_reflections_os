#include "drivers/network/nidm.hpp"
#include "drivers/network/ethernet.hpp"

#define PACKET_QUEUE_SIZE 128

static nidm_t* global_nidm = nullptr;

struct network_packet_t {
    std::unique_ptr<uint8_t> data;
    size_t size;
    network_interface_device_t* device;
};

static network_packet_t global_network_packet_array[PACKET_QUEUE_SIZE];
static uint32_t global_npa_head;
static uint32_t global_npa_tail;

void set_global_nidm(nidm_t* nidm) {
    global_nidm = nidm;
}

nidm_t* get_global_nidm() {
    return global_nidm;
}

void nidm_init(nidm_t* nidm) {
    nidm->devices = std::dynamic_array<std::unique_ptr<network_interface_device_t>>();
    nidm->udp_callbacks = std::linear_map<uint16_t, network_callback_t>();
    mutex_init(&nidm->mutex);
}

void nidm_shutdown(nidm_t* nidm) {
    mutex_lock_guard guard(&nidm->mutex);

    nidm->devices.clear();
    nidm->udp_callbacks.clear();
}

int nidm_register_device(nidm_t* nidm, std::unique_ptr<network_interface_device_t> device) {
    mutex_lock_guard guard(&nidm->mutex);

    nidm->devices.insert_back(move(device));
    return 0;
}

network_interface_device_t* nidm_get_device(nidm_t* nidm, const std::string& name) {
    for (auto& device : nidm->devices)
        if (device->name == name)
            return device.get();

    return nullptr;
}

network_interface_device_t* nidm_get_device_on_interface(nidm_t* nidm, const std::string& interface) {
    for (auto& device : nidm->devices)
        if (device->interface == interface)
            return device.get();

    return nullptr;
}

#include "utils/debug.hpp"

int nidm_packet_recieve(nidm_t* nidm, network_interface_device_t* p_device, const void* p_data, size_t size) {
    uint32_t next = (global_npa_head + 1) % PACKET_QUEUE_SIZE;

    if (next == global_npa_tail) {
        debug_puts("dropped packet");
        return 1;
    }

    network_packet_t packet {};
    packet.data = std::unique_ptr<uint8_t>((uint8_t*)malloc(size));
    memcpy(packet.data.get(), p_data, size);
    packet.size = size;
    packet.device = p_device;
    global_network_packet_array[global_npa_head] = move(packet);

    global_npa_head = next;

    return 0;
}

int nidm_process_packet() {
    if (global_npa_head == global_npa_tail)
        return 1;

    network_packet_t packet = move(global_network_packet_array[global_npa_tail]);
    auto result = ethernet_receive(packet.device, packet.data.get(), packet.size);
    packet.data.release();

    global_npa_tail = (global_npa_tail + 1) % PACKET_QUEUE_SIZE;

    return result;
}

int nidm_packet_send(nidm_t* nidm, const void* p_data, size_t size) {
    if (auto device = nidm_get_prefered_device(nidm))
        return device->send_packet(p_data, size);
    
    return 1;
}

int nidm_udp_bind(nidm_t* nidm, uint16_t port, network_callback_t p_callback) {
    mutex_lock_guard guard(&nidm->mutex);

    if (nidm->udp_callbacks.contains(port))
        return 1;

    nidm->udp_callbacks[port] = p_callback;
    return 0;
}

int nidm_udp_dispatch(nidm_t* nidm, uint16_t port, uint8_t* p_packet, size_t length) {
    // mutex_lock_guard guard(&nidm->mutex);

    auto it = nidm->udp_callbacks.get(port);
    if (it == nidm->udp_callbacks.end())
        return 1;

    it->value(p_packet, length);
    return 0;
}

network_interface_device_t* nidm_get_prefered_device(nidm_t* nidm) {
    for (auto& device : nidm->devices)
        if (device->is_up & device->is_prefered)
            return device.get();

    return nullptr;
}