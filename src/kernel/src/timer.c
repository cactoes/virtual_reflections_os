#include "timer.h"
#include "processor/cpu_direct.h"

#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
#include "processor/amd64_apic.h"
#endif

u64 get_local_timer_ticks() {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    return amd64_lapic_timer_get_ticks();
#endif
}