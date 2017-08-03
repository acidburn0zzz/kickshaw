#!/bin/bash

cd source/
make
clear
echo "#################################################################
#####       The following requires root privileges        ########
##################################################################"
sudo make install

exit 0
