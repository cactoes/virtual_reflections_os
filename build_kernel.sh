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