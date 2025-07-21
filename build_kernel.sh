#!/bin/bash

# ~~ build script to automate the building process on docker

make build

# other project
cd src/test_driver
make build