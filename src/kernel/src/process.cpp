#include "process.hpp"
#include "elf.hpp"
#include "memory/vmem.hpp"
#include "memory/paging.hpp"
#include "arch/generic.hpp"
#include "linker.hpp"
#include "virtual_thread.hpp"

static process_t* g_process = nullptr;

process_t* get_current_process() {
    return g_process;
}

void set_current_process(process_t* p_process) {
    g_process = p_process;
}

bool create_process(process_t* process, const char* path) {
    if (!process || !path)
        return false;

    file_descriptor_t fd = vfs_open_file(get_global_vfs(), path);
    if (fd == FILE_DESCRIPTOR_INVALID)
        return false;

    if (!vfs_read_file(get_global_vfs(), fd, &process->data, &process->data_size))
        return false;

    if (elf_check_file(process->data, elf_type_t::EXECUTABLE) != 0)
        return false;

    elf_program_section_info_t program_section_info = elf_parse_program_sections(process->data);
    
    if (program_section_info.size == 0)
        return false;

    void* base_address_v = malloc_aligned(align_up(program_section_info.size, PAGE_SIZE_LARGE), PAGE_SIZE_LARGE);
    if (!base_address_v)
        return false;

    elf_load_program_sections(process->data, (u8*)base_address_v, &program_section_info);

    auto tables = elf_get_tables(process->data);
    if (!tables.string_table || !tables.symbol_table)
        return false;

    // TODO @since 24/04/2026 -- 09:51
    // for now skip relocate sections

    process->page_table = malloc_aligned(PAGE_SIZE, PAGE_SIZE);
    memzero(process->page_table, PAGE_SIZE);
    void* new_page_table_p = vmem_virtual_to_physical(process->page_table);

    // revursive map page table
    vmem_recusive_map_page_table(process->page_table, new_page_table_p);

    // copy the kernel entries so its always linked
    u64* current_page_table_v = GET_PML4_VIRT();
    for (size_t i = KPAGING_GET_PE(KERNEL_VIRTUAL_BASE, 39); i < 511; i++)
        ((u64*)process->page_table)[i] = current_page_table_v[i];

    // TODO @since 24/04/2026 -- 09:53
    // place at a non randomly chosen location
    void* stack_user_v = (void*)(PAGE_SIZE_LARGE * 1ULL);
    void* stack_kernel_v = malloc_aligned(PAGE_SIZE_LARGE, PAGE_SIZE_LARGE);
    memzero(stack_kernel_v, PAGE_SIZE_LARGE);

    std::unique_ptr<vthread_t> p_vthread = std::make_unique<vthread_t>();
    p_vthread->stack_bottom = stack_user_v;
    p_vthread->stack_bottom_kernel = stack_kernel_v;

    p_vthread->kstack = (void*)((u64)malloc_aligned(PAGE_SIZE_LARGE, 16));

    u16 user_ds = (u16)((4 << 3) | 3); // USER_DATA_SELECTOR_INDEX
    u16 user_cs = (u16)((5 << 3) | 3); // USER_CODE_SELECTOR_INDEX

    u64* stack_top = (u64*)(((u64)stack_user_v + PAGE_SIZE_LARGE - sizeof(interrupt_regs_t)) & ~0xF);
    u64* mapped_stack_top = (u64*)(((u64)stack_kernel_v + PAGE_SIZE_LARGE - sizeof(interrupt_regs_t)) & ~0xF);

    void* entry = (void*)((elf_header_t*)process->data)->entry_point;

    // itret frame
    *(--mapped_stack_top) = (u64)user_ds;
    *(--mapped_stack_top) = (u64)stack_top;
    *(--mapped_stack_top) = 0x202;
    *(--mapped_stack_top) = (u64)user_cs;
    *(--mapped_stack_top) = (u64)entry;
    *(--mapped_stack_top) = 0;

    // general registers
    for (int i = 0; i < 13; i++)
        *(--mapped_stack_top) = 0;

    *(--mapped_stack_top) = 0;
    *(--mapped_stack_top) = 0;

    p_vthread->stack_top = (void*)mapped_stack_top;
    p_vthread->vt_state = vthread_state_t::RUNNING;
    p_vthread->handle = vhtread_next_handle();
    p_vthread->fpu_state = (u8*)malloc_aligned(sizeof(u8) * 512, 16);
    p_vthread->pml4 = new_page_table_p;
    p_vthread->tls.handle = p_vthread->handle;
    p_vthread->parent = process;

    // do this before the page table switch
    void* current_page_table_p = (void*)(current_page_table_v[511] & ~0xFFF);
    void* base_address_p = vmem_virtual_to_physical(base_address_v);
    void* user_stack_p = vmem_virtual_to_physical(stack_kernel_v);

    cli();
    set_pml4(new_page_table_p);

    for (size_t i = 0; i < program_section_info.size; i += PAGE_SIZE_LARGE) {
        if (!vmem_map_2mb(new_page_table_p, (void*)(program_section_info.min_address + i * PAGE_SIZE_LARGE), (void*)((u64)base_address_p + i * PAGE_SIZE_LARGE), true))
            return false;
    }

    set_pml4(current_page_table_p);
    sti();

    cli();
    set_pml4(new_page_table_p);
    if (!heap_init(&process->heap, new_page_table_p, (void*)PAGE_SIZE_HUGE, PAGE_SIZE_LARGE, true))
        return false;

    set_pml4(current_page_table_p);
    sti();

    cli();
    set_pml4(new_page_table_p);
    if (!vmem_map_2mb(new_page_table_p, stack_user_v, user_stack_p, true))
        return false;

    set_pml4(current_page_table_p);
    sti();

    vthread_add(move(p_vthread));

    return true;
}
