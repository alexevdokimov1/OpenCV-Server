#!/bin/bash

sudo apt update && sudo apt upgrade
sudo apt install -y g++ cmake make git libgtk2.0-dev pkg-config
uname -m && cat /etc/*release

echo Installing drivers
sudo apt-get install -y nvidia-open


wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2404/x86_64/cuda-ubuntu2404.pin
sudo mv cuda-ubuntu2404.pin /etc/apt/preferences.d/cuda-repository-pin-600
wget https://developer.download.nvidia.com/compute/cuda/12.8.1/local_installers/cuda-repo-ubuntu2404-12-8-local_12.8.1-570.124.06-1_amd64.deb
sudo dpkg -i cuda-repo-ubuntu2404-12-8-local_12.8.1-570.124.06-1_amd64.deb
sudo cp /var/cuda-repo-ubuntu2404-12-8-local/cuda-*-keyring.gpg /usr/share/keyrings/
sudo apt-get update
sudo apt-get -y install cuda-toolkit-12-8

wget https://developer.download.nvidia.com/compute/cudnn/9.8.0/local_installers/cudnn-local-repo-ubuntu2404-9.8.0_1.0-1_amd64.deb
sudo dpkg -i cudnn-local-repo-ubuntu2404-9.8.0_1.0-1_amd64.deb
sudo cp /var/cudnn-local-repo-ubuntu2404-9.8.0/cudnn-*-keyring.gpg /usr/share/keyrings/
sudo apt-get update
sudo apt-get -y install cudnn
sudo apt-get -y install cudnn-cuda-12


#git clone https://github.com/opencv/opencv.git
#git clone https://github.com/opencv/opencv_contrib.git
mkdir -p build && cd build
cmake   -D CMAKE_BUILD_TYPE=RELEASE \
        -D WITH_CUDA=ON \
        -D OPENCV_DNN_CUDA=ON \
        -D BUILD_opencv_dnn=ON \
        -D ENABLE_FAST_MATH=ON \
        -D BUILD_opencv_world=ON \
        -D OPENCV_EXTRA_MODULES_PATH=../opencv_contrib/modules \
        -D ENABLE_FAST_MATH=ON \
        -D CUDA_ARCH_BIN="8.6" \
        -D WITH_CUDNN=ON \
        -D CUDA_FAST_MATH=ON \
        ../opencv
make -j4
sudo make install