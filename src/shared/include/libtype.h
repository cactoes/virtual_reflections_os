//==========================================
/// @file       libtype.h
/// @brief      standard type defines
//==========================================

#pragma once

#ifndef LIBTYPE_H
#define LIBTYPE_H

typedef unsigned long long  u64;
typedef unsigned int        u32;
typedef unsigned short      u16;
typedef unsigned char       u8;

typedef signed long long    i64;
typedef signed int          i32;
typedef signed short        i16;
typedef signed char         i8;

typedef _Bool bool;
#define true 1
#define false 0

#define nullptr ((void*)0)

#endif // LIBTYPE_H