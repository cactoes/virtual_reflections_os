//==========================================
/// @file       minesweeper.hpp
/// @brief      minesweeper game for the desktop renderer
//==========================================

#pragma once

#ifndef __MINESWEEPER_HPP__
#define __MINESWEEPER_HPP__

#include "common.hpp"

void minesweeper_init();
void minesweeper_render_target(uint64_t dt, uint64_t x, uint64_t y);

#endif // __MINESWEEPER_HPP__