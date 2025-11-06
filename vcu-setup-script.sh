#!/bin/bash
sudo apt update && sudo apt upgrade
mkdir sufst
cd sufst
git clone https://github.com/sufst/vcu

sudo apt install gcc-arm-none-eabi
sudo apt install stlink-tools

cd vcu
git submodule init
git submodule update

sudo apt install python3-pip
python3 -m pip install ccdgen --break-system-packages
make -s ccd

echo "Ensure that Visual Studio Code is installed on Windows!"
echo "Then install the WSL extension"
echo "once complete run 'code .' in wsl