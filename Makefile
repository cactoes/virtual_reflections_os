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
COMPILER                    := aarch64-linux-gnu-g++
LINKER                      := aarch64-linux-gnu-ld
OBJCOPY                     := aarch64-linux-gnu-objcopy

# compiler flags
# -mgeneral-regs-only
COMPILER_FLAGS := -ffreestanding -nostdlib -O2 -Wall -Wextra -march=armv8-a -I$(HEADER_FILES_PATH) -fno-exceptions -fno-rtti -fno-use-cxa-atexit -I$(SHARED_HEADERS)
LINKER_FLAGS := -T $(LINKER_SCRIPT) -nostdlib

# compile time define
COMPILER_FLAGS += -DGIT_COMMIT_HASH=\"$(GIT_COMMIT_HASH)\"

# file sources & targets
source_files                := $(shell find $(SOURCE_FILES_PATH) -name "*.cpp")
object_files                := $(patsubst $(SOURCE_FILES_PATH)/%.cpp, $(BUILD_PATH)/objects/%.o, $(source_files))
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

$(BUILD_PATH)/objects/%.o: $(SOURCE_FILES_PATH)/%.cpp
> @echo "compiling: $<"
> @mkdir -p $(dir $@)
> $(SILENT_MODE)$(COMPILER) $(COMPILER_FLAGS) -c $< -o $@

$(BUILD_PATH)/objects/%.o: $(ASSEMBLY_FILES_PATH)/%.S
> @echo "assembling: $<"
> @mkdir -p $(dir $@)
> $(SILENT_MODE)$(COMPILER) $(COMPILER_FLAGS) -c $< -o $@

clean:
> @echo "cleaning build directory"
> @rm -rf $(BUILD_PATH)/*

rebuild: clean build