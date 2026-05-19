//==========================================
/// @file       webbrowser.hpp
/// @brief      testing web browser
//==========================================

#pragma once

#ifndef __WEBBROWSER_HPP__
#define __WEBBROWSER_HPP__

#include "common.hpp"

void webbrowser_init();
void webbrowser_render_target(u64 dt, u64 x, u64 y);

#endif // __WEBBROWSER_HPP__