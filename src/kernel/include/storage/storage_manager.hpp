//==========================================
/// @file       storage_manager.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __STORAGE_MANAGER_HPP__
#define __STORAGE_MANAGER_HPP__

#include "common.hpp"
#include "drivers/storage/ide.hpp"
#include "drivers/storage/ahci.hpp"
#include "std/array.hpp"

struct storage_manager_t {
    struct {
        std::dynamic_array<ide_device_t> devices;
    } ide;

    struct {
        ahci_driver_ctx_t driver_ctx;
        std::dynamic_array<ahci_device_t> devices;
    } ahci;
};

storage_manager_t* get_global_storage_manager();
void set_global_storage_manager(storage_manager_t* storage_manager);

bool storage_manager_init(storage_manager_t* storage_manager);

#endif // __STORAGE_MANAGER_HPP__