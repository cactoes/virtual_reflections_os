#include "processor/cpu_direct.h"


#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
#include "processor/amd64_cpu.h"
#else
#error "undefined cpu architecture";
#endif

u64 cpu_get_processor_extensions() {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    return amd64_get_processor_extensions();
#else
    return 0;
#endif
}

void cpu_enable_processor_extensions(u64 extensions) {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    amd64_enable_processor_extensions(extensions);
#else
#endif
}