#==========================================
## @file       Makefile
## @brief      build script (aarch64)
#==========================================

# makefile config
.RECIPEPREFIX = >

# directories & files
PROJECT_PATH                := src/kernel
SOURCE_FILES_PATH           := $(PROJECT_PATH)/src
HEADER_FILES_PATH           := $(PROJECT_PATH)/include
ASSEMBLY_FILES_PATH         := $(PROJECT_PATH)/asm
ROOT_PATH                   := .
BUILD_PATH                  := $(ROOT_PATH)/build

LINKER_SCRIPT               := $(PROJECT_PATH)/linker.ld
TARGET_NAME                 := VirtualReflectionsOS
SHARED_HEADERS              := $(ROOT_PATH)/src/shared_headers/
DRIVER_FILES                := $(wildcard $(ROOT_PATH)/build/**/*.sys)
INETDRIVERS_INCLUDES        := $(ROOT_PATH)/src/network_drivers/include

# compilers
COMPILER                    := aarch64-linux-gnu-gcc
# ASSEMBLER                   := nasm
LINKER                      := aarch64-linux-gnu-ld
OBJCOPY                     := aarch64-linux-gnu-objcopy

# compiler flags
COMPILER_FLAGS := -ffreestanding -nostdlib -O2 -Wall -Wextra -march=armv8-a -mgeneral-regs-only -I$(HEADER_FILES_PATH)
LINKER_FLAGS := -T $(LINKER_SCRIPT) -nostdlib

# # [!!!] -mno-sse2: SSE2 is disabled
# COMPILER_FLAGS              := -mcmodel=large -nostdlib -m64 -mno-red-zone -mno-sse2 -fno-rtti -fno-exceptions -Werror -Wunused-result -fno-use-cxa-atexit -ffreestanding -I$(HEADER_FILES_PATH) -I$(SHARED_HEADERS) -I$(INETDRIVERS_INCLUDES) -g
# NASM_FLAGS                  := -f elf64
# LINKER_FLAGS                := -z max-page-size=0x1000 -n -T $(LINKER_SCRIPT)

# compile time define
COMPILER_FLAGS += -DGIT_COMMIT_HASH=\"$(GIT_COMMIT_HASH)\"

# file sources & targets
source_files                := $(shell find $(SOURCE_FILES_PATH) -name "*.c")
object_files                := $(patsubst $(SOURCE_FILES_PATH)/%.c, $(BUILD_PATH)/objects/%.o, $(source_files))
asm_files                   := $(shell find $(ASSEMBLY_FILES_PATH) -name "*.S")
asm_object_files            := $(patsubst $(ASSEMBLY_FILES_PATH)/%.S, $(BUILD_PATH)/objects/%.o, $(asm_files))

# silent mode
SILENT_MODE = @

.PHONY: build clean rebuild

build: $(BUILD_PATH)/$(TARGET_NAME).bin

$(BUILD_PATH)/$(TARGET_NAME).bin: $(object_files) $(asm_object_files) $(LINKER_SCRIPT)
> @echo "linking kernel binary"
> @mkdir -p $(dir $@)
> $(LINKER) $(LINKER_FLAGS) -o $@ $(object_files) $(asm_object_files)

# $(BUILD_PATH)/$(TARGET_NAME).bin: $(object_files) $(asm_object_files) $(LINKER_SCRIPT)
# > @echo "linking kernel binary"
# > @mkdir -p $(dir $@)
# > $(SILENT_MODE)$(LINKER) $(LINKER_FLAGS) -o $@ $(object_files) $(asm_object_files)

# $(BUILD_PATH)/$(TARGET_NAME).iso: $(BUILD_PATH)/$(TARGET_NAME).bin $(DRIVER_FILES)
# > @echo "creating iso image"
# > @mkdir -p $(BUILD_PATH)/iso/boot
# > @cp -r $(ISO_PATH) $(BUILD_PATH) 2>/dev/null || true
# > @cp $< $(BUILD_PATH)/iso/boot/$(TARGET_NAME).bin
# > $(SILENT_MODE)$(GRUB_MKRESCUE) /usr/lib/grub/i386-pc -o $@ $(BUILD_PATH)/iso

$(BUILD_PATH)/objects/%.o: $(SOURCE_FILES_PATH)/%.c
> @echo "compiling: $<"
> @mkdir -p $(dir $@)
> $(SILENT_MODE)$(COMPILER) $(COMPILER_FLAGS) -c $< -o $@

# $(BUILD_PATH)/objects/%.o: $(SOURCE_FILES_PATH)/%.cpp
# > @echo "compiling: $<"
# > @mkdir -p $(dir $@)
# > $(SILENT_MODE)$(COMPILER) $(COMPILER_FLAGS) -c $< -o $@

$(BUILD_PATH)/objects/%.o: $(ASSEMBLY_FILES_PATH)/%.S
> @echo "assembling: $<"
> @mkdir -p $(dir $@)
> $(SILENT_MODE)$(COMPILER) $(COMPILER_FLAGS) -c $< -o $@

# $(BUILD_PATH)/objects/%.o: $(ASSEMBLY_FILES_PATH)/%.asm
# > @echo "assembling: $<"
# > @mkdir -p $(dir $@)
# > $(SILENT_MODE)$(ASSEMBLER) $(NASM_FLAGS) $< -o $@

clean:
> @echo "cleaning build directory"
> @rm -rf $(BUILD_PATH)/*

rebuild: clean build

# -include $(object_files:.o=.d)

# $(BUILD_PATH)/objects/%.d: $(SOURCE_FILES_PATH)/%.cpp
# > @mkdir -p $(dir $@)
# > $(SILENT_MODE)$(COMPILER) $(COMPILER_FLAGS) -MM -MT $(@:.d=.o) $< > $@