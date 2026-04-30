#include "network/nidm.hpp"
#include "network/ethernet.hpp"
#include "std/ring_buffer.hpp"
#include "drivers/network/e1000.hpp"

static network_interface_controller_t* global_nic = nullptr;

// struct network_packet_t {
//     std::unique_ptr<uint8_t> data;
//     size_t size;
//     network_interface_device_t* device;
// };

// static std::ring_buffer<64, network_packet_t> global_network_packet_array {};

void set_global_nic(network_interface_controller_t* nic) {
    global_nic = nic;
}

network_interface_controller_t* get_global_nic() {
    return global_nic;
}

void network_packet_destroy(network_packet_t* packet) {
    if (packet->data)
        free(packet->data);
}

void nic_init(network_interface_controller_t* nic) {
    if (!nic)
        return;

    nic->interfaces = std::dynamic_array<std::unique_ptr<network_interface_t>>();
}

bool nic_register_interface(network_interface_controller_t* nic, std::unique_ptr<network_interface_t> interface) {
    if (!nic)
        return false;

    nic->interfaces.insert_back(move(interface));
    return true;
}

bool nic_process_packet(network_interface_controller_t* nic) {
    // swap handling of packets
    static uint64_t buffer_selector = 0;
    
    network_packet_t packet {};

    if (buffer_selector % 2 == 0) {
        if (nic->incoming_packets.get(packet)) {
            ethernet_receive(packet.interface, packet.data, packet.size);
            network_packet_destroy(&packet);
        }
    } else {
        if (nic->outgoing_packets.get(packet)) {
            switch (packet.interface->device_type) {
                case network_interface_device_type_t::E1000:
                    e1000_send_packet((e1000_t*)packet.interface->device, packet.data, packet.size);
                    break;
                default:
                    break;
            }

            network_packet_destroy(&packet);
        }
    }

    buffer_selector++;

    return true;
}

bool nic_dispatch_packet(network_interface_controller_t* nic, const network_packet_t& packet) {
    return nic->outgoing_packets.insert(packet);
}

bool nic_receive_packet(network_interface_controller_t* nic, const network_packet_t& packet) {
    return nic->incoming_packets.insert(packet);
}

int nic_thread() {
    network_interface_controller_t* nic = get_global_nic();
    while (true)
        nic_process_packet(nic);

    return 1;
}

// void nidm_shutdown(nidm_t* nidm) {
//     mutex_lock_guard guard(&nidm->mutex);

//     nidm->devices.clear();
//     nidm->udp_callbacks.clear();
// }

// int nidm_register_device(nidm_t* nidm, std::unique_ptr<network_interface_device_t> device) {
//     mutex_lock_guard guard(&nidm->mutex);

//     nidm->devices.insert_back(move(device));
//     return 0;
// }

// network_interface_device_t* nidm_get_device(nidm_t* nidm, const std::string& name) {
//     for (auto& device : nidm->devices)
//         if (device->name == name)
//             return device.get();

//     return nullptr;
// }

// network_interface_device_t* nidm_get_device_on_interface(nidm_t* nidm, const std::string& interface) {
//     for (auto& device : nidm->devices)
//         if (device->interface == interface)
//             return device.get();

//     return nullptr;
// }

// #include "utils/debug.hpp"

// int nidm_packet_recieve(nidm_t* nidm, network_interface_device_t* p_device, const void* p_data, size_t size) {
//     network_packet_t packet {};
//     packet.data = std::unique_ptr<uint8_t>((uint8_t*)malloc(size));
//     memcpy(packet.data.get(), p_data, size);
//     packet.size = size;
//     packet.device = p_device;

//     if (!global_network_packet_array.insert(move(packet))) {
//         debug_puts("dropped packet");
//         return 1;
//     }

//     return 0;
// }

// int nidm_process_packet() {
//     network_packet_t packet {};
//     if (global_network_packet_array.get(packet))
//         return ethernet_receive(packet.device, packet.data.get(), packet.size);

//     return 1;
// }

// int nidm_packet_send(nidm_t* nidm, const void* p_data, size_t size) {
//     if (auto device = nidm_get_prefered_device(nidm))
//         return device->send_packet(p_data, size);
    
//     return 1;
// }

// int nidm_udp_bind(nidm_t* nidm, uint16_t port, network_callback_t p_callback) {
//     mutex_lock_guard guard(&nidm->mutex);

//     if (nidm->udp_callbacks.contains(port))
//         return 1;

//     nidm->udp_callbacks[port] = p_callback;
//     return 0;
// }

// int nidm_udp_dispatch(nidm_t* nidm, uint16_t port, uint8_t* p_packet, size_t length) {
//     // mutex_lock_guard guard(&nidm->mutex);

//     auto it = nidm->udp_callbacks.get(port);
//     if (it == nidm->udp_callbacks.end())
//         return 1;

//     it->value(p_packet, length);
//     return 0;
// }

// network_interface_device_t* nidm_get_prefered_device(nidm_t* nidm) {
//     for (auto& device : nidm->devices)
//         if (device->is_up & device->is_prefered)
//             return device.get();

//     return nullptr;
// }