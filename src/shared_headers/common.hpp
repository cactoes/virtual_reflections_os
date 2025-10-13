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

typedef long unsigned int size_t;

typedef unsigned long long uint64_t;
typedef          long long int64_t;

typedef unsigned int uint32_t;
typedef          int int32_t;

typedef unsigned short uint16_t;
typedef          short int16_t;

typedef unsigned char uint8_t;
typedef   signed char int8_t;

extern "C" void* x86_64_memset(void* p_dest, uint8_t val, size_t size);
static void* memset(void* p_dest, uint8_t val, size_t size) {
    return x86_64_memset(p_dest, val, size);
}

static void* memzero(void* p_dest, size_t size) {
    return memset(p_dest, 0, size);
}

extern "C" void* x86_64_memcpy(void* p_dest, const void* p_src, size_t size);
static void* memcpy(void* p_dest, const void* p_src, size_t size) {
    return x86_64_memcpy(p_dest, p_src, size);
}

static bool memeq(const void* p_a1, const void* p_a2, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (((uint8_t*)p_a1)[i] != ((uint8_t*)p_a2)[i])
            return false;
    }

    return true;
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

constexpr uint16_t ntohs(uint16_t netshort) {
    return ((netshort & 0xFF) << 8) | ((netshort >> 8) & 0xFF);
}

constexpr uint16_t htons(uint16_t hostshort) {
    return ntohs(hostshort);
}

constexpr uint32_t ntohl(uint32_t netlong) {
    return ((netlong & 0xFF) << 24) | 
           (((netlong >> 8) & 0xFF) << 16) |
           (((netlong >> 16) & 0xFF) << 8) |
           ((netlong >> 24) & 0xFF);
}

constexpr uint32_t htonl(uint32_t hostlong) {
    return ntohl(hostlong);
}

template <typename T>
constexpr T net_to_host(T n);

template <>
constexpr uint16_t net_to_host(uint16_t n) {
    return ntohs(n);
}

template <>
constexpr uint32_t net_to_host(uint32_t n) {
    return ntohl(n);
}

template <typename T>
constexpr T host_to_net(T n);

template <>
constexpr uint16_t host_to_net(uint16_t n) {
    return htons(n);
}

template <>
constexpr uint32_t host_to_net(uint32_t n) {
    return htonl(n);
}

// NOLINTNEXTLINE
constexpr uint64_t hash_fnv1a_64(const char* p_str, uint64_t hash = 14695981039346656037ULL) {
    return (*p_str == '\0') ? hash :
        // NOLINTNEXTLINE
        hash_fnv1a_64(p_str + 1, (hash ^ static_cast<uint64_t>(*p_str)) * 1099511628211ULL);
}

constexpr uint64_t hash_string_64(const char* p_str, uint64_t hash = 0ULL) {
    return (*p_str == '\0') ? hash :
        hash_string_64(p_str + 1, (hash << 1) + static_cast<uint64_t>(*p_str));
}

// FIXME @since 14/07/2025 -- 01:11
// this struct is still in incorrect order
struct cpu_state_t {
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

    uint64_t rax;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;

    uint64_t error_code;

    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    // uint64_t rsp;
    // uint64_t ss;
} PACKED;

#endif // __COMMON_HPP__