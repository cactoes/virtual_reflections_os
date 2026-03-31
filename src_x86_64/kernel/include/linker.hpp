//==========================================
/// @file       linker.hpp
/// @brief      all linker variables wrapped in defines
//==========================================

#pragma once

#ifndef __LINKER_HPP__
#define __LINKER_HPP__

#include "common.hpp"

// NOLINTBEGIN
namespace linker_variables {

extern "C" uint64_t __lnk_end_kernel_phys;

} // namespace __
// NOLINTEND

/// @brief variable placed at the end of the kernels physical address
#define LINKER_END_KERNEL_PHYS ((uint64_t)&linker_variables::__lnk_end_kernel_phys)

// not exported by linker, but it is defined there
#define KERNEL_VIRTUAL_BASE 0xFFFFF80000000000ull

// translate physical address, to kernel identity mapped address
#define PTOV_I(paddr) (((uint64_t)paddr) + KERNEL_VIRTUAL_BASE)

#endif // __LINKER_HPP__