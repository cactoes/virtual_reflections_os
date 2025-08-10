#!/bin/bash

# ~~ build script to automate the building process on docker

# test driver
cd src/test_driver
make build
cd ../..

# kernel
make build