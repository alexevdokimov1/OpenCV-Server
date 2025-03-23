#!/bin/bash
sudo apt update && sudo apt upgrade
sudo apt-get install -y git
git clone https://github.com/alexevdokimov1/OpenCV-Server.git
cd OpenCV-Server/Linux
chmod +x OpenCVInstall.sh
./OpenCVInstall.sh
cd Server
chmod +x launch.sh
./launch.sh