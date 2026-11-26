#include "memory/pmm.h"
#include "libmap.h"
#include "sync/spinlock.h"

static u64 bitmap[BITMAP_WORDS(PMM_MAX_PAGES)];
static u64 free_page_count;
static u64 max_page_count;
static u64 free_page_hint;
static spinlock_t pmm_lock = { 0 };

static
void mark_used(u64 i) {
    if (!bitmap_get(bitmap, i)) {
        bitmap_set(bitmap, i, true);
        free_page_count--;
    }
}

static
void mark_free(u64 i) {
    if (i == 0)
        return;

    if (bitmap_get(bitmap, i)) {
        bitmap_set(bitmap, i, false);
        free_page_count++;
    }
}

static
u64 find_run(u64 from, u64 to, u64 count, u64 align_pages) {
    u64 run = 0;
    u64 start = 0;

    for (u64 i = from; i < to; ) {
        if (run == 0 && (i & (align_pages - 1))) {
            i = (i + align_pages - 1) & ~(align_pages - 1);
            continue;
        }

        if ((i % 64) == 0 && bitmap[i / 64] == ~0ull) {
            run = 0; i += 64;
            continue;
        }

        if (bitmap_get(bitmap, i)) {
            run = 0;
        } else {
            if (run == 0)
                start = i;
            if (++run == count)
                return start;
        }

        i++;
    }

    return ~0ull;
}

static
void* alloc_run(u64 count, u64 align_pages) {
    if (count == 0 || count > free_page_count)
        return nullptr;

    u64 found = find_run(free_page_hint, max_page_count, count, align_pages);
    if (found == ~0ull)
        found = find_run(0, max_page_count, count, align_pages);
    if (found == ~0ull)
        return nullptr;

    for (u64 i = 0; i < count; i++)
        mark_used(found + i);

    free_page_hint = (found + count < max_page_count) ? found + count : 0;
    return (void*)(found * PMM_PAGE_SIZE);
}

bool pmm_reserve_range(void* addr, u64 count) {
    if ((u64)addr % PMM_PAGE_SIZE)
        return false;

    spinlock_lock(&pmm_lock);

    u64 first = (u64)addr / PMM_PAGE_SIZE;
    if (first >= max_page_count) {
        spinlock_unlock(&pmm_lock);
        return true;
    }

    if (count > max_page_count - first)
        count = max_page_count - first;

    for (u64 i = 0; i < count; i++)
        mark_used(first + i);

    spinlock_unlock(&pmm_lock);
    return true;
}

void* pmm_alloc_pages(u64 count) {
    spinlock_lock(&pmm_lock);
    void* result = alloc_run(count, 1);
    spinlock_unlock(&pmm_lock);
    return result;
}

void* pmm_alloc_pages_aligned(u64 count, u64 alignment) {
    if (alignment == 0 || (alignment & (alignment - 1)) != 0)
        return nullptr;

    if (alignment < PMM_PAGE_SIZE)
        alignment = PMM_PAGE_SIZE;

    spinlock_lock(&pmm_lock);
    void* result = alloc_run(count, alignment / PMM_PAGE_SIZE);
    spinlock_unlock(&pmm_lock);
    return result;
}

void pmm_free_pages(void* addr, u64 count) {
    if ((u64)addr % PMM_PAGE_SIZE)
        return;

    spinlock_lock(&pmm_lock);

    u64 first = (u64)addr / PMM_PAGE_SIZE;
    if (first >= max_page_count) {
        spinlock_unlock(&pmm_lock);
        return;
    }

    if (count > max_page_count - first)
        count = max_page_count - first;

    for (u64 i = 0; i < count; i++)
        mark_free(first + i);

    spinlock_unlock(&pmm_lock);
}

void pmm_init(u64 max_physical_pages) {
    spinlock_lock(&pmm_lock);

    max_page_count = (max_physical_pages < PMM_MAX_PAGES) ? max_physical_pages : PMM_MAX_PAGES;
    free_page_count = max_page_count;
    free_page_hint = 0;

    // zero page
    mark_used(0);

    spinlock_unlock(&pmm_lock);
}