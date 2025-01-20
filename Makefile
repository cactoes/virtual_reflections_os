SOURCE_FILES_PATH = src/os/src
HEADER_FILES_PATH = src/os/include

source_files := $(shell find $(SOURCE_FILES_PATH) -name *.cpp)
object_files := $(patsubst $(SOURCE_FILES_PATH)/%.cpp, build/objects/%.o, $(source_files))

asm_files := $(shell find $(SOURCE_FILES_PATH) -name *.asm)
asm_object_files := $(patsubst $(SOURCE_FILES_PATH)/%.asm, build/objects/%.o, $(asm_files))

# [!!!] -mno-sse2: SSE2 is disabled
GPP_COMPILE_FLAGS = -z max-page-size=0x1000 -nostdlib -m64 -mno-red-zone -mno-sse2 -fno-rtti -fno-exceptions -Werror -Wunused-result

$(object_files): build/objects/%.o : $(SOURCE_FILES_PATH)/%.cpp
	mkdir -p $(dir $@) && \
	x86_64-elf-g++ -g $(GPP_COMPILE_FLAGS) -c -I $(HEADER_FILES_PATH) -ffreestanding $(patsubst build/objects/%.o, $(SOURCE_FILES_PATH)/%.cpp, $@) -o $@

$(asm_object_files): build/objects/%.o : $(SOURCE_FILES_PATH)/%.asm
	mkdir -p $(dir $@) && \
	nasm -f elf64 $(patsubst build/objects/%.o, $(SOURCE_FILES_PATH)/%.asm, $@) -o $@

generate_grub_files:
	mkdir build/iso
	mkdir build/iso/boot
	mkdir build/iso/boot/grub
	echo 'set timeout=0' >> build/iso/boot/grub/grub.cfg
	echo 'set default=0' >> build/iso/boot/grub/grub.cfg
	echo 'menuentry "VirtualReflectionsOS" {' >> build/iso/boot/grub/grub.cfg
	echo '    multiboot /boot/VirtualReflectionsOS.bin' >> build/iso/boot/grub/grub.cfg
	echo '    boot' >> build/iso/boot/grub/grub.cfg
	echo '}' >> build/iso/boot/grub/grub.cfg

.PHONY: build
build: $(object_files) $(asm_object_files) generate_grub_files
	x86_64-elf-ld -z max-page-size=0x1000 -n -o build/VirtualReflectionsOS.bin -T $(SOURCE_FILES_PATH)/critical/linker.ld $(object_files) $(asm_object_files) && \
	cp build/VirtualReflectionsOS.bin build/iso/boot/VirtualReflectionsOS.bin && \
	grub-mkrescue /usr/lib/grub/i386-pc -o build/VirtualReflectionsOS.iso build/iso && \
	rm -rf build/iso

clean:
	rm -rf build/iso && \
	rm -rf build/objects && \
	rm -rf build/VirtualReflectionsOS.bin && \
	rm -rf build/VirtualReflectionsOS.iso