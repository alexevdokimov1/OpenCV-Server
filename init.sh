#!/bin/bash
sudo apt update && sudo apt upgrade
sudo apt-get install -y git
git clone https://github.com/alexevdokimov1/OpenCV-Server.git
cd OpenCV-Server/Linux/nginx
./install.sh
cd ../Server
./OpenCVInstall.sh
./launch