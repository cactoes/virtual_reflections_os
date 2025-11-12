#include "utils/mutex.hpp"
#include "arch/generic.hpp"
#include "virtual_thread.hpp"

#define MUTEX_ARRAY_SIZE 128

// random size 500, should be good for now
// check if we can just use a dynamic array
static mutex_t* global_mutex_array[MUTEX_ARRAY_SIZE] {};

bool append_to_mutex_array(mutex_t* array[MUTEX_ARRAY_SIZE], mutex_t* mutex) {
    for (size_t i = 0; i < MUTEX_ARRAY_SIZE; i++) {
        if (array[i] == nullptr) {
            array[i] = mutex;
            return true;
        }
    }

    return false;
}

bool remove_from_mutex_array(mutex_t* array[MUTEX_ARRAY_SIZE], mutex_t* mutex) {
    for (size_t i = 0; i < MUTEX_ARRAY_SIZE; i++) {
        if (array[i] == mutex) {
            array[i] = nullptr;
            return true;
        }
    }

    return false;
}

void mutex_init(mutex_t* p_mutex) {
    p_mutex->locked = 0;
    p_mutex->saved_flags = 0;
}

void mutex_lock(mutex_t* p_mutex) {
    p_mutex->saved_flags = save_flags_and_cli();
    p_mutex->handle = vthread_get_tls()->handle;
    if (!append_to_mutex_array(global_mutex_array, p_mutex))
        debug_puts("[WARN] mutex failed to append");
    
    while (atomic_exchange(&p_mutex->locked, 1))
        pause();
}

void mutex_unlock(mutex_t* p_mutex) {
    p_mutex->handle = VTHREAD_HANDLE_INVALID;
    atomic_exchange(&p_mutex->locked, 0);
    restore_flags(p_mutex->saved_flags);

    if (!remove_from_mutex_array(global_mutex_array, p_mutex))
        debug_puts("[WARN] mutex failed to remove");
}

void mutex_clear_all_thread_references_and_release(vthread_handle_t handle) {
    for (size_t i = 0; i < MUTEX_ARRAY_SIZE; i++) {
        if (global_mutex_array[i]->handle == handle)
            mutex_unlock(global_mutex_array[i]);
    }
}