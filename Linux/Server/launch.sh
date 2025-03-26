#!/bin/bash

mkdir -p build && cd build
cmake ..
make
chmod +x Server
./Server &