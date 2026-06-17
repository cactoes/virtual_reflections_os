#include "virtual_thread.hpp"
#include "std/map.hpp"
#include "utils/mutex.hpp"

#include "crash_handler.hpp"
#include "time/clock.hpp"
#include "std/string.hpp"
#include "cpu.hpp"
#include "linker.hpp"
#include "utils/debug.hpp"
#include "memory/heap.hpp"
#include "io.hpp"
#include "arch/amd64/vthread.hpp"
#include "arch/amd64/idt.hpp"
#include "process.hpp"

// TODO @since 23/10/2025 -- 19:06
// change into 1 "bigger" thread handler

static std::linear_map<vthread_handle_t, std::unique_ptr<vthread_t>> g_threads {};

static vthread_handle_t     global_vthread_handle_counter = 1;
static vthread_t*           global_current_thread = nullptr;
static volatile bool        global_is_in_critical_section = false;
static volatile spinlock_t  global_thread_lock {};

/// @brief                      cleans thread resources 
/// @param[inout] thread        pointer to thread context
static inline
void vthread_cleanup(vthread_t* thread) {
    // first clean platform based resources
#if CPU_ARCHITECTURE == ARCH_AMD64
    amd64_vthread_cleanup(thread);
#else
#error CPU_ARCH_NOT_SUPPORTED
#endif

    if (thread->stack_bottom_kernel)
        free_aligned(thread->stack_bottom_kernel);
    else if (thread->stack_bottom)
        free_aligned(thread->stack_bottom);

    if (thread->kstack)
        free_aligned(thread->kstack);

    // TODO @since 09/05/2026 -- 20:56
    // properly free parent process
    if (thread->parent)
        free(thread->parent);
}

/// @brief                  inits main thread context
/// @param[inout] thread    pointer to thread context
/// @return                 success status
static inline
bool vthread_init_main_thread(vthread_t* thread) {
    thread->handle = VTHREAD_MAIN_THREAD_HANDLE;
    thread->tls.handle = VTHREAD_MAIN_THREAD_HANDLE;
    thread->is_critical = true;

    thread->parent = get_current_process();

    const char name[] = "_ZN7kthread4mainEPv";
    memcpy(thread->name, name, sizeof(name));

#if CPU_ARCHITECTURE == ARCH_AMD64
    return amd64_vthread_init_main_thread(thread);
#else
#error CPU_ARCH_NOT_SUPPORTED
#endif
}

/// @brief                      inits a thread context
/// @param[inout] thread        pointer to thread context
/// @param[in] thread_entry     pointer to thread entry function
/// @return                     success status
static inline
bool vthread_init(vthread_t* thread, void* thread_entry) {
    const vthread_handle_t new_handle = vhtread_next_handle();

    thread->handle = new_handle;
    thread->tls.handle = new_handle;
    thread->vt_state = vthread_state_t::RUNNING;
    thread->parent = get_current_process();

#if CPU_ARCHITECTURE == ARCH_AMD64
    return amd64_vthread_init(thread, thread_entry);
#else
#error CPU_ARCH_NOT_SUPPORTED
#endif
}

/// @brief                  stores the current context in thread context
/// @param[inout] thread    pointer to thread context
/// @param[in] stack        pointer to last stack
static inline
void vthread_store_context(vthread_t* thread, void* stack) {
#if CPU_ARCHITECTURE == ARCH_AMD64
    amd64_vthread_store_context(thread, stack);
#else
#error CPU_ARCH_NOT_SUPPORTED
#endif
}

/// @brief                  loads the thread context into current context
/// @param[inout] thread    pointer to thread context
void vthread_load_context(vthread_t* thread) {
#if CPU_ARCHITECTURE == ARCH_AMD64
    amd64_vthread_load_context(thread);
#else
#error CPU_ARCH_NOT_SUPPORTED
#endif
}

void vthread_entry_point(thread_entry_t p_thread_entry) {
    global_current_thread->vt_state = vthread_state_t::STARTING;
    while (global_current_thread->vt_state == vthread_state_t::STARTING)
        vthread_yield();

    global_current_thread->exit_code = p_thread_entry();
    global_current_thread->vt_state = vthread_state_t::STOPPING;

    // catch & wait for deletion
    while (true);
}

vthread_t* vthread_get_next_thead(vthread_handle_t handle) {
    auto current_thread_it = g_threads.get(handle);
    if (current_thread_it == g_threads.end() || current_thread_it.advance() == g_threads.end())
        current_thread_it = g_threads.begin();
    
    return current_thread_it->value.get();
}

bool vthread_handle_sleeping(vthread_t* p_vthread) {
    if (p_vthread->sleep_until_ms <= clock_get_time_since_boot()) {
        p_vthread->vt_state = vthread_state_t::RUNNING;
        return true;
    }

    return false;
}

void vthread_handle_stopping(vthread_t* p_vthread) {
    if (p_vthread->is_critical) {
        char buffer[256];
        (void)sprintf(buffer, sizeof(buffer), "critical thread died! (%s)", p_vthread->name ? p_vthread->name : "unknown");
        kernel_fatal(KERNEL_FATAL_CRITICAL_THREAD_DIED, buffer);
    }

    spinlock_lock((spinlock_t*)&global_thread_lock);

    vthread_cleanup(p_vthread);

    g_threads.remove(p_vthread->handle);

    spinlock_unlock((spinlock_t*)&global_thread_lock);
}

void vthread_handle_starting(vthread_t* p_vthread) {
    // for now just promote to running
    // we dont yet need to do sht
    p_vthread->vt_state = vthread_state_t::RUNNING;
}

bool vthread_add(std::unique_ptr<vthread_t> p_vthread) {
    p_vthread->vt_state = vthread_state_t::RUNNING;

    spinlock_lock((spinlock_t*)&global_thread_lock);

    if (!g_threads.insert(p_vthread->handle, move(p_vthread))) {
        spinlock_unlock((spinlock_t*)&global_thread_lock);
        return false;
    }

    spinlock_unlock((spinlock_t*)&global_thread_lock);

    return true;
}

vthread_handle_t vthread_start_and_setup_main() {
    if (global_current_thread)
        return false;

    std::unique_ptr<vthread_t> p_vthread = std::make_unique<vthread_t>();

    vthread_init_main_thread(p_vthread.get());

    global_current_thread = p_vthread.get();

    return vthread_add(move(p_vthread)) ? VTHREAD_MAIN_THREAD_HANDLE : VTHREAD_HANDLE_INVALID;
}

vthread_handle_t vthread_create(thread_entry_t p_thread_entry, const char name[VTHREAD_MAX_NAME_SIZE]) {
    // TODO @since 22/05/2026 -- 18:25
    // remove this leaking smart pointer
    std::unique_ptr<vthread_t> p_vthread = std::make_unique<vthread_t>();
    auto pp_vthread = p_vthread.get();

    if (!vthread_init(p_vthread.get(), (void*)p_thread_entry)) {
        vthread_cleanup(p_vthread.get());
        return VTHREAD_HANDLE_INVALID;
    }

    if (name) {
        const size_t name_length = strlen(name);
        memzero(p_vthread->name, VTHREAD_MAX_NAME_SIZE + 1);
        memcpy(p_vthread->name, name, MIN(name_length, VTHREAD_MAX_NAME_SIZE));
    }

    if (vthread_add(move(p_vthread)))
        return pp_vthread->handle;

    vthread_cleanup(pp_vthread);

    return VTHREAD_HANDLE_INVALID;
}


bool vthread_check_stack(vthread_t* thread) {
    // stak has overflown
    if ((u64)thread->stack_top < (u64)thread->stack_bottom)
        return false;

    // thread stack is in deadzone, to prevent overflow we terminate it
    if ((u64)thread->stack_top - (u64)thread->stack_bottom < VTHREAD_STACK_DEADZONE)
        return false;

    // stack is still safe :)
    return true;
}

void* vthread_schedule(void* stack) {
    // BUG @since 22/05/2026 -- 20:01
    // there is an extremely rare case where this will not find any new thread in too long
    // im not sure if this is an issue or if we need to do some watchdog stuff here

    if (!global_current_thread)
        return stack;

    if (g_threads.size() <= 1)
        return stack;

    vthread_store_context(global_current_thread, stack);

    vthread_t* next_thread = vthread_get_next_thead(global_current_thread->handle);

    while (next_thread && next_thread->vt_state != vthread_state_t::RUNNING) {
        vthread_handle_t handle = next_thread->handle;

        switch (next_thread->vt_state) {
            case vthread_state_t::STARTING:
                vthread_handle_starting(next_thread);
                break;
            case vthread_state_t::SLEEPING:
                if (vthread_handle_sleeping(next_thread))
                    goto jumpout;
                break;
            case vthread_state_t::STOPPING:
                vthread_handle_stopping(next_thread);
                handle = VTHREAD_HANDLE_INVALID;
                break;
            case vthread_state_t::RUNNING:
                if (!vthread_check_stack(next_thread))
                    kernel_fatal(KERNEL_FATAL_VTHREAD_STACK_PROTECTION, "thread stack protection triggerd");
                break;
            case vthread_state_t::UNKNOWN:
            default:
                break;
        }

        next_thread = vthread_get_next_thead(handle);
    }

    jumpout:

    set_current_process(next_thread->parent);
    vthread_load_context(next_thread);

    global_current_thread = next_thread;
    return global_current_thread->stack_top;
}

void* vthread_handle_interrupt(void* stack, void*) {
    return vthread_schedule(stack);
}

void vthread_yield() {
#if CPU_ARCHITECTURE == ARCH_AMD64
    amd64_call_scheduler_interrupt();
#else
#error CPU_ARCH_NOT_SUPPORTED
#endif
}

thread_local_storage_t* vthread_get_tls() {
    return global_current_thread ? &global_current_thread->tls : nullptr;
}

void vthread_sleep(u64 time_ms) {
    if (global_current_thread->handle == VTHREAD_MAIN_THREAD_HANDLE) {
        u64 sleep_until_ms = clock_get_time_since_boot() + time_ms;
        while (clock_get_time_since_boot() < sleep_until_ms)
            vthread_yield();

        return;
    }

    // the rest can just yield until ready
    global_current_thread->sleep_until_ms = clock_get_time_since_boot() + time_ms;
    global_current_thread->vt_state = vthread_state_t::SLEEPING;
    vthread_yield();
}

int vthread_wait_for_close(vthread_handle_t handle) {
    while (g_threads.contains(handle));
    return 0;
}

size_t vthread_get_count() {
    return g_threads.size();
}

void vthread_terminate(vthread_handle_t handle) {
    auto current_thread_it = g_threads.get(handle);
    if (current_thread_it == g_threads.end())
        return;

    current_thread_it->value->vt_state = vthread_state_t::STOPPING;
}

void vthread_terminate() {
    global_current_thread->vt_state = vthread_state_t::STOPPING;
    vthread_yield();
}

bool vthread_is_closed(vthread_handle_t handle) {
    auto current_thread_it = g_threads.get(handle);
    if (current_thread_it == g_threads.end())
        return true;

    return current_thread_it->value->vt_state == vthread_state_t::STOPPING;
}

vthread_t* vthread_get(vthread_handle_t handle) {
    auto current_thread_it = g_threads.get(handle);
    if (current_thread_it == g_threads.end())
        return nullptr;

    return current_thread_it->value.get();
}

bool vthread_set_critical(vthread_handle_t handle, bool state) {
    auto current_thread_it = g_threads.get(handle);
    if (current_thread_it == g_threads.end())
        return false;

    current_thread_it->value->is_critical = state;

    return true;
}

vthread_state_t vthread_get_state() {
    if (!global_current_thread)
        return vthread_state_t::UNKNOWN;

    return global_current_thread->vt_state;
}

vthread_handle_t vhtread_next_handle() {
    return global_vthread_handle_counter++;
}