#!/bin/bash
sudo apt update
sudo apt-get install -y nginx
sudo cp nginx.conf /etc/nginx
sudo nginx -t
sudo nginx -s reload