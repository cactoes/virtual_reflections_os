//==========================================
/// @file       nic.hpp
/// @brief      network interface controller
//==========================================

#pragma once

#ifndef __DRIVERS_NETWORK_NIC_HPP__
#define __DRIVERS_NETWORK_NIC_HPP__

#include "common.hpp"
#include "utils/vector.hpp"

int nic_init();
int nic_queue_packet();
int nic_send_data();

#endif // __DRIVERS_NETWORK_NIC_HPP__