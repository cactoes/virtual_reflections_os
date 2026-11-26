#include "sync/spinlock.h"
#include "libatomic.h"
#include "processor/amd64_cpu.h"

void spinlock_lock(spinlock_t* lock) {
    while (atomic_xchg32(&lock->value, 1u) != 0) {
        while (atomic_fetch32(&lock->value) != 0)
            amd64_pause();
    }
}

void spinlock_unlock(spinlock_t* lock) {
    atomic_store32(&lock->value, 0u);
}

u64 spinlock_lock_irqsave(spinlock_t* lock) {
    u64 flags;
    __asm__ volatile(
        "pushfq\n\t"
        "popq %0\n\t"
        "cli"
        : "=r"(flags)
        :
        : "memory"
    );

    spinlock_lock(lock);
    return flags;
}

void spinlock_unlock_irqrestore(spinlock_t* lock, u64 flags) {
    spinlock_unlock(lock);

    if (flags & AMD64_RFLAGS_IF)
        __asm__ volatile("sti" ::: "memory");
}