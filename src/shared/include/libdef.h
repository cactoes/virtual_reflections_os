//==========================================
/// @file       libdef.h
/// @brief      standard defines
//==========================================

#pragma once

#ifndef LIBDEF_H
#define LIBDEF_H

#define __noreturn                      [[noreturn]]
#define __unused                        [[maybe_unused]]

#define __section_boot_text             __attribute__((section(".boot.text")))
#define __section_boot_bss              __attribute__((section(".boot.bss")))
#define __section_text                  __attribute__((section(".text")))
#define __section_bss                   __attribute__((section(".bss")))

#define __section_noinline_text         __attribute__((section(".text"), noinline))
#define __section_noinline_bss          __attribute__((section(".bss"), noinline))
#define __section_noinline_boot_text    __attribute__((section(".boot.text"), noinline))
#define __section_noinline_boot_bss     __attribute__((section(".boot.bss"), noinline))

#define __section_boot_bss_4k           __attribute__((section(".boot.bss"), aligned(4096)))
#define __align_packed(n)               __attribute__((aligned(n), packed))
#define __packed                        __attribute__((packed))
#define __naked_cleanup                 __attribute__((naked, noinline))

#define HAS_BIT(val, bit)           (((val) & (1ull << (bit))) != 0)
#define HAS_ALL(val, mask)          (((val) & (mask)) == (mask))
#define HAS_ANY(val, mask)          (((val) & (mask)) != 0)

#define BIT(n)                      (1ull << (n))
#define SET_BIT(val, bit)           ((val) |= BIT(bit))
#define CLEAR_BIT(val, bit)         ((val) &= ~BIT(bit))
#define TOGGLE_BIT(val, bit)        ((val) ^= BIT(bit))
#define ASSIGN_BIT(val, bit, on)    ((on) ? SET_BIT(val, bit) : CLEAR_BIT(val, bit))

#define HAS_NONE(val, mask)         (((val) & (mask)) == 0)
#define SET_MASK(val, mask)         ((val) |= (mask))
#define CLEAR_MASK(val, mask)       ((val) &= ~(mask))
#define TOGGLE_MASK(val, mask)      ((val) ^= (mask))
#define KEEP_MASK(val, mask)        ((val) & (mask))

#endif // LIBDEF_H