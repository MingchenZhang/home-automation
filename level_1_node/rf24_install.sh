#!/bin/bash
git clone https://github.com/TMRh20/RF24.git
cd RF24
echo "please uncomment failure_handling and continue"
read -p "Press any key to continue..." -n1 -s
make clean
make
sudo make install
