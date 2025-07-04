//==========================================
/// @file       fat32.hpp
/// @brief      fat32 file system
//==========================================

#pragma once

#ifndef __FAT32_HPP__
#define __FAT32_HPP__

#include "common.hpp"
#include "file_systems/vfs.hpp"

int fat32_drive_init(drive_t* drive, fs_t* fs);
int fat32_drive_deinit(fs_t* fs);
int fat32_read_file(fs_t*, drive_t* drive, const char* file_path, void** file_data, size_t* size);
int fat32_write_file(fs_t*, drive_t* drive, const char* file_path, void* file_data, size_t* size);

#endif // __FAT32_HPP__