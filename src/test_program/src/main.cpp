#include "common.hpp"
#include "vrosapi/memory.hpp"

int main() {
    // create inter process memory space
    // register screen buffer to kernel
    // render dekstop
    // handle mouse events
    // window management
    syscall_free(syscall_malloc(sizeof(int)));
    return 0;
}