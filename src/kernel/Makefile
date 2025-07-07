#==========================================
## @file       Makefile
## @brief      build script for src/kernel
#==========================================

SOURCE_FILES_PATH = src
HEADER_FILES_PATH = include
ASSEMBLY_FILES_PATH = asm
BUILD_PATH = ../../build

# [!!!] -mno-sse2: SSE2 is disabled
GPP_COMPILE_FLAGS = -nostdlib -m64 -mno-red-zone -mno-sse2 -fno-rtti -fno-exceptions -Werror -Wunused-result -fno-use-cxa-atexit

source_files := $(shell find $(SOURCE_FILES_PATH) -name *.cpp)
object_files := $(patsubst $(SOURCE_FILES_PATH)/%.cpp, $(BUILD_PATH)/objects/%.o, $(source_files))

asm_files := $(shell find $(ASSEMBLY_FILES_PATH) -name *.asm)
asm_object_files := $(patsubst $(ASSEMBLY_FILES_PATH)/%.asm, $(BUILD_PATH)/objects/%.o, $(asm_files))

$(object_files): $(BUILD_PATH)/objects/%.o : $(SOURCE_FILES_PATH)/%.cpp
	mkdir -p $(dir $@) && \
	x86_64-elf-g++ -z max-page-size=0x1000 -g $(GPP_COMPILE_FLAGS) -c -I $(HEADER_FILES_PATH) -ffreestanding $(patsubst $(BUILD_PATH)/objects/%.o, $(SOURCE_FILES_PATH)/%.cpp, $@) -o $@

$(asm_object_files): $(BUILD_PATH)/objects/%.o : $(ASSEMBLY_FILES_PATH)/%.asm
	mkdir -p $(dir $@) && \
	nasm -f elf64 $(patsubst $(BUILD_PATH)/objects/%.o, $(ASSEMBLY_FILES_PATH)/%.asm, $@) -o $@

.PHONY: build clean

build: $(object_files) $(asm_object_files)
	cp -r ../../iso $(BUILD_PATH)/ && \
	x86_64-elf-ld -z max-page-size=0x1000 -n -o $(BUILD_PATH)/VirtualReflectionsOS.bin -T linker.ld $(object_files) $(asm_object_files) && \
	cp $(BUILD_PATH)/VirtualReflectionsOS.bin $(BUILD_PATH)/iso/boot/VirtualReflectionsOS.bin && \
	grub-mkrescue /usr/lib/grub/i386-pc -o $(BUILD_PATH)/VirtualReflectionsOS.iso $(BUILD_PATH)/iso

clean:
	rm -rf build/*