#include "processor/cpu_direct.h"


#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
#include "processor/amd64_cpu.h"
#else
#error "undefined cpu architecture";
#endif

void cpu_enable_processor_extensions() {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    amd64_enable_processor_extensions();
#else
#endif
}