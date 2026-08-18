#==========================================
## @file       Makefile
## @brief      build script for the calculator
#==========================================

# makefile config
.RECIPEPREFIX = >

# directories & files
TARGET_NAME                 := Calculator
PROJECT_PATH                := .
SOURCE_FILES_PATH           := $(PROJECT_PATH)/src
HEADER_FILES_PATH           := $(PROJECT_PATH)/include
ASSEMBLY_FILES_PATH         := $(PROJECT_PATH)/asm
ROOT_PATH                   := ../..
BUILD_PATH                  := $(ROOT_PATH)/build/$(TARGET_NAME)
LINKER_SCRIPT               := $(PROJECT_PATH)/linker.ld
KERNEL_API                  := $(ROOT_PATH)/src/kernel/include/kernel_include
SHARED_FILES                := $(ROOT_PATH)/src/shared

# compilers
COMPILER                    := x86_64-elf-g++
ASSEMBLER                   := nasm
LINKER                      := x86_64-elf-ld

# compiler flags
COMPILER_FLAGS 				:= -nostdlib -mno-sse2 -m64 -mno-red-zone -fno-rtti -fno-exceptions -Werror -Wunused-result -fno-use-cxa-atexit -ffreestanding -I$(HEADER_FILES_PATH) -I$(KERNEL_API) -I$(SHARED_FILES)/include -g
NASM_FLAGS                  := -f elf64
LINKER_FLAGS                := -z max-page-size=0x1000 -T $(LINKER_SCRIPT) -static

# file sources & targets
source_files                := $(shell find $(SOURCE_FILES_PATH) -name "*.cpp")
object_files                := $(patsubst $(SOURCE_FILES_PATH)/%.cpp, $(BUILD_PATH)/objects/%.o, $(source_files))

shared_files                := $(shell find $(SHARED_FILES)/src -name "*.cpp")
shared_object_files         := $(patsubst $(SHARED_FILES)/src/%.cpp, $(BUILD_PATH)/objects/%.o, $(shared_files))

asm_files                   := $(shell find $(ASSEMBLY_FILES_PATH) -name "*.asm")
asm_object_files            := $(patsubst $(ASSEMBLY_FILES_PATH)/%.asm, $(BUILD_PATH)/objects/%.o, $(asm_files))

# silent mode
SILENT_MODE = @

.PHONY: build clean rebuild

build: $(ROOT_PATH)/iso/$(TARGET_NAME).exe

$(ROOT_PATH)/iso/$(TARGET_NAME).exe: $(BUILD_PATH)/$(TARGET_NAME).exe
> @echo "copying $(TARGET_NAME)"
> @mkdir -p $(dir $@)
> @cp $< $@

$(BUILD_PATH)/$(TARGET_NAME).exe: $(asm_object_files) $(object_files) $(shared_object_files) $(LINKER_SCRIPT)
> @echo "linking $(TARGET_NAME) binary"
> @mkdir -p $(dir $@)
> $(SILENT_MODE)$(LINKER) $(LINKER_FLAGS) -o $@ $(asm_object_files) $(object_files) $(shared_object_files)

$(BUILD_PATH)/objects/%.o: $(SHARED_FILES)/src/%.cpp
> @echo "compiling: $<"
> @mkdir -p $(dir $@)
> $(SILENT_MODE)$(COMPILER) $(COMPILER_FLAGS) -c $< -o $@

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
> @rm -f $(ROOT_PATH)/iso/$(TARGET_NAME).sys

rebuild: clean build

-include $(object_files:.o=.d)

$(BUILD_PATH)/objects/%.d: $(SOURCE_FILES_PATH)/%.cpp
> @mkdir -p $(dir $@)
> $(SILENT_MODE)$(COMPILER) $(COMPILER_FLAGS) -MM -MT $(@:.d=.o) $< > $@