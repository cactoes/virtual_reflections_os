//==========================================
/// @file       amd64_tlb.h
/// @brief      
//==========================================

#pragma once

#ifndef AMD64_TLB_H
#define AMD64_TLB_H

#define TLB_ACK_POLL_LIMIT      100000000u
#define TLB_CR4_PGE             (1ull << 7)
#define TLB_CR4_PCIDE           (1ull << 17)

#include "libtype.h"

static
void amd64_flush_tlb(const void* vaddr) {
    __asm__ volatile(
        "invlpg (%0)"
        :
        : "r"(vaddr)
        : "memory"
    );
}

void amd64_tlb_init();
u32 amd64_tlb_invalidate_page(const void* address);
u32 amd64_tlb_flush_all();
void amd64_tlb_handle_ipi();

#endif // AMD64_TLB_H