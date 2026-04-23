//==========================================
/// @file       common.hpp
/// @brief      general items that are used
///             in the entire kernel
//==========================================

#pragma once

#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#ifndef GIT_COMMIT_HASH
#define GIT_COMMIT_HASH ""
#endif

#define PAGE_SIZE           0x1000
#define PAGE_SIZE_LARGE     0x200000
#define PAGE_SIZE_HUGE      0x40000000

#define ABS(a)              (((a) < 0) ? -(a) : (a))
#define MIN(a, b)           ((a) > (b) ? (b) : (a))
#define MAX(a, b)           ((a) < (b) ? (b) : (a))
#define CLAMP(x, a, b)      (MIN(MAX((x), (a)), (b)))

#define KB(x)               ((x) * 1024UL)
#define MB(x)               (KB(x) * 1024UL)
#define GB(x)               (MB(x) * 1024UL)

#define UNUSED(x)           (void)(x)

#define NOINLINE            __attribute__((noinline))
#define ALWAYS_INLINE       __attribute__((always_inline))
#define PACKED              __attribute__((packed))
#define NAKED               __attribute__((naked))
#define ALIGNED(x)          __attribute__((aligned(x)))
#define USED                __attribute__((used))
#define NODISCARD           [[nodiscard]]
#define NORETURN            [[noreturn]]
#define UNUSED_PARAM        [[maybe_unused]]

#define BIT(n)              (1UL << (n))
#define BIT_SET(x, n)       ((x) |= BIT(n))
#define BIT_CLEAR(x, n)     ((x) &= ~BIT(n))
#define BIT_TOGGLE(x, n)    ((x) ^= BIT(n))
#define BIT_CHECK(x, n)     (((x) >> (n)) & 1U)

#define ARRAY_LENGTH(arr)   sizeof((arr)) / sizeof(decltype(*(arr)))
#define ARRAY_SIZE(arr)     sizeof((arr)) * sizeof(decltype(*(arr)))

#define MAX_UINT8           (uint8_t)-1
#define MAX_UINT16          (uint16_t)-1
#define MAX_UINT32          (uint32_t)-1
#define MAX_UINT64          (uint64_t)-1

#define MAX_INT8            ((int8_t)(MAX_UINT8 >> 1))
#define MAX_INT16           ((int16_t)(MAX_UINT16 >> 1))
#define MAX_INT32           ((int32_t)(MAX_UINT32 >> 1))
#define MAX_INT64           ((int64_t)(MAX_UINT64 >> 1))

#define TO_IP(a0, a1, a2, a3)   ((((uint32_t)(a0) & 0xff) << 24) | (((uint32_t)(a1) & 0xff) << 16) | (((uint32_t)(a2) & 0xff) << 8) | (((uint32_t)(a3) & 0xff) << 0))
#define FROM_IP(ip)             ((ip) >> 24) & 0xFF, ((ip) >> 16) & 0xFF, ((ip) >> 8)  & 0xFF, ((ip) >> 0)  & 0xFF

#define BCD_TO_BIN(bcd)         (((bcd) >> 4) * 10) + ((bcd) & 0x0F)

#define NTOHS bswap16
#define NTOHL bswap32
#define HTONS bswap16
#define HTONL bswap32

#define PI 3.14159265358979323846
#define TWO_PI (2.0 * PI)

#define RFLAGS_CF       (1 << 0)
#define RFLAGS_PF       (1 << 2)
#define RFLAGS_AF       (1 << 4)
#define RFLAGS_ZF       (1 << 6)
#define RFLAGS_SF       (1 << 7)
#define RFLAGS_TF       (1 << 8)
#define RFLAGS_IF       (1 << 9)
#define RFLAGS_DF       (1 << 10)
#define RFLAGS_OF       (1 << 11)
#define RFLAGS_IOPL     (3 << 12)
#define RFLAGS_NT       (1 << 14)
#define RFLAGS_RF       (1 << 16)
#define RFLAGS_VM       (1 << 17)
#define RFLAGS_AC       (1 << 18)
#define RFLAGS_VIF      (1 << 19)
#define RFLAGS_VIP      (1 << 20)
#define RFLAGS_ID       (1 << 21)

typedef __SIZE_TYPE__       size_t;

typedef unsigned long long  uint64_t;
typedef unsigned int        uint32_t;
typedef unsigned short      uint16_t;
typedef unsigned char       uint8_t;

typedef signed long long    int64_t;
typedef signed int          int32_t;
typedef signed short        int16_t;
typedef signed char         int8_t;

extern "C" void* memset_impl(void* dst, uint8_t val, size_t size) noexcept;
extern "C" void* memzero_impl(void* dst, size_t size) noexcept;
extern "C" void* memcpy_impl(void* dst, const void* src, size_t size) noexcept;
extern "C" bool  memeq_impl(const void* a, const void* b, size_t size) noexcept;

extern "C" void* malloc(size_t size) noexcept;
extern "C" void free(void* ptr) noexcept;

extern "C" void* malloc_aligned(size_t size, size_t align) noexcept;
extern "C" void free_aligned(void* ptr) noexcept;

void* operator new(__SIZE_TYPE__ size) noexcept;
void* operator new(__SIZE_TYPE__ size, void* p_ptr) noexcept;
void* operator new[](__SIZE_TYPE__ size) noexcept;
void* operator new[](__SIZE_TYPE__ size, void*) noexcept;

void operator delete(void* p_ptr) noexcept;
void operator delete(void* p_ptr, __SIZE_TYPE__) noexcept;
void operator delete[](void* ptr) noexcept;
void operator delete[](void* ptr, __SIZE_TYPE__) noexcept;

template<typename T>
struct remove_reference {
    using type_t = T;
};

template<typename T>
struct remove_reference<T&> {
    using type_t = T;
};

template<typename T>
struct remove_reference<T&&> {
    using type_t = T;
};

template<typename T>
constexpr typename remove_reference<T>::type_t&& move(T&& t) noexcept {
    return static_cast<typename remove_reference<T>::type_t&&>(t);
}

template<typename T>
T&& forward(typename remove_reference<T>::type_t& arg) {
    return static_cast<T&&>(arg);
}

template<typename T>
T&& forward(typename remove_reference<T>::type_t&& arg) {
    return static_cast<T&&>(arg);
}

static void* memset(void* dst, uint8_t val, size_t size) {
    return memset_impl(dst, val, size);
}

static void* memzero(void* dst, size_t size) {
    return memzero_impl(dst, size);
}

static void* memcpy(void* dst, const void* src, size_t size) {
    return memcpy_impl(dst, src, size);
}

static bool memeq(const void* a, const void* b, size_t size) {
    return memeq_impl(a, b, size);
}

static bool is_aligned(uint64_t addr, uint64_t align) {
    return (addr & (align - 1)) == 0;
}

static uint64_t align_up(uint64_t addr, uint64_t align) {
    return (addr + (align - 1)) & ~(align - 1);
}

static uint64_t align_down(uint64_t addr, uint64_t align) {
    return (addr) & ~(align - 1);
}

static double round(double x) {
    return x >= 0.0
        ? static_cast<double>(static_cast<long long>(x + 0.5))
        : static_cast<double>(static_cast<long long>(x - 0.5));
}

static double sin(double x) {
    while (x > PI)  x -= TWO_PI;
    while (x < -PI) x += TWO_PI;

    return 1.27323954 * x - 0.405284735 * x * ( x < 0 ? -x : x );
}

static double cos(double x) {
    return sin(x + PI / 2);
}

static double pow(double base, int exponent) {
    if (exponent == 0) return 1;

    bool is_negative = exponent < 0;
    if (is_negative) exponent = -exponent;

    double result = 1.0;

    while (exponent > 0) {
        if (exponent % 2 == 1) {
            result *= base;
        }
        base *= base;
        exponent /= 2;
    }

    return is_negative ? 1.0 / result : result;
}

constexpr uint16_t bswap16(uint16_t num) {
    return ((num & 0xFF) << 8) | ((num >> 8) & 0xFF);
}

constexpr uint32_t bswap32(uint32_t num) {
    return ((num & 0xFF) << 24) | (((num >> 8) & 0xFF) << 16) | (((num >> 16) & 0xFF) << 8) | ((num >> 24) & 0xFF);
}

constexpr uint64_t hash_fnv1a_64(const char* str, uint64_t hash = 14695981039346656037ULL) {
    return (*str == '\0') ? hash : 
        hash_fnv1a_64(str + 1, (hash ^ static_cast<uint64_t>(*str)) * 1099511628211ULL);
}

constexpr uint64_t hash_string_64(const char* str, uint64_t hash = 0ULL) {
    return (*str == '\0') ? hash :
        hash_string_64(str + 1, (hash << 1) + static_cast<uint64_t>(*str));
}

#endif // __COMMON_HPP__