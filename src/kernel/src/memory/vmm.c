#include "memory/vmm.h"
#include "processor/cpu_direct.h"

#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
#include "processor/amd64_vmm.h"
#else
#error "undefined cpu architecture";
#endif

bool vmm_map_1gb(const void* vaddr, const void* paddr, u64 flags) {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    bool use_nx = amd64_vmm_get_use_nx();
    u64 converted_flags = amd64_vmm_flags_to_pte(flags, VMEM_PAGE_SIZE_HUGE, use_nx);
    return amd64_vmm_install_virtual_address(vaddr, paddr, VMEM_PAGE_SIZE_HUGE, converted_flags);
#else
#endif
    return false;
}

bool vmm_map_2mb(const void* vaddr, const void* paddr, u64 flags) {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    bool use_nx = amd64_vmm_get_use_nx();
    u64 converted_flags = amd64_vmm_flags_to_pte(flags, VMEM_PAGE_SIZE_LARGE, use_nx);
    return amd64_vmm_install_virtual_address(vaddr, paddr, VMEM_PAGE_SIZE_LARGE, converted_flags);
#else
#endif
    return false;
}

bool vmm_map_4kb(const void* vaddr, const void* paddr, u64 flags) {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    bool use_nx = amd64_vmm_get_use_nx();
    u64 converted_flags = amd64_vmm_flags_to_pte(flags, VMEM_PAGE_SIZE_SMALL, use_nx);
    return amd64_vmm_install_virtual_address(vaddr, paddr, VMEM_PAGE_SIZE_SMALL, converted_flags);
#else
#endif
    return false;
}

bool vmm_unmap_1gb(const void* vaddr, void* old_paddr) {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    return amd64_vmm_remove_mapping(vaddr, VMEM_PAGE_SIZE_HUGE, old_paddr);
#else
#endif
    return false;
}

bool vmm_unmap_2mb(const void* vaddr, void* old_paddr) {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    return amd64_vmm_remove_mapping(vaddr, VMEM_PAGE_SIZE_LARGE, old_paddr);
#else
#endif
    return false;
}

bool vmm_unmap_4kb(const void* vaddr, void* old_paddr) {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    return amd64_vmm_remove_mapping(vaddr, VMEM_PAGE_SIZE_SMALL, old_paddr);
#else
#endif
    return false;
}