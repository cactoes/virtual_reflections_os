#include "memory/pmm.h"
#include "libarray.h"

static u64 bitmap[BITMAP_WORDS(PMM_MAX_PAGES)];
static u64 free_page_count;
static u64 free_page_hint;

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
u64 find_run(u64 from, u64 to, u64 count) {
    u64 run = 0;
    u64 start = 0;

    for (u64 i = from; i < to; ) {
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

bool pmm_reserve_range(void* addr, u64 count) {
    if ((u64)addr % PMM_PAGE_SIZE)
        return false;

    u64 first = (u64)addr / PMM_PAGE_SIZE;
    if (first >= PMM_MAX_PAGES || count > PMM_MAX_PAGES - first)
        return false;

    for (u64 i = 0; i < count; i++)
        mark_used(first + i);

    return true;
}

void* pmm_alloc_pages(u64 count) {
    if (count == 0 || count > free_page_count)
        return nullptr;

    u64 found = find_run(free_page_hint, PMM_MAX_PAGES, count);
    if (found == ~0ull)
        found = find_run(0, PMM_MAX_PAGES, count);
    if (found == ~0ull)
        return 0;

    for (u64 i = 0; i < count; i++)
        mark_used(found + i);

    free_page_hint = (found + count < PMM_MAX_PAGES) ? found + count : 0;
    return (void*)(found * PMM_PAGE_SIZE);
}

void pmm_free_pages(void* addr, u64 count) {
    if ((u64)addr % PMM_PAGE_SIZE)
        return;

    u64 first = (u64)addr / PMM_PAGE_SIZE;
    if (first >= PMM_MAX_PAGES)
        return;

    if (count > PMM_MAX_PAGES - first)
        count = PMM_MAX_PAGES - first;

    for (u64 i = 0; i < count; i++)
        mark_free(first + i);
}

void pmm_init() {
    free_page_count = PMM_MAX_PAGES;
    free_page_hint = 0;

    // zero page
    pmm_reserve_range(0, 1);
}