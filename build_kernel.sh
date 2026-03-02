#!/bin/bash

# ~~ build script to automate the building process on docker

export GIT_COMMIT_HASH

# network drivers
echo "Building drivers ..."
cd src/network_drivers
make -j$(nproc) build
cd ../..

# kernel
echo "Building kernel ..."
make -j$(nproc) build

# generate gdb symbol file
# echo "Generating gdb symbols (for kernel) ..."
# TEXT_ADDR=$(x86_64-elf-objdump -h ./build/VirtualReflectionsOS.bin | awk '/.text/{print $4; exit}')
# HIGH_ADDR=$(printf "0xFFFFF800%08X" $((16#${TEXT_ADDR})))
# ESCAPED_PATH=$(echo "$WINDOWS_PATH" | sed 's/\\/\\\\/g')
# echo "add-symbol-file \"${ESCAPED_PATH}\\\\build\\\\VirtualReflectionsOS.bin\" $HIGH_ADDR" > ./build/.gdbinit_symbols