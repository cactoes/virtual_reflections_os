#include "network/nidm.hpp"
#include "network/ethernet.hpp"
#include "std/ring_buffer.hpp"
#include "drivers/network/e1000.hpp"
#include "drivers/network/rtl8168.hpp"

static network_interface_controller_t* global_nic = nullptr;

// struct network_packet_t {
//     std::unique_ptr<u8> data;
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

    nic->interfaces = std::dynamic_array<network_interface_t*>();
}

bool nic_register_interface(network_interface_controller_t* nic, network_interface_t* interface) {
    if (!nic || !interface)
        return false;

    if (!nic_get_default_interface(nic))
        interface->is_prefered = true;

    nic->interfaces.insert_back(interface);

    return true;
}

bool nic_process_packet(network_interface_controller_t* nic) {
    // swap handling of packets
    static u64 buffer_selector = 0;
    
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
                case network_interface_device_type_t::RTL8168:
                    rtl8168_send_packet((rtl8168_t*)packet.interface->device, packet.data, packet.size);
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

bool is_same_subnet(u32 target_ip, u32 our_ip, u32 subnet_mask) {
    return (target_ip & subnet_mask) == (our_ip & subnet_mask);
}

network_interface_t* route_lookup(network_interface_controller_t* nic, u32 dst_ip) {
    if (nic->interfaces.length() == 0)
        return nullptr;

    for (auto& interface : nic->interfaces) {
        if (is_same_subnet(dst_ip, interface->ip.raw, interface->subnet_mask.raw)) {
            return interface;
        }
    }

    for (auto& interface : nic->interfaces) {
        if (interface->is_prefered)
            return interface;
    }

    return *nic->interfaces.get_at(0);
}

network_interface_t* nic_get_default_interface(network_interface_controller_t* nic) {
    if (nic->interfaces.length() == 0)
        return nullptr;

    for (auto& interface : nic->interfaces)
        if (interface->is_prefered)
            return interface;

    return *nic->interfaces.get_at(0);
}

network_interface_t* nic_get_interface_from_device(network_interface_controller_t* nic, void* device) {
    for (auto& interface : nic->interfaces)
        if (interface->device == device)
            return interface;

    return nullptr;
}

int nic_thread() {
    network_interface_controller_t* nic = get_global_nic();
    while (true) {
        if (!nic)
            continue;

        nic_process_packet(nic);
    }

    return 1;
}

// void nidm_shutdown(nidm_t* nidm) {
//     mutex_lock_guard guard(&nidm->mutex);

//     nidm->devices.clear();
//     nidm->udp_callbacks.clear();
// }