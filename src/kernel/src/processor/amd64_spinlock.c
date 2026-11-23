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