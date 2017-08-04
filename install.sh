#!/bin/bash

cd source/

make

clear

echo "##################################################################
#####       The following requires root privileges        ########
##################################################################"

sudo make install

# removing folder after install
cd ../..
rm -rf kickshaw

clear

echo "##################################################################
#####         The installation was successfull            ########
##################################################################"

sleep 1

exit 0
