//==========================================
/// @file       driver.hpp
/// @brief      system drivers, aka custom .sys files
//==========================================

#pragma once

#ifndef __DRIVERS_DRIVER_HPP__
#define __DRIVERS_DRIVER_HPP__

#define SYSTEM_DRIVER_HANDLE_INVALID (system_driver_handle_t)-1

#include "common.hpp"
#include "std/string.hpp"
#include "std/map.hpp"
#include "std/pointer.hpp"

struct system_driver_functions_t {
    int(*driver_init)();
    int(*driver_exit)();
};

struct system_driver_t {
    std::string name;
    void* base_address;
    void* file_data_ptr;
    system_driver_functions_t functions;
};

typedef u64 system_driver_handle_t;
typedef u64(*driver_query_capability_t)(const char*);

struct driver_manager_t {
    system_driver_handle_t current_handle;
    std::linear_map<system_driver_handle_t, std::unique_ptr<system_driver_t>> loaded_drivers;
};

void set_global_driver_manager(driver_manager_t* driver_manager);
driver_manager_t* get_global_driver_manager();

system_driver_handle_t driver_manager_get_driver_handle(driver_manager_t* driver_manager, const char* driver_name);

system_driver_handle_t driver_load(driver_manager_t* driver_manager, const char* p_name, void* p_driver_file);
int driver_unload(driver_manager_t* driver_manager, system_driver_handle_t handle);

int driver_start(driver_manager_t* driver_manager, system_driver_handle_t handle);
int driver_stop(driver_manager_t* driver_manager, system_driver_handle_t handle);

void* driver_get_function(driver_manager_t* driver_manager, system_driver_handle_t handle, const char* p_name);

u64 driver_query_capability(driver_manager_t* driver_manager, system_driver_handle_t handle, const char* feature);

#endif // __DRIVERS_DRIVER_HPP__