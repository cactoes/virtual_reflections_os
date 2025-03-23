SOURCE_FILES_PATH = src/kernel/src
HEADER_FILES_PATH = src/kernel/include
ASSEMBLY_FILES_PATH = src/kernel/asm

# [!!!] -mno-sse2: SSE2 is disabled
GPP_COMPILE_FLAGS = -nostdlib -m64 -mno-red-zone -mno-sse2 -fno-rtti -fno-exceptions -Werror -Wunused-result

source_files := $(shell find $(SOURCE_FILES_PATH) -name *.cpp)
object_files := $(patsubst $(SOURCE_FILES_PATH)/%.cpp, build/objects/%.o, $(source_files))

asm_files := $(shell find $(ASSEMBLY_FILES_PATH) -name *.asm)
asm_object_files := $(patsubst $(ASSEMBLY_FILES_PATH)/%.asm, build/objects/%.o, $(asm_files))

$(object_files): build/objects/%.o : $(SOURCE_FILES_PATH)/%.cpp
	mkdir -p $(dir $@) && \
	x86_64-elf-g++ -z max-page-size=0x1000 -g $(GPP_COMPILE_FLAGS) -c -I $(HEADER_FILES_PATH) -ffreestanding $(patsubst build/objects/%.o, $(SOURCE_FILES_PATH)/%.cpp, $@) -o $@

$(asm_object_files): build/objects/%.o : $(ASSEMBLY_FILES_PATH)/%.asm
	mkdir -p $(dir $@) && \
	nasm -f elf64 $(patsubst build/objects/%.o, $(ASSEMBLY_FILES_PATH)/%.asm, $@) -o $@

.PHONY: build clean

build: $(object_files) $(asm_object_files)
	cp -r iso build/ && \
	x86_64-elf-ld -z max-page-size=0x1000 -n -o build/VirtualReflectionsOS.bin -T linker.ld $(object_files) $(asm_object_files) && \
	cp build/VirtualReflectionsOS.bin build/iso/boot/VirtualReflectionsOS.bin && \
	grub-mkrescue /usr/lib/grub/i386-pc -o build/VirtualReflectionsOS.iso build/iso

clean:
	rm -rf build/*