#include "subsystem_interface.hpp"

/// @brief subsystem interface array
static std::unique_ptr<subsystem_interface_t> global_subsystem_interfaces[SUBSYSTEM_INTERFACE_COUNT] {};

bool subsystem_interface_set(uint64_t interface, std::unique_ptr<subsystem_interface_t> ptr) {
    const int index = subsystem_interface_to_index(interface);
    if (index == -1)
        return false;

    global_subsystem_interfaces[index] = move(ptr);
    return true;
}

subsystem_interface_t* subsystem_interface_get(uint64_t interface) {
    const int index = subsystem_interface_to_index(interface);
    if (index == -1)
        return nullptr;

    return global_subsystem_interfaces[index].get();
}