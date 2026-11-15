#!/bin/bash

# ~~ build script to automate the building process on docker

export GIT_COMMIT_HASH

build_component() {
    echo "Building $1 ..."
    make -C "$2" -j$(nproc) build
}

build_component "Kernel" src/kernel