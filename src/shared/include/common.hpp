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

#define __ignore(x)           (void)(x)

#define __noinline          __attribute__((noinline))
#define __force_inline      __attribute__((always_inline))
#define __packed            __attribute__((packed))
#define __naked             __attribute__((naked))
#define __aligned(x)        __attribute__((aligned(x)))
#define __disable_sse       __attribute__((target("no-sse,no-sse2")))
#define __nodiscard         [[nodiscard]]
#define __noreturn          [[noreturn]]
#define __unused            [[maybe_unused]]

#define BIT(n)              (1UL << (n))
#define BIT_SET(x, n)       ((x) |= BIT(n))
#define BIT_CLEAR(x, n)     ((x) &= ~BIT(n))
#define BIT_TOGGLE(x, n)    ((x) ^= BIT(n))
#define BIT_CHECK(x, n)     (((x) >> (n)) & 1U)

#define ARRAY_LENGTH(arr)   sizeof((arr)) / sizeof(decltype(*(arr)))
#define ARRAY_SIZE(arr)     sizeof((arr)) * sizeof(decltype(*(arr)))

#define MAX_UINT8           (u8)-1
#define MAX_UINT16          (u16)-1
#define MAX_UINT32          (u32)-1
#define MAX_UINT64          (u64)-1

#define MAX_INT8            ((i8)(MAX_UINT8 >> 1))
#define MAX_INT16           ((i16)(MAX_UINT16 >> 1))
#define MAX_INT32           ((i32)(MAX_UINT32 >> 1))
#define MAX_INT64           ((i64)(MAX_UINT64 >> 1))

#define TO_IP(a0, a1, a2, a3)   ((((u32)(a0) & 0xff) << 24) | (((u32)(a1) & 0xff) << 16) | (((u32)(a2) & 0xff) << 8) | (((u32)(a3) & 0xff) << 0))
#define FROM_IP(ip)             ((ip) >> 24) & 0xFF, ((ip) >> 16) & 0xFF, ((ip) >> 8)  & 0xFF, ((ip) >> 0)  & 0xFF

#define BCD_TO_BIN(bcd)         (((bcd) >> 4) * 10) + ((bcd) & 0x0F)

#define NTOHS bswap16
#define NTOHL bswap32
#define HTONS bswap16
#define HTONL bswap32

#define PI 3.14159265358979323846
#define TWO_PI (2.0 * PI)

// ? this needs to move to amd64 header
#define RFLAGS_CF       (1 << 0)
#define RFLAGS_RES      (1 << 1)
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

typedef unsigned long long  u64;
typedef unsigned int        u32;
typedef unsigned short      u16;
typedef unsigned char       u8;

typedef signed long long    i64;
typedef signed int          i32;
typedef signed short        i16;
typedef signed char         i8;

extern "C" void* amd64_memset_impl(void* dst, u8 val, u64 size) noexcept;
extern "C" void* amd64_memcpy_impl(void* dst, const void* src, u64 size) noexcept;

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

static inline void* memset(void* dst, u8 val, u64 size) {
    return amd64_memset_impl(dst, val, size);
}

static inline void* memzero(void* dst, u64 size) {
    return amd64_memset_impl(dst, 0, size);
}

static inline void* memcpy(void* dst, const void* src, u64 size) {
    return amd64_memcpy_impl(dst, src, size);
}

static inline bool memeq(const void* a, const void* b, u64 size) noexcept {
    for (size_t i = 0; i < size; i++) {
        if (((u8*)a)[i] != ((u8*)b)[i])
            return false;
    }

    return true;
}

static inline
bool is_aligned(u64 addr, u64 align) {
    return (addr & (align - 1)) == 0;
}

static inline
u64 align_up(u64 addr, u64 align) {
    return (addr + (align - 1)) & ~(align - 1);
}

static inline
u64 align_down(u64 addr, u64 align) {
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

constexpr u16 bswap16(u16 num) {
    return ((num & 0xFF) << 8) | ((num >> 8) & 0xFF);
}

constexpr u32 bswap32(u32 num) {
    return ((num & 0xFF) << 24) | (((num >> 8) & 0xFF) << 16) | (((num >> 16) & 0xFF) << 8) | ((num >> 24) & 0xFF);
}

constexpr u64 hash_fnv1a_64(const char* str, u64 hash = 14695981039346656037ULL) {
    return (*str == '\0') ? hash : 
        hash_fnv1a_64(str + 1, (hash ^ (u64)(*str)) * 1099511628211ULL);
}

constexpr u64 hash_string_64(const char* str, u64 hash = 0ULL) {
    return (*str == '\0') ? hash :
        hash_string_64(str + 1, (hash << 1) + (u64)(*str));
}

#endif // __COMMON_HPP__