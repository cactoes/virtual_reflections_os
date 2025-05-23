#include "virtual_thread.hpp"
#include "hardware_compatibility.hpp"
#include "pit_driver.hpp"

static uint64_t g_vtid_counter = 1;
static vthread_t* g_current_thread = nullptr;

bool vthread_add(vthread_t* vthread) {
    if (g_vthread_count >= VTHREAD_MAX_COUNT)
        return false;

    vthread->vt_state = vthread_state_t::RUNNING;
    pit_add_clock(vthread->vtid);
    g_vthreads[g_vthread_count++] = vthread;
    return true;
}

void vthread_entry_point(void(*entry)()) {
    entry();
    g_current_thread->vt_state = vthread_state_t::STOPPING;
    
    // catch it and wait for deletion
    while (true) {}
}

bool vthread_create(vthread_t* vthread, void(*entry)()) {
    if (g_vthread_count >= VTHREAD_MAX_COUNT)
        return false;

    uint64_t* stack = (uint64_t*)heap_alloc(get_global_heap(), VTHREAD_STACK_SIZE);
    vthread->stack_og = stack;

    if (!stack)
        return false;

    memzero(stack, VTHREAD_STACK_SIZE);

    uint64_t* sp = (uint64_t*)(((uint64_t)stack + VTHREAD_STACK_SIZE - sizeof(cpu_state_t)) & ~0xF);

    *(--sp) = (uint64_t)sp;
    *(--sp) = 0x202;
    *(--sp) = 0x8;
    // *(--sp) = (uint64_t)entry;
    *(--sp) = (uint64_t)vthread_entry_point;

    for (int i = 0; i < 12; i++)
        *(--sp) = 0;

    *(--sp) =(uint64_t)entry;
    *(--sp) = 0;

    vthread->stack = sp;
    vthread->vtid = g_vtid_counter++;
    vthread->vt_state = vthread_state_t::RUNNING;
    ((tls_base_t*)vthread->tls)->vtid = vthread->vtid;

    if (vthread_add(vthread))
        return true;

    heap_free(get_global_heap(), stack);

    return false;
}

void vthread_loop_next_thread() {
    g_current_vthread_index = (g_current_vthread_index + 1) % g_vthread_count;
    g_current_thread = g_vthreads[g_current_vthread_index];
}

cpu_state_t* vthread_schedule(cpu_state_t* stack) {
    if (g_vthread_count == 0)
        return nullptr;

    g_vthreads[g_current_vthread_index]->stack = (void*)stack;
    
    do {
        vthread_loop_next_thread();
        switch (g_current_thread->vt_state) {
            case vthread_state_t::UNKNOWN:
                break;
            case vthread_state_t::STARTING:
                break;
            case vthread_state_t::RUNNING:
                break;
            case vthread_state_t::SLEEPING: {
            //     // check if we can unsleep the thread
            //     if (vthread_get_tls()->sleep_until_tick >= hc::pit::read())
            //         g_current_thread->vt_state = vthread_state_t::RUNNING;

                break;
            }
            case vthread_state_t::STOPPING: {
                heap_free(get_global_heap(), g_current_thread->stack_og);

                // temp until we use a proper list
                g_vthreads[g_current_vthread_index] = nullptr;
                break;
            }
            default:
                break;
        }
    } while (g_current_thread->vt_state != vthread_state_t::RUNNING);

    hc::gdt_tss::set_stack_pointer0(g_current_thread->stack);

    return (cpu_state_t*)g_current_thread->stack;
}

tls_base_t* vthread_get_tls() {
    return (tls_base_t*)g_current_thread->tls;
}

// extern "C" void __get_cpu_state(void* state);
// extern "C" void __set_cpu_state(void* state);

// void vthread_yield() {
//     g_current_thread->vt_state = vthread_state_t::SLEEPING;

//     cpu_state_t state;
//     __get_cpu_state(&state);
    
//     void* new_state = vthread_schedule(&state);
    
//     hc::gdt_tss::set_stack_pointer0(new_state);
//     __set_cpu_state(new_state);
// }