// TODO @since 28/08/2025 -- 17:23
// export e1000 driver to here

typedef unsigned long long uint64_t;

constexpr uint64_t hash_string_64(const char* p_str, uint64_t hash = 0ULL) {
    return (*p_str == '\0') ? hash :
        hash_string_64(p_str + 1, (hash << 1) + static_cast<uint64_t>(*p_str));
}

extern "C" int kernel_test_function(const char* p_str);

extern "C" {

int driver_init() {
    return 0;
}

int driver_exit() {
    return 0;
}

bool check_driver(const char* p_name) {
    const uint64_t name_hash = hash_string_64(p_name);

    switch (name_hash) {
    case hash_string_64("e1000"):
        return true;
    default:
        return false;
    }
}

}