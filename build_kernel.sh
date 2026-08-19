#!/bin/bash

# ~~ build script to automate the building process on docker

export GIT_COMMIT_HASH

build_component() {
    echo "Building $1 ..."
    make -C "$2" -j$(nproc) build
}

build_component "Network Drivers" src/network_drivers
build_component "Minesweeper"     src/minesweeper
build_component "Calculator"      src/console_calculator
build_component "Kernel"          src/kernel

# generate gdb symbol file
# echo "Generating gdb symbols (for kernel) ..."
# TEXT_ADDR=$(x86_64-elf-objdump -h ./build/VirtualReflectionsOS.bin | awk '/.text/{print $4; exit}')
# HIGH_ADDR=$(printf "0xFFFFF800%08X" $((16#${TEXT_ADDR})))
# ESCAPED_PATH=$(echo "$WINDOWS_PATH" | sed 's/\\/\\\\/g')
# echo "add-symbol-file \"${ESCAPED_PATH}\\\\build\\\\VirtualReflectionsOS.bin\" $HIGH_ADDR" > ./build/.gdbinit_symbols