//==========================================
/// @file       subsystem_interface.hpp
/// @brief      sub system interface manager
//==========================================

#pragma once

#ifndef __SUBSYSTEM_INTERFACE_HPP__
#define __SUBSYSTEM_INTERFACE_HPP__

/// @brief all interfaces get register here as SUBSYTEM_[X] and are callable as ISUBSYTEM_[X]
#define SUBSYSTEM_INTERFACE_LIST(x) \
    x(DHCP_CLIENT) \
    x(DNS_CLIENT)

/// @brief generator for variables inside the enum
#define SUBSYSTEM_INTERFACE_TO_ENUM_NAME(x) SUBSYSTEM_INTERFACE_##x##_INDEX

/// @brief generator for the I[X] defines that the user can call
#define GENERATE_INTERFACE_NAMES(x) \
    static constexpr uint64_t ISUBSYSTEM_##x = hash_fnv1a_64(#x);

/// @brief generator for the enum variables
#define GENERATE_INTERFACE_ENUM(x) SUBSYSTEM_INTERFACE_TO_ENUM_NAME(x),

/// @brief generator for the switch statement
#define GENERATE_CASE_STATEMENT(x) case hash_fnv1a_64(#x): return SUBSYSTEM_INTERFACE_TO_ENUM_NAME(x);

#include "common.hpp"
#include "std/pointer.hpp"

/// @brief generate all the interfaces of the interface list
SUBSYSTEM_INTERFACE_LIST(GENERATE_INTERFACE_NAMES)

/// @brief generic subsystem interface typedef
class subsystem_interface_t {
public:
    /// @brief default virtual destructor
    virtual ~subsystem_interface_t() = default;
};

/// @brief interface enum that gets automatically generated
enum subsystem_lookup_table_t {
    SUBSYSTEM_INTERFACE_LIST(GENERATE_INTERFACE_ENUM)
    SUBSYSTEM_INTERFACE_COUNT
};

/// @brief      converts the I[X] to an index in the subsystem interface array
/// @param hash I[X], aka the interface hash
/// @return     pointer to the interface or nullptr if invalid
static inline int subsystem_interface_to_index(uint64_t hash) {
    switch (hash) {
        SUBSYSTEM_INTERFACE_LIST(GENERATE_CASE_STATEMENT)
        default: return -1;
    }
}

/// @brief              sets the interface ptr to the interface
/// @param interface    the interface hash (I[X])
/// @param[in] ptr      pointer to the interface
/// @return             true if valid interface hash else false
bool subsystem_interface_set(uint64_t interface, std::unique_ptr<subsystem_interface_t> ptr);

/// @brief              gets the interface ptr by the hash
/// @param interface    the interface hash (I[X])
/// @return             ptr to the interface or nullptr if invalid
subsystem_interface_t* subsystem_interface_get(uint64_t interface);

/// @brief              templated subsystem_interface_get
template <typename I>
I* subsystem_interface_get(uint64_t interface) {
    return (I*)subsystem_interface_get(interface);
}

// undef all lol
#undef GENERATE_CASE_STATEMENT
#undef SUBSYSTEM_INTERFACE_LIST
#undef SUBSYSTEM_INTERFACE_TO_ENUM_NAME
#undef GENERATE_INTERFACE_NAMES
#undef GENERATE_INTERFACE_ENUM

#endif // __SUBSYSTEM_INTERFACE_HPP__