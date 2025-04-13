#include "mutex.hpp"

static inline int atomic_exchange(volatile int* ptr, int value) {
    int old;
    asm volatile (
        "lock xchg %0, %1"
        : "=r"(old), "+m"(*ptr)
        : "0"(value)
        : "memory"
    );
    return old;
}

void mutex_init(mutex_t* mutex) {
    mutex->locked = 0;
}

void mutex_lock(mutex_t* mutex) {
    while (atomic_exchange(&mutex->locked, 1))
        asm volatile("pause");
}

void mutex_unlock(mutex_t* mutex) {
    asm volatile("" ::: "memory");
    mutex->locked = 0;
}