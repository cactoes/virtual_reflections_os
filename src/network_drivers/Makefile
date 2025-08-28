#==========================================
## @file       Makefile
## @brief      build script for the network drivers
#==========================================

# makefile config
.RECIPEPREFIX = >

# directories & files
TARGET_NAME                 := INetDrivers
PROJECT_PATH                := .
SOURCE_FILES_PATH           := $(PROJECT_PATH)/src
HEADER_FILES_PATH           := $(PROJECT_PATH)/include
ASSEMBLY_FILES_PATH         := $(PROJECT_PATH)/asm
ROOT_PATH                   := ../..
BUILD_PATH                  := $(ROOT_PATH)/build/$(TARGET_NAME)
LINKER_SCRIPT               := $(PROJECT_PATH)/linker.ld
KERNEL_API                  := $(ROOT_PATH)/src/kernel/include/

# compilers
COMPILER                    := x86_64-elf-g++
ASSEMBLER                   := nasm
LINKER                      := x86_64-elf-ld

# compiler flags
# [!!!] -mno-sse2: SSE2 is disabled
COMPILER_FLAGS              := -nostdlib -m64 -mno-red-zone -mno-sse2 -fno-rtti -fno-exceptions -Werror -Wunused-result -fno-use-cxa-atexit -ffreestanding -I$(HEADER_FILES_PATH) -I$(KERNEL_API) -g -z max-page-size=0x1000 -fPIC
NASM_FLAGS                  := -f elf64
LINKER_FLAGS                := -shared -z max-page-size=0x1000 -T $(LINKER_SCRIPT) -static --unresolved-symbols=ignore-all

# file sources & targets
source_files                := $(shell find $(SOURCE_FILES_PATH) -name "*.cpp")
object_files                := $(patsubst $(SOURCE_FILES_PATH)/%.cpp, $(BUILD_PATH)/objects/%.o, $(source_files))
asm_files                   := $(shell find $(ASSEMBLY_FILES_PATH) -name "*.asm")
asm_object_files            := $(patsubst $(ASSEMBLY_FILES_PATH)/%.asm, $(BUILD_PATH)/objects/%.o, $(asm_files))

# silent mode
SILENT_MODE = @

.PHONY: build clean rebuild

build: $(ROOT_PATH)/iso/$(TARGET_NAME).sys

$(ROOT_PATH)/iso/$(TARGET_NAME).sys: $(BUILD_PATH)/$(TARGET_NAME).sys
> @echo "copying $(TARGET_NAME)"
> @mkdir -p $(dir $@)
> @cp $< $@

$(BUILD_PATH)/$(TARGET_NAME).sys: $(object_files) $(asm_object_files) $(LINKER_SCRIPT)
> @echo "linking kernel binary"
> @mkdir -p $(dir $@)
> $(SILENT_MODE)$(LINKER) $(LINKER_FLAGS) -o $@ $(object_files) $(asm_object_files)

$(BUILD_PATH)/objects/%.o: $(SOURCE_FILES_PATH)/%.cpp
> @echo "compiling: $<"
> @mkdir -p $(dir $@)
> $(SILENT_MODE)$(COMPILER) $(COMPILER_FLAGS) -c $< -o $@

$(BUILD_PATH)/objects/%.o: $(ASSEMBLY_FILES_PATH)/%.asm
> @echo "assembling: $<"
> @mkdir -p $(dir $@)
> $(SILENT_MODE)$(ASSEMBLER) $(NASM_FLAGS) $< -o $@

clean:
> @echo "cleaning build directory"
> @rm -rf $(BUILD_PATH)/*

rebuild: clean build

-include $(object_files:.o=.d)

$(BUILD_PATH)/objects/%.d: $(SOURCE_FILES_PATH)/%.cpp
> @mkdir -p $(dir $@)
> $(SILENT_MODE)$(COMPILER) $(COMPILER_FLAGS) -MM -MT $(@:.d=.o) $< > $@