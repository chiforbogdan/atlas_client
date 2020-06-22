#!/bin/bash
echo "THIS SCRIPT WILL NOW BUILD THE ATLAS_CLIENT SECURITY COMPONENTS"

# Create random socket unix path (atlas client to atlas data plane communication)
rand=`cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w ${1:-32} | head -n 1`
echo "#define ATLAS_DATA_PLANE_UNIX_SOCKET_PATH \"/tmp/atlas_data_plane_$rand\"" > src/utils/atlas_unix_socket_path.h 

# Create Build directory
echo "*********** Step 1. Creating build directories ************"
mkdir build build/data_plane
echo "******************* Step 1 finished! ********************"

# Building project
echo "******* Step 2. Building ATLAS_CLIENT components *******"
cd build/
cmake ..
make
echo "******************* Step 2 finished! ********************"

echo "  ALL ATLAS_CLIENT COMPONENTS HAVE BEEN BUILT"
