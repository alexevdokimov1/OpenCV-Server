#!/bin/bash

mkdir build
cp ./images/* ./build
cp ./config/* ./build
cd build
cmake ..
make
./Client