#include "libtype.h"
#include "processor/cpu_direct.h"

void virtual_kernel_entry(void* multiboot2) {
    cpu_enable_processor_extensions();
    while (true);
}