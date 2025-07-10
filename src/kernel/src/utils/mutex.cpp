#include "utils/mutex.hpp"
#include "arch/generic.hpp"

void mutex_init(mutex_t* p_mutex) {
    p_mutex->locked = 0;
}

void mutex_lock(mutex_t* p_mutex) {
    while (atomic_exchange(&p_mutex->locked, 1))
        pause();
}

void mutex_unlock(mutex_t* p_mutex) {
    memory();
    p_mutex->locked = 0;
}