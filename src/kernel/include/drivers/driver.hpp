//==========================================
/// @file       driver.hpp
/// @brief      system drivers, aka custom .sys files
//==========================================

#pragma once

#ifndef __DRIVERS_DRIVER_HPP__
#define __DRIVERS_DRIVER_HPP__

#define SYSTEM_DRIVER_HANDLE_INVALID (system_driver_handle_t)-1

#include "common.hpp"
#include "string.hpp"

struct system_driver_functions_t {
    int(*driver_init)();
    int(*driver_exit)();
};

struct system_driver_t {
    string name;
    void* driver_code;
    system_driver_functions_t functions;
};

typedef uint64_t system_driver_handle_t;

system_driver_handle_t driver_load(const char* p_name, void* p_driver_file);
int driver_unload(system_driver_handle_t handle);

int driver_start(system_driver_handle_t handle);
int driver_stop(system_driver_handle_t handle);

#endif // __DRIVERS_DRIVER_HPP__