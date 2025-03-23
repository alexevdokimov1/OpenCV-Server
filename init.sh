#!/bin/bash
sudo apt update && sudo apt upgrade
sudo apt-get install -y git
git clone https://github.com/alexevdokimov1/OpenCV-Server.git
cd OpenCV-Server/Linux/nginx
chmod +x install.sh
./install.sh
cd ../Server
chmod +x OpenCVInstall.sh
./OpenCVInstall.sh
chmod +x launch.sh
./launch.sh