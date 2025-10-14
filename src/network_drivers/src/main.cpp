#define DHCP_VERSION    0
#define E1000_VERSION   0

#include "common.hpp"

/// @brief kernel api functions
extern "C" {
    // NOLINTNEXTLINE
    void kprint(const char* str);
}

extern "C" int DriverInit() {
    return 0;
}

extern "C" int DriverExit() {
    return 0;
}

/// @brief                  returns version of the feature
/// @param szFeature        name of the feature
/// @return                 feature version / capability of the feature
extern "C" uint64_t QueryCapability(const char* szFeature) {
    const uint64_t uHash = hash_fnv1a_64(szFeature);

    switch (uHash) {
        case hash_fnv1a_64("dhcp"):     return DHCP_VERSION;
        case hash_fnv1a_64("e1000"):    return E1000_VERSION;
        default:                        return (uint64_t)-1;
    }
}