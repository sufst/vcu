#!/bin/bash

set -exo pipefail

echo "Installing dependencies"
sudo apt-get update
sudo apt-get install -y gcc-arm-none-eabi stlink-tools python3-pip clang-format

# If repo already got cloned then skip
if ! [[ -f vcu-setup-script.sh ]]; then
	git clone https://github.com/sufst/vcu sufst-vcu
	cd sufst-vcu
else
	git pull
fi

echo "Initialising submodules"
git submodule init
git submodule update

echo "Setting up venv"
python3 -m venv .venv
# shellcheck disable=SC1091 # github actions doesn't have the venv setup
source .venv/bin/activate
python3 -m pip install ccdgen
make -s ccd

echo "Setting up Trunk"
curl https://get.trunk.io -fsSL | bash
trunk init || echo "trunk already initialised, skipping"
trunk check

echo

echo "Ensure that Visual Studio Code is installed on Windows!"
echo "Then install the WSL extension"
echo "once complete run 'code .' in wsl"
