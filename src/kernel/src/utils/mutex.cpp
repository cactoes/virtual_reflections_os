#include "utils/mutex.hpp"
#include "arch/generic.hpp"
#include "virtual_thread.hpp"
#include "interrupt_manager.hpp"
#include "utils/debug.hpp"

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
    if (is_in_interrupt())
        debug_trap("lock while inside interrupt");

    if (thread_local_storage_t* tls = __thread_tls) {
        if (tls->irq_disable_depth == 0)
            tls->saved_irq_flags = save_flags_and_cli();
    
        tls->irq_disable_depth++;
    }

    while (atomic_exchange(&p_mutex->locked, 1) != 0)
        vthread_yield();

    if (thread_local_storage_t* tls = __thread_tls) {
        p_mutex->handle = tls->handle;
    }

    if (!append_to_mutex_array(global_mutex_array, p_mutex))
        debug_puts("[WARN] mutex array full\n");
}

void mutex_unlock(mutex_t* p_mutex) {
    if (thread_local_storage_t* tls = __thread_tls) {
        if (p_mutex->handle != tls->handle)
            debug_trap("wrong thread tried mutex unlock");

        p_mutex->handle = VTHREAD_HANDLE_INVALID;

        tls->irq_disable_depth--;

        if (tls->irq_disable_depth == 0)
            restore_flags(tls->saved_irq_flags);
    }

    remove_from_mutex_array(global_mutex_array, p_mutex);
    
    atomic_exchange(&p_mutex->locked, 0);
}

void mutex_clear_all_thread_references_and_release(vthread_handle_t handle) {
    for (size_t i = 0; i < MUTEX_ARRAY_SIZE; i++) {
        if (global_mutex_array[i]->handle == handle)
            mutex_unlock(global_mutex_array[i]);
    }
}