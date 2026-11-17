//==========================================
/// @file       libdef.h
/// @brief      standard defines
//==========================================

#pragma once

#ifndef LIBDEF_H
#define LIBDEF_H

#define __noreturn                       [[noreturn]]
#define __unused                         [[maybe_unused]]

#define __section_boot_text              __attribute__((section(".boot.text")))
#define __section_boot_bss               __attribute__((section(".boot.bss")))
#define __section_text                   __attribute__((section(".text")))
#define __section_bss                    __attribute__((section(".bss")))

#define __section_noinline_text          __attribute__((section(".text"), noinline))
#define __section_noinline_bss           __attribute__((section(".bss"), noinline))
#define __section_noinline_boot_text     __attribute__((section(".boot.text"), noinline))
#define __section_noinline_boot_bss      __attribute__((section(".boot.bss"), noinline))

#define __section_boot_bss_4k            __attribute__((section(".boot.bss"), aligned(4096)))

#define HAS_BIT(val, bit)           (((val) & (1ull << (bit))) != 0)
#define HAS_ALL(val, mask)          (((val) & (mask)) == (mask))
#define HAS_ANY(val, mask)      (((val) & (mask)) != 0)


#endif // LIBDEF_H