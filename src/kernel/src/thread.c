#include "thread.h"
#include "memory/vmm.h"
#include "libmem.h"
#include "processor/cpu_direct.h"
#include "memory/generic.h"
#include "sync/spinlock.h"
#include "io/debug.h"

#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
#include "processor/amd64_interrupt.h"
#endif

static vthread_queue_t ready_queue;
static spinlock_t scheduler_lock;
static bool scheduler_initialized;

static vthread_t* current_vthread[MAX_CPUS];
static vthread_t* idle_vthread[MAX_CPUS];
static bool scheduler_cpu_initialized[MAX_CPUS];

static
void vthread_queue_push(vthread_queue_t* queue, vthread_t* thread) {
    thread->next = nullptr;

    if (queue->tail)
        queue->tail->next = thread;
    else
        queue->head = thread;

    queue->tail = thread;
}

static
vthread_t* vthread_queue_pop(vthread_queue_t* queue) {
    vthread_t* thread = queue->head;
    if (!thread)
        return nullptr;

    queue->head = thread->next;
    if (!queue->head)
        queue->tail = nullptr;

    thread->next = nullptr;
    return thread;
}

static
bool is_idle_thread_locked(const vthread_t* thread) {
    for (u32 i = 0; i < MAX_CPUS; i++) {
        if (scheduler_cpu_initialized[i] && idle_vthread[i] == thread)
            return true;
    }

    return false;
}

bool vthread_scheduler_init() {
    ready_queue.head = nullptr;
    ready_queue.tail = nullptr;
    scheduler_lock.value = 0;

    for (u32 i = 0; i < MAX_CPUS; i++) {
        current_vthread[i] = nullptr;
        idle_vthread[i] = nullptr;
        scheduler_cpu_initialized[i] = false;
    }

    scheduler_initialized = true;
    return true;
}

bool vthread_enqueue(vthread_t* thread) {
    if (!scheduler_initialized)
        return false;

    const u64 flags = spinlock_lock_irqsave(&scheduler_lock);
    bool queued = false;

    if (!is_idle_thread_locked(thread) &&
        thread->state != VTHREAD_READY &&
        thread->state != VTHREAD_RUNNING &&
        thread->state != VTHREAD_ZOMBIE) {
        thread->state = VTHREAD_READY;
        vthread_queue_push(&ready_queue, thread);
        queued = true;
    }

    spinlock_unlock_irqrestore(&scheduler_lock, flags);
    return queued;
}

bool vthread_scheduler_init_local(vthread_t* initial_thread, vthread_t* idle_thread) {
    cpu_t* cpu = get_current_cpu();
 
    if (!scheduler_initialized)
        return false;

    const u32 cpu_index = cpu->id;
    const u64 flags = spinlock_lock_irqsave(&scheduler_lock);

    if (scheduler_cpu_initialized[cpu_index]) {
        spinlock_unlock_irqrestore(&scheduler_lock, flags);
        return false;
    }

    initial_thread->context = nullptr;
    initial_thread->next = nullptr;
    initial_thread->state = VTHREAD_RUNNING;

    idle_thread->next = nullptr;
    idle_thread->state = VTHREAD_READY;

    current_vthread[cpu_index] = initial_thread;
    idle_vthread[cpu_index] = idle_thread;
    scheduler_cpu_initialized[cpu_index] = true;

    spinlock_unlock_irqrestore(&scheduler_lock, flags);
    return true;
}

void* vthread_schedule(void* frame) {
    if (!scheduler_initialized)
        return frame;

    cpu_t* cpu = get_current_cpu();
    const u32 cpu_index = cpu->id;
    if (!scheduler_cpu_initialized[cpu_index])
        return frame;

    const u64 flags = spinlock_lock_irqsave(&scheduler_lock);

    vthread_t* current = current_vthread[cpu_index];
    vthread_t* idle = idle_vthread[cpu_index];
    if (!current || !idle) {
        spinlock_unlock_irqrestore(&scheduler_lock, flags);
        return frame;
    }

    current->context = frame;

    if (current == idle) {
        if (current->state == VTHREAD_RUNNING)
            current->state = VTHREAD_READY;
    } else if (current->state == VTHREAD_RUNNING) {
        current->state = VTHREAD_READY;
        vthread_queue_push(&ready_queue, current);
    }

    vthread_t* next = vthread_queue_pop(&ready_queue);
    if (!next)
        next = idle;

    next->state = VTHREAD_RUNNING;
    current_vthread[cpu_index] = next;

    void* next_context = next->context;
    spinlock_unlock_irqrestore(&scheduler_lock, flags);
    return next_context;
}

bool vthread_create(process_t* process, void* entry, vthread_t* vthread) {
    void* stack_base = kalloc_aligned(VTHREAD_STACK_SIZE, 16);
    if (!stack_base)
        return false;

    memzero(stack_base, VTHREAD_STACK_SIZE);

    u64* top = (u64*)((u64)stack_base + VTHREAD_STACK_SIZE);

#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    u64* sp = amd64_insert_kernel_trap_frame(top, entry);
#else
    u64* sp = top;
#endif

    memzero(vthread, sizeof(vthread_t));
    vthread->handle = (vthread_handle_t)vthread;
    vthread->state = VTHREAD_UNKNOWN;
    vthread->process = process;
    vthread->entry = entry;
    vthread->context = sp;
    vthread->stack_base = stack_base;
    vthread->stack_top = (void*)((u64)stack_base + VTHREAD_STACK_SIZE);

    return true;
}