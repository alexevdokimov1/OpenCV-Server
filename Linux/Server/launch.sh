#!/bin/bash

mkdir build && cd build
cmake ..
make
chmod +x Server
./Server