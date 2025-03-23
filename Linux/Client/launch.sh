#!/bin/bash

mkdir build
mkdir -p ./build/images/input
cp ./images/* ./build/images/input
cd build
mkdir -p ./images/output
cmake ..
make
./Client