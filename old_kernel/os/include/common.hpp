//==========================================
/// @file       common.hpp
/// @brief      general items that are used
///             in the entire kernel
//==========================================

#pragma once

#ifndef __COMMON_HPP____
#define __COMMON_HPP____

namespace signed_number_types {

typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;

} // namespace signed_number_types
using namespace signed_number_types;

namespace unsigned_number_types {

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

typedef unsigned long long  size_t;
} // namespace unsigned_number_types
using namespace unsigned_number_types;

#define KRESULT(code)               (kresult_t)((uint64_t)(code))
#define KRESULT_OK                  (KRESULT(0))
#define KRESULT_IS_ERROR(code)      (KRESULT(code) != KRESULT_OK)
#define KRESULT_IS_OK(code)         (!KRESULT_IS_ERROR(code))

namespace custom_types {

/// @brief used for critical kernel functions
typedef uint64_t kresult_t;

} // namespace custom_types
using namespace custom_types;

#endif // __COMMON_HPP____