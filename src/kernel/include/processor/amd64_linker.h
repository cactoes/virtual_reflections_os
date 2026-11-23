//==========================================
/// @file       linker.h
/// @brief      linker variables coming from the amd64 linker script
///             or defined in the .asm files
//==========================================

#ifndef LINKER_H
#define LINKER_H

#define KERNEL_VIRT_TO_PHYS(addr)   ((u64)(addr) - KERNEL_VIRTUAL_BASE)
#define KERNEL_PHYS_TO_VIRT(addr)   ((u64)(addr) + KERNEL_VIRTUAL_BASE)

#define LNK_TEXT_START              ((u64)__lnk_text_start)
#define LNK_TEXT_END                ((u64)__lnk_text_end)
#define LNK_DATA_START              ((u64)__lnk_data_start)
#define LNK_DATA_END                ((u64)__lnk_data_end)
#define LNK_BSS_START               ((u64)__lnk_bss_start)
#define LNK_BSS_END                 ((u64)__lnk_bss_end)
#define LNK_BSS_SIZE                ((u64)__lnk_bss_size)
#define LNK_KERNEL_END_VIRTUAL      ((u64)__lnk_kernel_end_virtual)
#define LNK_KERNEL_END_PHYSICAL     ((u64)__lnk_kernel_end_physical)
#define KERNEL_STACK_TOP            ((u64)stack_top)

#include "libtype.h"

extern char __lnk_text_start[];
extern char __lnk_text_end[];
extern char __lnk_data_start[];
extern char __lnk_data_end[];
extern char __lnk_bss_start[];
extern char __lnk_bss_end[];
extern char __lnk_bss_size[];
extern char __lnk_kernel_end_virtual[];
extern char __lnk_kernel_end_physical[];
extern char stack_top[];

extern u64 page_table_l4[];

extern char amd64_ap_trampoline_start[];
extern char amd64_ap_trampoline_end[];
extern u64 amd64_ap_boot_pml4[];

#endif // LINKER_H