//==========================================
/// @file       kernel_api.hpp
/// @brief      kernel api header for drivers
//==========================================
#pragma once

#ifndef __KERNEL_API_HPP__
#define __KERNEL_API_HPP__

#ifdef __cplusplus
extern "C" {
#endif

int kernel_test_function(const char* p_str);

int driver_init();
int driver_exit();

#ifdef __cplusplus
}
#endif

#endif // __KERNEL_API_HPP__