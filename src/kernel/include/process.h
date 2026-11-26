//==========================================
/// @file       process.h
/// @brief      
//==========================================

#pragma once

#ifndef PROCESS_H
#define PROCESS_H

#include "libtype.h"

typedef struct __process {
    u64 id;
    void* page_table;
} process_t;

#endif // PROCESS_H