//==========================================
/// @file       display_driver.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __DISPLAY_DRIVER_HPP__
#define __DISPLAY_DRIVER_HPP__

#include "common.hpp"

void dd_set_active_buffer(void* buffer);
bool dd_buffer_render_loop();

#endif // __DISPLAY_DRIVER_HPP__