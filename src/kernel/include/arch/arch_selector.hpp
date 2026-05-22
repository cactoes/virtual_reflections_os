//==========================================
/// @file       arch_selector.hpp
/// @brief      here we define what target architecture we compile for incase we havent yet
//==========================================

#pragma once

#ifndef __ARCH_SELECTOR_HPP__
#define __ARCH_SELECTOR_HPP__

#define ARCH_AMD64 1

#define CPU_ARCH_NOT_SUPPORTED "this cpu is not supported please select a supported cpu architecture"

#ifndef CPU_ARCHITECTURE
// for now
#define CPU_ARCHITECTURE ARCH_AMD64
// else we do this
// #error CPU_ARCH_NOT_SUPPORTED
#endif



#endif // __ARCH_SELECTOR_HPP__