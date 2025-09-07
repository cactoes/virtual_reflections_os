#!/bin/bash

# ~~ build script to automate the building process on docker

# network drivers
cd src/network_drivers
make -j$(nproc) build
cd ../..

# kernel
make -j$(nproc) build