; Copied to physical 0x8000 by the BSP
%define AP_TRAMPOLINE_PHYS 0x8000
%define AP_DATA_OFFSET     0x300
%define PHYS(label) (AP_TRAMPOLINE_PHYS + (label - amd64_ap_trampoline_start))

%define DATA_BOOT_CR3   0
%define DATA_KERNEL_CR3 8
%define DATA_STACK_TOP  16
%define DATA_CPU        24
%define DATA_ENTRY      32
%define DATA_EXTENSIONS 40
%define DATA_PAT        48

CODE32 equ 0x08
DATA   equ 0x10
CODE64 equ 0x18

CR0_PE equ (1 << 0)
CR0_MP equ (1 << 1)
CR0_EM equ (1 << 2)
CR0_TS equ (1 << 3)
CR0_NW equ (1 << 29)
CR0_CD equ (1 << 30)
CR0_PG equ (1 << 31)
CR4_PAE equ (1 << 5)

MSR_EFER equ 0xc0000080
MSR_PAT  equ 0x277
EFER_LME equ (1 << 8)
EFER_NXE equ (1 << 11)
EXT_NX   equ (1 << 1)

section .rodata
align 16
bits 16
    global amd64_ap_trampoline_start
    global amd64_ap_trampoline_end

amd64_ap_trampoline_start:
    cli
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax

    lgdt [PHYS(ap_gdtr32)]
    mov eax, cr0
    or eax, CR0_PE
    mov cr0, eax
    jmp dword CODE32:PHYS(ap_protected32)

bits 32
ap_protected32:
    mov ax, DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    mov eax, cr4
    or eax, CR4_PAE
    mov cr4, eax

    mov eax, [PHYS(ap_boot_data) + DATA_BOOT_CR3]
    mov cr3, eax

    mov ecx, MSR_PAT
    mov eax, [PHYS(ap_boot_data) + DATA_PAT]
    mov edx, [PHYS(ap_boot_data) + DATA_PAT + 4]
    wrmsr

    mov ecx, MSR_EFER
    rdmsr
    or eax, EFER_LME
    test dword [PHYS(ap_boot_data) + DATA_EXTENSIONS], EXT_NX
    jz .no_nx
    or eax, EFER_NXE
.no_nx:
    wrmsr

    wbinvd
    mov eax, cr0
    and eax, ~(CR0_EM | CR0_TS | CR0_NW | CR0_CD)
    or eax, CR0_PE | CR0_MP | CR0_PG
    mov cr0, eax
    jmp CODE64:PHYS(ap_long64)

bits 64
ap_long64:
    mov rdx, [rel ap_boot_data + DATA_KERNEL_CR3]
    mov r8,  [rel ap_boot_data + DATA_STACK_TOP]
    mov rdi, [rel ap_boot_data + DATA_CPU]
    mov rax, [rel ap_boot_data + DATA_ENTRY]
    mov rsi, [rel ap_boot_data + DATA_EXTENSIONS]

    lgdt [rel ap_gdtr64]

    mov rbx, KERNEL_VIRTUAL_BASE + PHYS(ap_higher64)
    jmp rbx

ap_higher64:
    mov cr3, rdx
    mov rsp, r8
    and rsp, -16
    xor ebp, ebp

    call rax

.park:
    cli
    hlt
    jmp .park

align 8
ap_gdt:
    dq 0
    dq 0x00cf9a000000ffff ; 32-bit code
    dq 0x00cf92000000ffff ; data
    dq 0x00af9a000000ffff ; 64-bit code
ap_gdt_end:

ap_gdtr32:
    dw ap_gdt_end - ap_gdt - 1
    dd PHYS(ap_gdt)

ap_gdtr64:
    dw ap_gdt_end - ap_gdt - 1
    dq KERNEL_VIRTUAL_BASE + PHYS(ap_gdt)

times AP_DATA_OFFSET - ($ - amd64_ap_trampoline_start) db 0
ap_boot_data:
    times 7 dq 0
amd64_ap_trampoline_end:

section .boot.bss
align 4096
global amd64_ap_boot_pml4
amd64_ap_boot_pml4:
    resb 4096