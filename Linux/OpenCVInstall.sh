#!/bin/bash

sudo apt update && sudo apt upgrade
sudo apt-get install -y g++ cmake make git libgtk2.0-dev pkg-config
cd ~
git clone https://github.com/opencv/opencv.git
mkdir -p build && cd build
cmake ../opencv
make -j4
sudo make install