#==========================================
## @file       Makefile
## @brief      build script (c kernel, amd64 only (x86-64))
#==========================================

# makefile config
.RECIPEPREFIX = >

# directories & files
PROJECT_PATH        := .
SRC_DIR             := $(PROJECT_PATH)/src
INC_DIR             := $(PROJECT_PATH)/include
ASM_DIR             := $(PROJECT_PATH)/asm
LINKER_SCRIPT       := $(PROJECT_PATH)/linker.ld

ROOT_PATH           := ../..
BUILD_PATH          := $(ROOT_PATH)/build
OBJ_DIR             := $(BUILD_PATH)/objects
ISO_SRC_DIR         := $(ROOT_PATH)/iso
ISO_BUILD_DIR       := $(BUILD_PATH)/iso

SHARED_DIR          := $(ROOT_PATH)/src/shared

TARGET_NAME         := VirtualReflectionsOS
KERNEL_BIN          := $(BUILD_PATH)/$(TARGET_NAME).bin
ISO_IMAGE           := $(BUILD_PATH)/$(TARGET_NAME).iso

EFI_DIR             := $(ISO_BUILD_DIR)/EFI/BOOT
EFI_FILE            := $(EFI_DIR)/BOOTX64.EFI
GRUB_CFG            := $(ISO_SRC_DIR)/boot/grub/grub.cfg

# toolchain
CC                  := x86_64-elf-gcc
AS                  := nasm
LD                  := x86_64-elf-ld
GRUB_MKRESCUE       := grub-mkrescue
GRUB_MKSTANDALONE   := grub-mkstandalone

# build time values
GIT_COMMIT_HASH     ?= $(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)
CPU_ARCHITECTURE    := 1    # 1 == amd64
KERNEL_VIRTUAL_BASE := 0xFFFFF80000000000

# flags
CPPFLAGS            := -I$(INC_DIR) \
                       -I$(SHARED_DIR)/include \
                       -DGIT_COMMIT_HASH=\"$(GIT_COMMIT_HASH)\" \
                       -DCPU_ARCHITECTURE=$(CPU_ARCHITECTURE) \
                       -DKERNEL_VIRTUAL_BASE=$(KERNEL_VIRTUAL_BASE)ull \
                       -MMD -MP

CFLAGS              := -std=gnu11 -g -m64 -mcmodel=large \
                       -ffreestanding -nostdlib \
                       -msse -msse2 -mno-red-zone \
                       -Wunused-result -Werror

ASFLAGS             := -f elf64 -DKERNEL_VIRTUAL_BASE=$(KERNEL_VIRTUAL_BASE)
LDFLAGS             := -z max-page-size=0x1000 -n -T $(LINKER_SCRIPT) \
                       --defsym KERNEL_VIRTUAL_BASE=$(KERNEL_VIRTUAL_BASE)

# sources & objects
src_files           := $(shell find $(SRC_DIR) -name '*.c')
shared_files        := $(shell find $(SHARED_DIR)/src -name '*.c' -not -path '*/vrosapi/*')
asm_files           := $(shell find $(ASM_DIR) -name '*.asm')

src_objs            := $(patsubst $(SRC_DIR)/%.c,        $(OBJ_DIR)/%.o, $(src_files))
shared_objs         := $(patsubst $(SHARED_DIR)/src/%.c, $(OBJ_DIR)/%.o, $(shared_files))
asm_objs            := $(patsubst $(ASM_DIR)/%.asm,      $(OBJ_DIR)/%.o, $(asm_files))

all_objs            := $(src_objs) $(shared_objs) $(asm_objs)

# verbosity
ifeq ($(V),1)
Q :=
else
Q := @
endif

# targets
.PHONY: build clean rebuild

build: $(ISO_IMAGE)

$(KERNEL_BIN): $(all_objs) $(LINKER_SCRIPT)
> @echo "linking kernel binary"
> @mkdir -p $(dir $@)
> $(Q)$(LD) $(LDFLAGS) -o $@ $(all_objs)

$(EFI_FILE): $(GRUB_CFG)
> @echo "creating UEFI bootloader (BOOTX64.EFI)"
> @mkdir -p $(EFI_DIR)
> $(Q)$(GRUB_MKSTANDALONE) -O x86_64-efi -o $@ "boot/grub/grub.cfg=$(GRUB_CFG)"

$(ISO_IMAGE): $(KERNEL_BIN) $(EFI_FILE)
> @echo "creating iso image"
> @mkdir -p $(ISO_BUILD_DIR)/boot
> @cp -r $(ISO_SRC_DIR)/. $(ISO_BUILD_DIR)/
> @cp $(KERNEL_BIN) $(ISO_BUILD_DIR)/boot/$(TARGET_NAME).bin
> $(Q)$(GRUB_MKRESCUE) -o $@ $(ISO_BUILD_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
> @echo "compiling: $<"
> @mkdir -p $(dir $@)
> $(Q)$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SHARED_DIR)/src/%.c
> @echo "compiling: $<"
> @mkdir -p $(dir $@)
> $(Q)$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(ASM_DIR)/%.asm
> @echo "assembling: $<"
> @mkdir -p $(dir $@)
> $(Q)$(AS) $(ASFLAGS) $< -o $@

clean:
> @echo "cleaning build directory"
> @rm -rf $(BUILD_PATH)

rebuild: clean build

# auto-generated header dependencies (from -MMD -MP)
-include $(src_objs:.o=.d) $(shared_objs:.o=.d)