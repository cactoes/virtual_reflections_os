#include "drivers/network/nidm.hpp"
#include "drivers/network/ethernet.hpp"

nidm_t* global_nidm = nullptr;

void set_global_nidm(nidm_t* nidm) {
    global_nidm = nidm;
}

nidm_t* get_global_nidm() {
    return global_nidm;
}

void nidm_init(nidm_t* nidm) {
    nidm->devices = std::dynamic_array<std::unique_ptr<network_interface_device_t>>();
    nidm->udp_callbacks = linear_map<uint16_t, network_callback_t>();
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

network_interface_device_t* nidm_get_device(nidm_t* nidm, const string& name) {
    for (auto& device : nidm->devices)
        if (device->name == name)
            return device.get();

    return nullptr;
}

int nidm_packet_recieve(nidm_t* nidm, network_interface_device_t* p_device, const void* p_data, size_t size) {
    return ethernet_receive(p_device, (uint8_t*)p_data, size);
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
    mutex_lock_guard guard(&nidm->mutex);

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