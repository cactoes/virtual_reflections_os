#!/bin/bash

# ~~ build script to automate the building process on docker

cd src/kernel
make clean
make build