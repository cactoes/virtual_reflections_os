#include "utils/mutex.hpp"
#include "arch/generic.hpp"

void mutex_init(mutex_t* p_mutex) {
    p_mutex->locked = 0;
    p_mutex->saved_flags = 0;
}

void mutex_lock(mutex_t* p_mutex) {
    p_mutex->saved_flags = save_flags_and_cli();
    
    while (atomic_exchange(&p_mutex->locked, 1))
        pause();
}

void mutex_unlock(mutex_t* p_mutex) {
    atomic_exchange(&p_mutex->locked, 0);
    restore_flags(p_mutex->saved_flags);
}