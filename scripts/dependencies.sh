#!/bin/bash
echo "THIS SCRIPT WILL NOW BEGIN INSTALLING ALL THE DEPENDENCIES NEEDED BY THE ATLAS_CLIENT"

# Update repository definitions
echo "*********************************************************"
echo "*********** Updating repository definitions *************"
echo "*********************************************************"
sudo apt-get update
echo "*********************************************************"
echo "************ Repository definitions updated *************"
echo "*********************************************************"

# Install Eclipse Paho and its dependencies, with support for Cecho
echo "*********************************************************"
echo "***** Installing Eclipse Paho and its dependencies ******"
echo "*********************************************************"
# __dependencies__
echo "************ Step 1. Installing dependencies ************"
sudo apt-get install -y build-essential gcc make cmake cmake-gui cmake-curses-gui
sudo apt-get install -y libssl-dev
sudo apt-get install -y git
echo "******************* Step 1 finished! ********************"
# __build Paho with support for C__
echo "*** Step 2. Building Eclipse Paho with support for C ****"
git clone https://github.com/eclipse/paho.mqtt.c.git
cd paho.mqtt.c
git checkout v1.3.1
cmake -Bbuild -H. -DPAHO_WITH_SSL=ON -DPAHO_ENABLE_TESTING=OFF
sudo cmake --build build/ --target install
sudo ldconfig
cd ..
echo "******************* Step 2 finished! ********************"
echo "*********************************************************"
echo "********* Eclipse Paho successfully installed! **********"
echo "*********************************************************"

# Build and install libcoap
echo "*********************************************************"
echo "***************** Adding suport for CoAP ****************"
echo "*********************************************************"
sudo apt-get install -y autotools-dev autoconf automake m4 libtool pkg-config
git clone https://github.com/obgm/libcoap.git
cd libcoap
git checkout develop
sudo sh ./autogen.sh
cp ../ltmain.sh .
sudo sh ./autogen.sh
sudo sh ./configure --with-openssl --enable-shared --disable-documentation --disable-examples
make
sudo make install
cd ..
echo "*********************************************************"
echo "****************** CoAP support added *******************"
echo "*********************************************************"

# Add support for UUID
echo "*********************************************************"
echo "**************** Adding support for UUID ****************"
echo "*********************************************************"
sudo apt-get install -y uuid-dev uuid uuid-runtime
echo "*********************************************************"
echo "****************** UUID support added *******************"
echo "*********************************************************"

echo "  ALL ATLAS_CLIENT DEPENDENCIES HAVE BEEN INSTALLED"
