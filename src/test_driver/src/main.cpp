// #include "kernel_api.hpp"

extern "C" int kernel_test_function(const char* p_str);

extern "C" {

int driver_init() {
    // kernel_test_function("yuhhh!");
    return 0;
}

int driver_exit() {
    return 0;
}

}