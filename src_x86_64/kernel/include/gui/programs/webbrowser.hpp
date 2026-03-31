//==========================================
/// @file       webbrowser.hpp
/// @brief      testing web browser
//==========================================

#pragma once

#ifndef __WEBBROWSER_HPP__
#define __WEBBROWSER_HPP__

#include "common.hpp"

void webbrowser_init();
void webbrowser_render_target(uint64_t dt, uint64_t x, uint64_t y);

#endif // __WEBBROWSER_HPP__